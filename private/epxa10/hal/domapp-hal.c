#include <math.h>

#include "hal/DOM_MB_domapp.h"
#include "hal/DOM_FPGA_domapp_regs.h"
#include "booter/epxa.h"

#define lengthof(a) (sizeof(a)/sizeof(a[0]))

/* irq handler, for:
 *
 * cal flash indicator
 * rate meter done
 * sn event ready
 */
#define INT_MASK_SET       (REGISTERS + 0xc00)
#define INT_MASK_CLEAR     (REGISTERS + 0xc04)
#define INT_SOURCE_STATUS  (REGISTERS + 0xc08)
#define INT_REQUEST_STATUS (REGISTERS + 0xc0c)
#define INT_ID             (REGISTERS + 0xc10)
#define INT_MODE           (REGISTERS + 0xc18)
#define INT_PRIORITY_PLD0  (REGISTERS + 0xc80)
#define INT_PRIORITY_PLD1  (REGISTERS + 0xc84)
#define INT_PRIORITY_PLD2  (REGISTERS + 0xc88)
#define INT_REG(a) (* (unsigned volatile *) INT_##a )

static inline unsigned intID(void)        { return INT_REG(ID); }
static inline unsigned intRequestStatus() { return INT_REG(REQUEST_STATUS); }

#define FLASH_NOW_IRQ  10
#define RATE_METER_IRQ 11
#define SN_UPDATE_IRQ  12

enum IrqNumbers {
   IRQ_CAL_FLASH  = 1, /* pld irq 0 */
   IRQ_RATE_METER = 2, /* pld irq 1 */
   IRQ_SN_UPDATE  = 4  /* pld irq 2 */
}; 

/* we keep a queue of sn events, we would like
 * to keep 10s worth of data, this amounts to
 * 10 s / 4 e/1.6ms -> 4000 entries...
 */
static unsigned short snHead, snTail;
static SNEvent snEvents[4096];

static unsigned short speHead, speTail, mpeHead, mpeTail;
static unsigned speQueue[128], mpeQueue[128];

static unsigned cmHead, cmTail;
static unsigned cmQueue[4096];
static int cmPending; /* is there an interrupt pending? */

static void irqHandler(void) {
   if (intRequestStatus()==IRQ_CAL_FLASH) {
      if (cmHead!=cmTail) {
         const unsigned idx = cmTail % lengthof(cmQueue);
         FPGA(CAL_TIME) = cmQueue[idx];
         cmTail++;
      }
      else {
         /* interrupt when empty means that we won't be able
          * to help in the future...
          */
         cmPending = 0;
      }
      
      FPGA(INT_ACK) = FPGABIT(INT_ACK, CAL);
   }
   else if (intRequestStatus()==IRQ_RATE_METER) {
      /* read rate meter... */
      speQueue[speHead % lengthof(speQueue)] = FPGA(RATE_SPE);
      mpeQueue[mpeHead % lengthof(mpeQueue)] = FPGA(RATE_MPE);
      speHead++; mpeHead++;
      FPGA(INT_ACK) = FPGABIT(INT_ACK, RATE);
   }
   else if (intRequestStatus()==IRQ_SN_UPDATE) {
      /* FIXME: deal with 32 bit rollover... */
      int i;
      unsigned sn = FPGA(SN_DATA);
      unsigned long long ticks = 
         ((unsigned long long) FPGA(SYSTIME_MSB) << 32) | (sn&0xffff0000) | 1;
      
      /* get sn data */
      for (i=0; i<4; i++) {
         const int idx = snHead % lengthof(snEvents);
         snEvents[idx].counts = (sn>>(4*i))&0xf;
         snEvents[idx].ticks = ticks + 65536 * i;
         snHead++;
      }

      /* ack the interrupt... */
      FPGA(INT_ACK) = FPGABIT(INT_ACK, SN);
   }
   else {
      /* yikes!!! */
   }
}

static void disableIRQ(enum IrqNumbers irq) {
   unsigned mask = ~ (unsigned) irq;
   FPGA(INT_EN) &= mask;
   * (unsigned volatile *) INT_MASK_CLEAR |= irq; 
}

