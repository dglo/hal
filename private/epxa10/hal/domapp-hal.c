#include <math.h>

#include "hal/DOM_MB_domapp.h"
#include "hal/DOM_FPGA_domapp_regs.h"
#include "booter/epxa.h"

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
   return FPGA(LBM_POINTER)&HAL_FPGA_DOMAPP_LBM_MASK; 
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

void hal_FPGA_DOMAPP_lc_up_cable(HAL_FPGA_DOMAPP_LC_CABLES c) {
   FPGA(LC_CONTROL) = 
      ( FPGA(LC_CONTROL) & ~FPGABIT(LC_CONTROL, UP_CABLE_LENGTH) ) | (c<<8);
}

void hal_FPGA_DOMAPP_lc_down_cable(HAL_FPGA_DOMAPP_LC_CABLES c) {
   FPGA(LC_CONTROL) = 
      ( FPGA(LC_CONTROL) & ~FPGABIT(LC_CONTROL, DOWN_CABLE_LENGTH) ) | (c<<9);
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
   rate = 26 + (int) (log(rateHz/(1e9/25)) / log(2));
   if (rate<0) rate = 0;
   if (rate>17) rate = 17;
   
   FPGA(CAL_CONTROL) =
      (FPGA(CAL_CONTROL) & ~FPGABIT(CAL_CONTROL, PULSER_RATE))|(rate<<24);
       
   return (1.0e9/(25.0 * (1<<26))) * (1<<rate);
}

void hal_FPGA_DOMAPP_cal_mode(HAL_FPGA_DOMAPP_CAL_MODES mode) {
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

void hal_FPGA_DOMAPP_cal_match_time(unsigned clk) { FPGA(CAL_TIME) = clk; }
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

void hal_FPGA_DOMAPP_rate_monitor_enable(int mask) {
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

unsigned hal_FPGA_DOMAPP_spe_rate(void) { return FPGA(RATE_SPE); }
unsigned hal_FPGA_DOMAPP_mpe_rate(void) { return FPGA(RATE_MPE); }

void hal_FPGA_DOMAPP_sn_mode(HAL_FPGA_DOMAPP_SN_MODES mode) {
   FPGA(SN_CONTROL) = 
      ( FPGA(SN_CONTROL) & ~FPGABIT(SN_CONTROL, ENABLE) ) | mode;
}

int hal_FPGA_DOMAPP_sn_gate_time(int time) {
   if (time<6554 || time>104858) return -1;
   time *= 1000; /* convert to ns */
   
   FPGA(SN_CONTROL) = 
      ( FPGA(SN_CONTROL) & ~FPGABIT(SN_CONTROL, GATE_TIME) ) | 
   ( ((time - 6553600)/6553600) << 8 );

   return 0;
}

int hal_FPGA_DOMAPP_sn_dead_time(int time) {
   if (time<6400 || time>512000) return -1;
   
   FPGA(SN_CONTROL) = 
      ( FPGA(SN_CONTROL) & ~FPGABIT(SN_CONTROL, DEAD_TIME) ) | 
   ( ((time - 6400)/6400) << 16 );

   return 0;
}

int hal_FPGA_DOMAPP_sn_ready(void) { return 0; }
unsigned hal_FPGA_DOMAPP_sn_event(void) {
   while (!hal_FPGA_DOMAPP_sn_event()) ;
   return 0;
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

static int ticks;

static unsigned maxtick;

void irqHandler(void) {
   if (*((volatile unsigned *) 0x7fffcc10) == 18) {
      ticks++;
      *((volatile unsigned *) 0x7fffc200) = 0x1c;
      if (*(volatile unsigned *) 0x7fffc230 > maxtick) {
         maxtick = *(volatile unsigned *) 0x7fffc230;
      }
   }
}

#if 0
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
      printf("ticks = %d [%u=%uns]\r\n", (volatile int) ticks, (volatile unsigned) maxtick, maxtick*(1000000000/(AHB1/2)));
      halUSleep(1000000);
   }
}
#endif