static void enableIRQ(enum IrqNumbers irq) {
   static int isinit;
  
   /* make sure handler is installed... */
   if (!isinit) {
      /* install irq handler... */
      extern void irqInstall( void (*handler)(void) );
      irqInstall(irqHandler);
      *(volatile unsigned *) INT_MODE = 3; /* "normal" mode */
      isinit=1;
   }

   /* set interrupt priority... */
   if (irq==IRQ_CAL_FLASH) {
      *(volatile unsigned *) INT_PRIORITY_PLD0 = FLASH_NOW_IRQ;
   }
   else if (irq==IRQ_SN_UPDATE) {
      *(volatile unsigned *) INT_PRIORITY_PLD1 = SN_UPDATE_IRQ;
   }
   else if (irq==IRQ_RATE_METER) {
      *(volatile unsigned *) INT_PRIORITY_PLD2 = RATE_METER_IRQ;
   } 
   else {
      return;
   }

   /* enable it in stripe... */
   FPGA(INT_EN) |= irq;
   * (unsigned volatile *) INT_MASK_SET |= irq;
}

unsigned long long hal_FPGA_DOMAPP_get_local_clock(void) {
   unsigned long long h1 = FPGA(SYSTIME_MSB);
   unsigned long long l1 = FPGA(SYSTIME_LSB);
   unsigned long long h2 = FPGA(SYSTIME_MSB);
   
   if (h1==h2) return (h1<<32)|l1;
   if (l1<0x80000000ULL) return (h2<<32)|l1;
   return (h1<<32)|l1;
}

void hal_FPGA_DOMAPP_trigger_source(int srcs) {
   FPGA(TRIGGER_SOURCE) = srcs & 0x1ff;
}

void hal_FPGA_DOMAPP_enable_daq(void) {
   FPGA(DAQ) |= FPGABIT(DAQ, ENABLE_DAQ);
}

void hal_FPGA_DOMAPP_disable_daq(void) {
   FPGA(DAQ) &= ~FPGABIT(DAQ, ENABLE_DAQ);
}

void hal_FPGA_DOMAPP_enable_atwds(int mask) {
   FPGA(DAQ) = (FPGA(DAQ) & ~FPGABIT(DAQ, ENABLE_ATWD)) | mask;
}

void hal_FPGA_DOMAPP_daq_mode(HAL_FPGA_DOMAPP_DAQ_MODES mode) {
   FPGA(DAQ) = ( FPGA(DAQ) & ~FPGABIT(DAQ, DAQ_MODE)) | mode;
}

void hal_FPGA_DOMAPP_atwd_mode(HAL_FPGA_DOMAPP_ATWD_MODES mode) {
   FPGA(DAQ) = ( FPGA(DAQ) & ~FPGABIT(DAQ, ATWD_MODE)) | mode;
}

void hal_FPGA_DOMAPP_lc_mode(HAL_FPGA_DOMAPP_LC_MODES mode) {
   FPGA(DAQ) = ( FPGA(DAQ) & ~FPGABIT(DAQ, LC_MODE)) | mode;
}

void hal_FPGA_DOMAPP_lbm_mode(HAL_FPGA_DOMAPP_LBM_MODES mode) {
   FPGA(DAQ) = ( FPGA(DAQ) & ~FPGABIT(DAQ, LBM_MODE) ) | mode;
}

void hal_FPGA_DOMAPP_compression_mode(HAL_FPGA_DOMAPP_COMPRESSION_MODES mode) {
   FPGA(DAQ) = ( FPGA(DAQ) & ~FPGABIT(DAQ, COMP_MODE) ) | mode;
}

void hal_FPGA_DOMAPP_lbm_reset(void) {
   FPGA(LBM_CONTROL) |= FPGABIT(LBM_CONTROL, RESET);
}

unsigned hal_FPGA_DOMAPP_lbm_pointer(void) { 
   return FPGA(LBM_POINTER); 
}

unsigned *hal_FPGA_DOMAPP_lbm_address(void) { 
   return (unsigned *) 0x01000000;
}

void hal_FPGA_DOMAPP_lc_enable(int mask) {
   FPGA(LC_CONTROL) = 
      ( FPGA(LC_CONTROL) & ~FPGABIT(LC_CONTROL, ENABLE) ) | mask;
}

void hal_FPGA_DOMAPP_lc_span(int doms) {
   FPGA(LC_CONTROL) = 
      ( FPGA(LC_CONTROL) & ~FPGABIT(LC_CONTROL, LC_LENGTH) ) | 
      (((doms&3)-1)<<4);
}

/**
 * lc cable length correction
 *
 * \param dist neighbor distance (0..3)
 * \param ns correction in ns (precise to 25ns)
 * \returns actual ns of correction or -1 on error
 */
static int lcLength(unsigned *reg, int dist, int ns) {
   int ticks = ns/25;

   if (ticks<0) ticks=0;
   if (ticks>127) ticks=127;

   dist&=3;

   {
      const unsigned mask = (0x7f << (dist*8));
      *(unsigned volatile *) reg = 
         ((*(unsigned volatile *) reg) & ~mask) | (ticks<<dist*8);
   }

   return ticks*25;
}

int hal_FPGA_DOMAPP_lc_length_up(int dist, int ns) {
   return lcLength( (unsigned *) DOM_FPGA_LC_CABLE_LENGTH_UP, dist, ns);
}

int hal_FPGA_DOMAPP_lc_length_down(int dist, int ns) {
   return lcLength( (unsigned *) DOM_FPGA_LC_CABLE_LENGTH_DOWN, dist, ns);
}

void hal_FPGA_DOMAPP_lc_disc_spe(void) {
   FPGA(LC_CONTROL) &= ~FPGABIT(LC_CONTROL, DISC_SOURCE);
}

void hal_FPGA_DOMAPP_lc_disc_mpe(void) {
   FPGA(LC_CONTROL) |= FPGABIT(LC_CONTROL, DISC_SOURCE);
}

int hal_FPGA_DOMAPP_lc_windows(int pre, int post) {
   if (pre<100 || pre>6200 || post<100 || post>6200) return -1;
   
   pre=(pre-100)/100;
   post=(post-100)/100;

   FPGA(LC_CONTROL) = 
      (FPGA(LC_CONTROL) & 
       ~(FPGABIT(LC_CONTROL, PRE_WINDOW)|FPGABIT(LC_CONTROL, POST_WINDOW)))| 
      (pre<<16)|(post<<24);

   return 0;
}

double hal_FPGA_DOMAPP_cal_pulser_rate(double rateHz) {
   /* rateHz = (1e9 / (25*2^26)) * 2^rate */
   /*        = (1e9 / 25) * 2^(rate - 26) */
   /* -> log2(rateHz) = log2(1e9/25) + (rate - 26) */
   /* -> log2(rateHz) - log2(1e9/25) + 26 = rate */
   /* -> rate = log2(rateHz/(1e9/25)) + 26 */
   /* log2(a) = log(a)/log(2) */
   int rate;
   
   if (rateHz<=0.6) rateHz = 0.69;
   rate = (int)(26 + (log(rateHz/(1e9/25)) / log(2)));
   if (rate<0) rate = 0;
   if (rate>17) rate = 17;
   
   FPGA(CAL_CONTROL) =
      (FPGA(CAL_CONTROL) & ~FPGABIT(CAL_CONTROL, PULSER_RATE))|(rate<<24);
       
   return (1.0e9/(25.0 * (1<<26))) * (1<<rate);
}

void hal_FPGA_DOMAPP_cal_mode(HAL_FPGA_DOMAPP_CAL_MODES mode) {
   static HAL_FPGA_DOMAPP_CAL_MODES save = HAL_FPGA_DOMAPP_CAL_MODE_OFF;

   /* protect from duplicate calls... */
   if (mode==save) return;
   save = mode;
   
   if (mode==HAL_FPGA_DOMAPP_CAL_MODE_MATCH) {
      cmHead = cmTail = cmPending = 0;
      enableIRQ(IRQ_CAL_FLASH);
   }
   else {
      disableIRQ(IRQ_CAL_FLASH);
      cmHead = cmTail = cmPending = 0;
   }
   
   FPGA(CAL_CONTROL) = (FPGA(CAL_CONTROL) & ~FPGABIT(CAL_CONTROL, MODE))|mode;
}

void hal_FPGA_DOMAPP_cal_source(int srcs) {
   FPGA(CAL_CONTROL) = (FPGA(CAL_CONTROL) & ~FPGABIT(CAL_CONTROL, ENABLE)) |
      srcs;
}

int hal_FPGA_DOMAPP_cal_atwd_offset(int offset) {
   if (offset<-200 || offset>175) return -1;
   
   FPGA(CAL_CONTROL) = 
      (FPGA(CAL_CONTROL) & ~FPGABIT(CAL_CONTROL, ATWD_OFFSET)) |
      ((offset/25)<<16);
   
   return 0;
}

int hal_FPGA_DOMAPP_cal_match_time(unsigned clk) {
   if (cmHead - cmTail == lengthof(cmQueue)) return 1;

   cmQueue[cmHead % lengthof(cmQueue)] = clk;
   cmHead++;

   if (cmPending==0) {
      /* if we're the only one in there, we have to put
       * ourselves in the register...
       */
      cmPending = 1;
      FPGA(CAL_TIME) = clk;
   }

   return 0;
}

void hal_FPGA_DOMAPP_cal_launch(void) { FPGA(CAL_LAUNCH) = 0xa5; }

static unsigned long long getLastFlash(void) {
   unsigned long long lsb = FPGA(CAL_LAST_FLASH_LSB);
   unsigned long long msb = FPGA(CAL_LAST_FLASH_MSB);
   return (msb<<32) | lsb;
}

unsigned long long hal_FPGA_DOMAPP_cal_last_flash(void) {
   unsigned long long last = getLastFlash();
   unsigned long long fl;
   while ((fl=getLastFlash())!=last) last = fl;
   return last;
}

void hal_FPGA_DOMAPP_FB_set_aux_reset(void) {
    FPGA(FL_CONTROL) |=  FPGABIT(FL_CONTROL, AUX_RESET);
}

void hal_FPGA_DOMAPP_FB_clear_aux_reset(void) {
    FPGA(FL_CONTROL) &= ~FPGABIT(FL_CONTROL, AUX_RESET);
}

int hal_FPGA_DOMAPP_FB_get_attn(void) {
    return (FPGA(FL_STATUS) & FPGABIT(FL_STATUS, ATTN)) ? 1 : 0;
}

double hal_FPGA_DOMAPP_FB_set_rate(double rateHz) {
   /* Sets flasher board rate.  Uses standard calibration rate routine, */
   /* but clamp maximum rate to 1 kHz */  
   return hal_FPGA_DOMAPP_cal_pulser_rate((rateHz > 1000) ? 1000 : rateHz);
}

void hal_FPGA_DOMAPP_rate_monitor_enable(int mask) {
   if (mask==0) {
      disableIRQ(IRQ_RATE_METER);
      speHead=speTail=mpeHead=mpeTail=0;
   }
   else {
      speHead=speTail=mpeHead=mpeTail=0;
      enableIRQ(IRQ_RATE_METER);
   }
   FPGA(RATE_CONTROL) = 
      ( FPGA(RATE_CONTROL) & FPGABIT(RATE_CONTROL, ENABLE) ) | (mask&3);
}

int hal_FPGA_DOMAPP_rate_monitor_deadtime(int time) {
   if (time<100 || time>102400) return -1;
   
   FPGA(RATE_CONTROL) = 
      ( FPGA(RATE_CONTROL) & FPGABIT(RATE_CONTROL, ENABLE) ) | 
      ((time-100)/100);

   return 0;
}

int hal_FPGA_DOMAPP_spe_rate_ready(void) { return speHead!=speTail; }
int hal_FPGA_DOMAPP_mpe_rate_ready(void) { return mpeHead!=mpeTail; }

int hal_FPGA_DOMAPP_spe_rate(unsigned *spe) {
   int ret = 0;

   while (!hal_FPGA_DOMAPP_spe_rate_ready()) ;

   while (1) {
      *spe = speQueue[speTail % lengthof(speQueue)];

      if (speHead - speTail > lengthof(speQueue) - 1) {
         speTail = speHead - lengthof(speQueue) + 1;
         ret = 1;
      }
      else {
         speTail++;
         break;
      }
   }

   return ret;
}

int hal_FPGA_DOMAPP_mpe_rate(unsigned *mpe) {
   int ret = 0;

   while (!hal_FPGA_DOMAPP_mpe_rate_ready()) ;

   while (1) {
      *mpe = mpeQueue[mpeTail % lengthof(mpeQueue)];

      if (mpeHead - mpeTail >= lengthof(mpeQueue)) {
         mpeTail = mpeHead - lengthof(mpeQueue) + 1;
         ret = 1;
      }
      else {
         mpeTail++;
         break;
      }
   }

   return ret;
}

unsigned hal_FPGA_DOMAPP_spe_rate_immediate(void) { return FPGA(RATE_SPE); }
unsigned hal_FPGA_DOMAPP_mpe_rate_immediate(void) { return FPGA(RATE_MPE); }

void hal_FPGA_DOMAPP_sn_mode(HAL_FPGA_DOMAPP_SN_MODES mode) {
   if (mode==HAL_FPGA_DOMAPP_SN_MODE_OFF) {
      disableIRQ(IRQ_SN_UPDATE);
      snHead = snTail = 0;
   }
   else {
      snHead = snTail = 0;
      enableIRQ(IRQ_SN_UPDATE);
   }

   /* set the bits... */
   FPGA(SN_CONTROL) = 
      ( FPGA(SN_CONTROL) & ~FPGABIT(SN_CONTROL, ENABLE) ) | mode;
}

int hal_FPGA_DOMAPP_sn_dead_time(int time) {
   if (time<6400 || time>512000) return -1;
   
   FPGA(SN_CONTROL) = 
      ( FPGA(SN_CONTROL) & ~FPGABIT(SN_CONTROL, DEAD_TIME) ) | 
   ( ((time - 6400)/6400) << 16 );

   return 0;
}

int hal_FPGA_DOMAPP_sn_ready(void) { return snHead!=snTail; }
int hal_FPGA_DOMAPP_sn_event(SNEvent *evt) {
   int ret = 0;
   
   /* wait for an event... */
   while (!hal_FPGA_DOMAPP_sn_ready()) ;

   /* retrieve it... */
   while (1) {
      *evt = snEvents[snTail % lengthof(snEvents)];
      
      /* make sure we're still safe from overrun... */
      if (snHead - snTail > lengthof(snEvents) - 4) {
         snTail = snHead - lengthof(snEvents) + 16;
         ret = 1;
      }
      else {
         snTail++;
         break;
      }
   }
   
   return ret;
}

void hal_FPGA_DOMAPP_pedestal(int atwd, int channel, const short *pattern) {
   unsigned *addr = 
      (unsigned *) (DOM_FPGA_ATWD_PEDESTAL + 4*128*channel + 4*4*128*atwd);
   int i;
   for (i=0; i<128; i++) addr[i] = pattern[i]&0x3ff;
}

void hal_FPGA_DOMAPP_R2R_ladder(const unsigned char *pattern) {
   unsigned *addr = (unsigned *) DOM_FPGA_R2R_LADDER;
   int i;
   for (i=0; i<256; i++) addr[i] = pattern[i];
}

void hal_FPGA_DOMAPP_RG_set_zero_threshold(void) {
   unsigned *addr = (unsigned *) DOM_FPGA_COMP_CONTROL;
   *addr |= FPGABIT(COMP_CONTROL, SET_0_THRESH);
}

void hal_FPGA_DOMAPP_RG_clear_zero_threshold(void) {
   unsigned *addr = (unsigned *) DOM_FPGA_COMP_CONTROL;
   *addr &= ~FPGABIT(COMP_CONTROL, SET_0_THRESH);
}

void hal_FPGA_DOMAPP_RG_compress_last_only(void) {
   unsigned *addr = (unsigned *) DOM_FPGA_COMP_CONTROL;
   *addr |= FPGABIT(COMP_CONTROL, ONLY_LAST);
}

void hal_FPGA_DOMAPP_RG_compress_all(void) {
   unsigned *addr = (unsigned *) DOM_FPGA_COMP_CONTROL;
   *addr &= ~FPGABIT(COMP_CONTROL, ONLY_LAST);
}

void hal_FPGA_DOMAPP_RG_fadc_threshold(short thresh) {
   unsigned *addr = (unsigned *) DOM_FPGA_FADC_THRESHOLD;
   *addr = thresh&0x3ff;
}

void hal_FPGA_DOMAPP_RG_atwd_threshold(short chip, short ch, short thresh) {
   unsigned *addr = (unsigned *) DOM_FPGA_FADC_THRESHOLD;
   unsigned mask, val;
   
   chip &= 1;
   addr += chip*2;
   
   ch &= 3;
   addr += (ch/2);

   if ( (ch%2) == 0 ) {
      /* low bits... */
      mask = ~0x3ff;
      val = thresh;
   }
   else {
      /* high bits... */
      mask = ~(0x3ff<<16);
      val = thresh<<16;
   }

   *addr = (*addr & mask) | val;
}

#if 0
static int ticks;
static unsigned maxtick;
static unsigned mintick = 0xffffffff;

void irqHandler(void) {
   if (*((volatile unsigned *) 0x7fffcc10) == 18) {
      ticks++;
      *((volatile unsigned *) 0x7fffc200) = 0x1c;
      if (*(volatile unsigned *) 0x7fffc230 > maxtick) {
         maxtick = *(volatile unsigned *) 0x7fffc230;
      }
      if (*(volatile unsigned *) 0x7fffc230 < mintick) {
         mintick = *(volatile unsigned *) 0x7fffc230;
      }
   }
}

void ticker(void) {
   /* install irq handler... */
   extern void irqInstall( void (*handler)(void) );

   /* everything off for now... */
   *((volatile unsigned *) 0x7fffc200) = 0;

   /* period = (limit+1)/ahb2
    * -> limit+1 = period*ahb2
    * -> limit = period*ahb2 - 1
    *  where:
    *     period = 10ms (0.01s)
    *     ahb2 is AHB1/2
    */
   *((volatile unsigned *) 0x7fffc220) = (AHB1/2)/100 - 1;

   /* prescale is zero */
   *((volatile unsigned *) 0x7fffc210) = 0;

   /* assign irq priority... */
   *((volatile unsigned *) 0x7fffcca0) = 18;
   
   /* get ready for interrupts... */
   irqInstall(irqHandler);
   
   /* unmask t0 interrupts... */
   *((volatile unsigned *) 0x7fffcc00) = 0x100;
   
   /* go! */
   *((volatile unsigned *) 0x7fffc200) = 0x1c;

   while (1) {
      printf("ticks = %d [%u=%uns] [%u] %u\r\n", (volatile int) ticks, (volatile unsigned) maxtick, ((volatile unsigned)maxtick)*(1000000000/(AHB1/2)), mintick,
             (AHB1/2)/100 - 1);
      halUSleep(1000000);
   }
}
#endif

int hal_FPGA_DOMAPP_dump_regs(HALDOMAPPReg *regs, int n) {
   int i = 0;

#define STR(a) #a
#define STRING(a) STR(a)
#define DUMPREG(a) do { \
   if (i<n) { regs[i].name=STRING(a); regs[i].reg=FPGA(a); i++; } \
  } while (0)

   DUMPREG(TRIGGER_SOURCE);
   DUMPREG(TRIGGER_SETUP);
   DUMPREG(DAQ);
   DUMPREG(LBM_CONTROL);
   DUMPREG(LBM_POINTER);
   DUMPREG(DOM_STATUS);
   DUMPREG(SYSTIME_LSB);
   DUMPREG(SYSTIME_MSB);
   DUMPREG(LC_CONTROL);
   DUMPREG(LC_CABLE_LENGTH_UP);
   DUMPREG(LC_CABLE_LENGTH_DOWN);
   DUMPREG(CAL_CONTROL);
   DUMPREG(CAL_TIME);
   DUMPREG(CAL_LAUNCH);
   DUMPREG(CAL_LAST_FLASH_LSB);
   DUMPREG(CAL_LAST_FLASH_MSB);
   DUMPREG(RATE_CONTROL);
   DUMPREG(RATE_SPE);
   DUMPREG(RATE_MPE);
   DUMPREG(SN_CONTROL);
   DUMPREG(SN_DATA);
   DUMPREG(INT_EN);
   DUMPREG(INT_MASK);
   DUMPREG(INT_ACK);
   DUMPREG(FL_CONTROL);
   DUMPREG(FL_STATUS);
   DUMPREG(COMP_CONTROL);
   DUMPREG(FADC_THRESHOLD);
   DUMPREG(ATWD_A_01_THRESHOLD);
   DUMPREG(ATWD_A_23_THRESHOLD);
   DUMPREG(ATWD_B_01_THRESHOLD);
   DUMPREG(ATWD_B_23_THRESHOLD);
   DUMPREG(PONG);
   DUMPREG(FW_DEBUGGING);
   DUMPREG(R2R_LADDER);
   DUMPREG(ATWD_PEDESTAL);

   return i;
}

