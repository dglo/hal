/**
 * \file fpga-hal.c, the fpga dom hal.
 */
#include <stddef.h>
#include <stdio.h>
#include <math.h>
#include <string.h>

#include "hal/DOM_MB_fpga.h"
#include "hal/DOM_MB_pld.h"
#include "DOM_FPGA_regs.h"

#include "dom-fpga/fpga-versions.h"

#include "booter/epxa.h"

/* Global for ping-pong mode management */
static short g_atwd_ping_pong = 0;

BOOLEAN
hal_FPGA_TEST_readout_done(int trigger_mask) {
   int ret = trigger_mask;
   
   if (trigger_mask & HAL_FPGA_TEST_TRIGGER_ATWD0) {
      if (RFPGABIT(TEST_SIGNAL_RESPONSE, ATWD0)) {
	 ret &= (~HAL_FPGA_TEST_TRIGGER_ATWD0);
      }
   }
   
   if (trigger_mask & HAL_FPGA_TEST_TRIGGER_ATWD1) {
      if (RFPGABIT(TEST_SIGNAL_RESPONSE, ATWD1)) {
	 ret &= (~HAL_FPGA_TEST_TRIGGER_ATWD1);
      }
   }

   if (trigger_mask & HAL_FPGA_TEST_TRIGGER_FADC) {
      if (RFPGABIT(TEST_SIGNAL_RESPONSE, FADC_DONE)) {
	 ret &= (~HAL_FPGA_TEST_TRIGGER_FADC);
      }
   }

   return ret==0;
}

/* clear all launch bits...
 */
static unsigned clearLaunch() {
   return FPGA(TEST_SIGNAL) = FPGA(TEST_SIGNAL) & 
      ~(DOM_FPGA_TEST_SIGNAL_FADC|DOM_FPGA_TEST_SIGNAL_FADC_DISC|
	DOM_FPGA_TEST_SIGNAL_ATWD0|DOM_FPGA_TEST_SIGNAL_ATWD0_DISC|
	DOM_FPGA_TEST_SIGNAL_ATWD1|DOM_FPGA_TEST_SIGNAL_ATWD1_DISC|
    DOM_FPGA_TEST_SIGNAL_ATWD0_LED|DOM_FPGA_TEST_SIGNAL_ATWD1_LED|
	DOM_FPGA_TEST_SIGNAL_ATWD_PING_PONG);
}

int
hal_FPGA_TEST_readout(short *ch0, short *ch1, short *ch2, short *ch3,
		      short *ch4, short *ch5, short *ch6, short *ch7,
		      int max, 
		      short *fadc, int fadclen, 
		      int trigger_mask) {
   short *chs[] = { ch0, ch1, ch2, ch3, ch4, ch5, ch6, ch7 };
   int i, j;

   /* wait for done...
    */
   for (i=0; i<10000; i++) {
      if (hal_FPGA_TEST_readout_done(trigger_mask)) break;
      halUSleep(10);
   }
   
   if (!hal_FPGA_TEST_readout_done(trigger_mask)) return 1;
   
   /* don't readout buffers with no trigger...
    */
   if ((trigger_mask & HAL_FPGA_TEST_TRIGGER_ATWD0) == 0) {
      for (i=0; i<4; i++) chs[i] = NULL;
   }

   if ((trigger_mask & HAL_FPGA_TEST_TRIGGER_ATWD1) == 0) {
      for (i=4; i<8; i++) chs[i] = NULL;
   }

   if ((trigger_mask & HAL_FPGA_TEST_TRIGGER_FADC) == 0) {
      fadc = NULL;
   }

   /* readout atwd data...
    */
   for (j=0; j<8; j++) {
      short *ch = chs[j];
      unsigned volatile *data = (unsigned volatile *)
	 ((j<4) ? DOM_FPGA_TEST_ATWD0_DATA : DOM_FPGA_TEST_ATWD1_DATA);

      if (ch!=NULL) {
	 for (i=0; i<128 && i<max; i++) 
	    ch[i] = data[(j%4)*128 + i];
      }
   }
   
   /* readout data...
    */
   if (fadc!=NULL) {
      unsigned volatile *data = 
	 (unsigned volatile *) DOM_FPGA_TEST_FAST_ADC_DATA;
      for (i=0; i<512 && i<fadclen; i++) 
	 fadc[i] = data[i];
   }
   
   clearLaunch();
   
   return 0;
}

static unsigned waveformTriggers(int trigger_mask) {
   unsigned reg = 0;
   
   if (trigger_mask & HAL_FPGA_TEST_TRIGGER_FE_PULSER)
      reg |= FPGABIT(TEST_SIGNAL, FE_PULSER);
   if (trigger_mask & HAL_FPGA_TEST_TRIGGER_LED_PULSER)
      reg |= FPGABIT(TEST_SIGNAL, LED_PULSER);
   if (trigger_mask & HAL_FPGA_TEST_TRIGGER_R2R_TRI)
      reg |= FPGABIT(TEST_SIGNAL, R2R_TRIANGLE);
   if (trigger_mask & HAL_FPGA_TEST_TRIGGER_R2R_TRI_FE)
      reg |= FPGABIT(TEST_SIGNAL, R2R_TRIANGLE_FE);
   
   return reg;
}

void hal_FPGA_TEST_trigger_forced(int trigger_mask) {
   unsigned reg = 0;

   if (trigger_mask & HAL_FPGA_TEST_TRIGGER_ATWD0) 
      reg |= FPGABIT(TEST_SIGNAL, ATWD0);
   if (trigger_mask & HAL_FPGA_TEST_TRIGGER_ATWD1) 
      reg |= FPGABIT(TEST_SIGNAL, ATWD1);
   if (trigger_mask & HAL_FPGA_TEST_TRIGGER_FADC) 
      reg |= FPGABIT(TEST_SIGNAL, FADC);
   
   reg |= waveformTriggers(trigger_mask);

   FPGA(TEST_SIGNAL) = clearLaunch() | reg;
}

void hal_FPGA_TEST_trigger_disc(int trigger_mask) {
   unsigned reg = 0;
   if (trigger_mask & HAL_FPGA_TEST_TRIGGER_ATWD0) 
      reg |= FPGABIT(TEST_SIGNAL, ATWD0_DISC);
   if (trigger_mask & HAL_FPGA_TEST_TRIGGER_ATWD1) 
      reg |= FPGABIT(TEST_SIGNAL, ATWD1_DISC);
   if (trigger_mask & HAL_FPGA_TEST_TRIGGER_FADC) 
      reg |= FPGABIT(TEST_SIGNAL, FADC_DISC);

   reg |= waveformTriggers(trigger_mask);

   FPGA(TEST_SIGNAL) = clearLaunch() | reg;
}

void hal_FPGA_TEST_trigger_LED(int trigger_mask) {
   unsigned reg = 0;
   if (trigger_mask & HAL_FPGA_TEST_TRIGGER_ATWD0) 
      reg |= FPGABIT(TEST_SIGNAL, ATWD0_LED);
   if (trigger_mask & HAL_FPGA_TEST_TRIGGER_ATWD1) 
      reg |= FPGABIT(TEST_SIGNAL, ATWD1_LED);
   if (trigger_mask & HAL_FPGA_TEST_TRIGGER_FADC) 
      reg |= FPGABIT(TEST_SIGNAL, FADC_DISC);

   reg |= waveformTriggers(trigger_mask);

   FPGA(TEST_SIGNAL) = clearLaunch() | reg;
}

static int chkcomp(int mask, unsigned comps, 
		   DOM_HAL_FPGA_COMPONENTS cmp,
		   int val) {
   const unsigned *rom = (const unsigned *) DOM_FPGA_VERSIONING;
   if (comps&cmp) {
      if (rom[val]!=expected_versions[rom[0]][val]) {
#if 0
	 printf("val: %d, rom: %d, exp: %d\r\n", val, rom[val],
		expected_versions[rom[0]][val]);
#endif
	 mask|=cmp;
      }
   }
   return mask;
}

int
hal_FPGA_query_versions(DOM_HAL_FPGA_TYPES type, unsigned comps) { 
   const unsigned *rom = (const unsigned *) DOM_FPGA_VERSIONING;
   int mask = 0;

   /** Config boot fpga */
   if (type==DOM_HAL_FPGA_TYPE_CONFIG) {
      if (rom[0]!=FPGA_VERSIONS_TYPE_CONFIGBOOT) return -1;
   }
   else if (type==DOM_HAL_FPGA_TYPE_ICEBOOT) {
      if (rom[0]!=FPGA_VERSIONS_TYPE_ICEBOOT) return -1;
   }
   else if (type==DOM_HAL_FPGA_TYPE_STF_NOCOM) {
      if (rom[0]!=FPGA_VERSIONS_TYPE_STF_NOCOM) return -1;
   }
   else if (type==DOM_HAL_FPGA_TYPE_STF_COM) {
      if (rom[0]!=FPGA_VERSIONS_TYPE_STF_COM) return -1;
   }
   else if (type==DOM_HAL_FPGA_TYPE_DOMAPP) {
      if (rom[0]!=FPGA_VERSIONS_TYPE_DOMAPP) return -1;
   }
   else {
      return -1;
   }

   mask = chkcomp(mask, comps, 
		  DOM_HAL_FPGA_COMP_COM_FIFO, FPGA_VERSIONS_COM_FIFO);

   mask = chkcomp(mask, comps, 
		  DOM_HAL_FPGA_COMP_COM_DP, FPGA_VERSIONS_COM_DP);

   mask = chkcomp(mask, comps, 
		  DOM_HAL_FPGA_COMP_DAQ, FPGA_VERSIONS_DAQ);

   mask = chkcomp(mask, comps, 
		  DOM_HAL_FPGA_COMP_PULSERS, FPGA_VERSIONS_PULSERS);

   mask = chkcomp(mask, comps, 
		  DOM_HAL_FPGA_COMP_DISCRIMINATOR_RATE, 
		  FPGA_VERSIONS_DISCRIMINATOR_RATE);

   mask = chkcomp(mask, comps, 
		  DOM_HAL_FPGA_COMP_LOCAL_COINC, 
		  FPGA_VERSIONS_LOCAL_COINC);

   mask = chkcomp(mask, comps, 
		  DOM_HAL_FPGA_COMP_FLASHER_BOARD, 
		  FPGA_VERSIONS_FLASHER_BOARD);

   mask = chkcomp(mask, comps, 
		  DOM_HAL_FPGA_COMP_TRIGGER, FPGA_VERSIONS_TRIGGER);

   mask = chkcomp(mask, comps, 
		  DOM_HAL_FPGA_COMP_LOCAL_CLOCK, FPGA_VERSIONS_LOCAL_CLOCK);

   mask = chkcomp(mask, comps, 
		  DOM_HAL_FPGA_COMP_SUPERNOVA, FPGA_VERSIONS_SUPERNOVA);

   return mask;
}

int hal_FPGA_query_build(void) { 
   const unsigned *rom = (const unsigned *) DOM_FPGA_VERSIONING;
   return rom[1] | rom[2]<<16;
}

DOM_HAL_FPGA_TYPES hal_FPGA_query_type(void) { 
   const unsigned *rom = (const unsigned *) DOM_FPGA_VERSIONING;

   if (rom[0]==FPGA_VERSIONS_TYPE_STF_COM) {
      return DOM_HAL_FPGA_TYPE_STF_COM;
   }
   else if (rom[0]==FPGA_VERSIONS_TYPE_DOMAPP) {
      return DOM_HAL_FPGA_TYPE_DOMAPP;
   }
   else if (rom[0]==FPGA_VERSIONS_TYPE_CONFIGBOOT) {
      return DOM_HAL_FPGA_TYPE_CONFIG;
   }
   else if (rom[0]==FPGA_VERSIONS_TYPE_ICEBOOT) {
      return DOM_HAL_FPGA_TYPE_ICEBOOT;
   }
   else if (rom[0]==FPGA_VERSIONS_TYPE_STF_NOCOM) {
      return DOM_HAL_FPGA_TYPE_STF_NOCOM;
   }
   
   return DOM_HAL_FPGA_TYPE_INVALID;
}

void
hal_FPGA_TEST_set_pulser_rate(DOM_HAL_FPGA_PULSER_RATES rate) {
   unsigned bits = 0;
   
   if (rate==DOM_HAL_FPGA_PULSER_RATE_78k) bits = DOMPulserRate78k;
   else if (rate==DOM_HAL_FPGA_PULSER_RATE_39k) bits = DOMPulserRate39k;
   else if (rate==DOM_HAL_FPGA_PULSER_RATE_19_5k) bits = DOMPulserRate19_5k;
   else if (rate==DOM_HAL_FPGA_PULSER_RATE_9_7k) bits = DOMPulserRate9_7k;
   else if (rate==DOM_HAL_FPGA_PULSER_RATE_4_8k) bits = DOMPulserRate4_8k;
   else if (rate==DOM_HAL_FPGA_PULSER_RATE_2_4k) bits = DOMPulserRate2_4k;
   else if (rate==DOM_HAL_FPGA_PULSER_RATE_1_2k) bits = DOMPulserRate1_2k;
   else if (rate==DOM_HAL_FPGA_PULSER_RATE_0_6k) bits = DOMPulserRate_6k;

   FPGA(TEST_COMM) = 
      (FPGA(TEST_COMM) & ~0xf0000) | bits;
}

int hal_FPGA_TEST_get_spe_rate(void) {
   return FPGA(TEST_SINGLE_SPE_RATE_FPGA);
}

int hal_FPGA_TEST_get_mpe_rate(void) {
   return FPGA(TEST_MULTIPLE_SPE_RATE_FPGA);
}

unsigned long long hal_FPGA_TEST_get_local_clock(void) {
   unsigned long long h1 = FPGA(TEST_LOCAL_CLOCK_HIGH);
   unsigned long long l1 = FPGA(TEST_LOCAL_CLOCK_LOW);
   unsigned long long h2 = FPGA(TEST_LOCAL_CLOCK_HIGH);
   
   if (h1==h2) return (h1<<32)|l1;

   if (l1<0x80000000ULL) return (h2<<32)|l1;
   
   return (h1<<32)|l1;
}

unsigned long long hal_FPGA_TEST_get_atwd0_clock(void) {
   unsigned long long h1 = FPGA(TEST_ATWD0_TIMESTAMP_HIGH);
   unsigned long long l1 = FPGA(TEST_ATWD0_TIMESTAMP_LOW);
   
   return (h1<<32)|l1;
}

unsigned long long hal_FPGA_TEST_get_atwd1_clock(void) {
   unsigned long long h1 = FPGA(TEST_ATWD1_TIMESTAMP_HIGH);
   unsigned long long l1 = FPGA(TEST_ATWD1_TIMESTAMP_LOW);
   
   return (h1<<32)|l1;
}

void hal_FPGA_TEST_enable_ping_pong(void) {
   unsigned reg = 0;

#if 0
   // Override the lower 16 bits (enables) but keep the upper 16
   // (Dependent upon bit assignments in TEST_SIGNAL)
   reg  =  FPGA(TEST_SIGNAL) & 0xffff0000;
   reg |= FPGABIT(TEST_SIGNAL, ATWD_PING_PONG);
#endif
   FPGA(TEST_SIGNAL) = clearLaunch() | reg;

   /* always start with ATWD 0 */
   g_atwd_ping_pong = 0;
}

void hal_FPGA_TEST_readout_ping_pong(short *ch0, short *ch1, 
                                     short *ch2, short *ch3,
                                     int max, short ch_mask) {
    short *chs[] = { ch0, ch1, ch2, ch3};
    int i, j;
    unsigned trigger_mask;

    /* wait for done...
     */
    trigger_mask = (g_atwd_ping_pong == 0) ? HAL_FPGA_TEST_TRIGGER_ATWD0 :
        HAL_FPGA_TEST_TRIGGER_ATWD1;
    while (!hal_FPGA_TEST_readout_done(trigger_mask)) ;

    /* readout atwd data... 
     * but only the specified channels for speed
     */
    for (j=0; j<4; j++) {
        /* Check to see if channel is requested */
        if ((1 << j) & ch_mask) {
            short *ch = chs[j];
            unsigned volatile *data = (unsigned volatile *)
                ((g_atwd_ping_pong == 0) ? 
                 DOM_FPGA_TEST_ATWD0_DATA : 
                 DOM_FPGA_TEST_ATWD1_DATA);
        
            if (ch!=NULL) {
                for (i=0; i<128 && i<max; i++) 
                    ch[i] = data[j*128 + i];
            }
        }
    }
}

unsigned long long hal_FPGA_TEST_get_ping_pong_clock(void) {
    return (g_atwd_ping_pong == 0 ? hal_FPGA_TEST_get_atwd0_clock() : 
            hal_FPGA_TEST_get_atwd1_clock());
}

void hal_FPGA_TEST_readout_ping_pong_done(void) {
   unsigned reg = 0;

   reg |= FPGABIT(TEST_SIGNAL, ATWD_PING_PONG);
   if (g_atwd_ping_pong == 0) 
       reg |= FPGABIT(TEST_SIGNAL, ATWD0_READ_DONE);
   else
       reg |= FPGABIT(TEST_SIGNAL, ATWD1_READ_DONE);
   FPGA(TEST_SIGNAL) = reg;

   /* Flip which ATWD we're reading */
   g_atwd_ping_pong = g_atwd_ping_pong ^ 1;
}

void hal_FPGA_TEST_disable_ping_pong(void) {
    FPGA(TEST_SIGNAL) = clearLaunch();
}

void hal_FPGA_TEST_enable_pulser(void) {
   FPGA(TEST_SIGNAL) = FPGA(TEST_SIGNAL) | FPGABIT(TEST_SIGNAL, FE_PULSER);
}

void hal_FPGA_TEST_disable_pulser(void) {
   FPGA(TEST_SIGNAL) = FPGA(TEST_SIGNAL) & ~FPGABIT(TEST_SIGNAL, FE_PULSER);
}

void hal_FPGA_TEST_enable_LED(void) {
   FPGA(TEST_SIGNAL) = FPGA(TEST_SIGNAL) | FPGABIT(TEST_SIGNAL, LED_PULSER);
}

void hal_FPGA_TEST_disable_LED(void) {
   FPGA(TEST_SIGNAL) = FPGA(TEST_SIGNAL) & ~FPGABIT(TEST_SIGNAL, LED_PULSER);
}

void hal_FPGA_TEST_set_atwd_LED_delay(int delay) {
    /* Only low 4 bits used */
    FPGA(TEST_LED_ATWD_DELAY) = (delay & 0xf);
}

void hal_FPGA_TEST_start_FB_flashing(void) {
    FPGA(TEST_MISC) |= FPGABIT(TEST_MISC, FL_TRIGGER);
}

void hal_FPGA_TEST_stop_FB_flashing(void) {
    FPGA(TEST_MISC) &= ~FPGABIT(TEST_MISC, FL_TRIGGER);
}

void hal_FPGA_TEST_FB_JTAG_enable(void) {
    FPGA(TEST_MISC) |= FPGABIT(TEST_MISC, FL_EN_JTAG);
}

void hal_FPGA_TEST_FB_JTAG_disable(void) {
    FPGA(TEST_MISC) &= ~FPGABIT(TEST_MISC, FL_EN_JTAG);
}

void hal_FPGA_TEST_FB_JTAG_set_TCK(unsigned char val) {
    if (val & 0x1) {
        FPGA(TEST_MISC) |= FPGABIT(TEST_MISC, FL_TCK);
    }
    else {
        FPGA(TEST_MISC) &= ~FPGABIT(TEST_MISC, FL_TCK);
    }
}

void hal_FPGA_TEST_FB_JTAG_set_TMS(unsigned char val) {
    if (val & 0x1) {
        FPGA(TEST_MISC) |= FPGABIT(TEST_MISC, FL_TMS);
    }
    else {
        FPGA(TEST_MISC) &= ~FPGABIT(TEST_MISC, FL_TMS);
    }
}

void hal_FPGA_TEST_FB_JTAG_set_TDI(unsigned char val) {
    if (val & 0x1) {
        FPGA(TEST_MISC) |= FPGABIT(TEST_MISC, FL_TDI);
    }
    else {
        FPGA(TEST_MISC) &= ~FPGABIT(TEST_MISC, FL_TDI);
    }
}

unsigned char hal_FPGA_TEST_FB_JTAG_get_TDO(void) {
    if (FPGA(TEST_MISC_RESPONSE) & FPGABIT(TEST_MISC_RESPONSE, FL_TDO))
        return 0x1;
    else
        return 0x0;
}

void hal_FPGA_TEST_clear_trigger(void) {
   clearLaunch();
}

void hal_FPGA_TEST_init_state(void) {
   FPGA(TEST_SIGNAL) = 0;
   FPGA(TEST_COMM) = 0;
   FPGA(TEST_SINGLE_SPE_RATE) = 0;
   FPGA(TEST_MULTIPLE_SPE_RATE) = 0;
   FPGA(TEST_MISC) = 0;
   FPGA(TEST_HDV_CONTROL) = 0;
   FPGA(TEST_SINGLE_SPE_RATE_FPGA) = 0;
   FPGA(TEST_MULTIPLE_SPE_RATE_FPGA) = 0;
   FPGA(TEST_AHB_MASTER_TEST) = DOMPulserRate78k;
   /* FPGA(TEST_COM_CTRL) = ; better to not mess with this one */
   FPGA(TEST_LED_ATWD_DELAY) = 0;
}

void hal_FPGA_TEST_set_scalar_period(DOM_HAL_FPGA_SCALAR_PERIODS ms) {
   if (ms==DOM_HAL_FPGA_SCALAR_10MS) {
      FPGA(TEST_LED_ATWD_DELAY) = 
         FPGA(TEST_LED_ATWD_DELAY) | FPGABIT(TEST_LED_ATWD_DELAY, FAST_SCALAR);
   }
   else if (ms==DOM_HAL_FPGA_SCALAR_100MS) {
      FPGA(TEST_LED_ATWD_DELAY) = 
         FPGA(TEST_LED_ATWD_DELAY) & 
         ~FPGABIT(TEST_LED_ATWD_DELAY, FAST_SCALAR);
   }
}

void hal_FPGA_TEST_set_deadtime(int ns) {
   /*  50 -> 0 */
   /* 100 -> 1 */
   /* 200 -> 2 */
   /* 400 -> 3 */
   /* 800 -> 4 */
   /* 50 * (2^i) = ns => ns / 50 = (2^i) => log2(ns/50) = i */
   /* log2(a) = log(a)/log(2) */
   int i = (int) (log(ns/50.0)/log(2.0));
   if (i<0) i=0;
   if (i>15) i=15;
   FPGA(TEST_LED_ATWD_DELAY) = 
      (FPGA(TEST_LED_ATWD_DELAY) & (~0xf000)) | (i<<12);
}

void hal_FPGA_TEST_comm_bit_bang_dac(int v) {
   /* FIXME: only on stf no comm! */
   FPGA(TEST_COMM) = FPGABIT(TEST_COMM, DAC_BIT_BANG) | (v<<24);
}

int hal_FPGA_TEST_atwd0_has_lc(void) {
  return RFPGABIT(TEST_SIGNAL_RESPONSE, ATWD0_LC) ? 1 : 0;
}

int hal_FPGA_TEST_atwd1_has_lc(void) {
  return RFPGABIT(TEST_SIGNAL_RESPONSE, ATWD1_LC) ? 1 : 0;
}


#define WINDOW_BITS  6
#warning fixme TICKTIME_NS defined better somewhere else?
#define TICKTIME_NS  50
#define MAXWINDOW    ((1<<WINDOW_BITS)-1)
#define MAXWINDOW_NS (MAXWINDOW*TICKTIME_NS)

int hal_FPGA_TEST_set_lc_launch_window(int up_pre_ns,
				       int up_post_ns,
				       int down_pre_ns,
				       int down_post_ns) {
  if(   up_pre_ns    < 0
	|| up_post_ns   < 0
	|| down_pre_ns  < 0
	|| down_post_ns < 0
	|| up_pre_ns    > MAXWINDOW_NS
	|| up_post_ns   > MAXWINDOW_NS
	|| down_pre_ns  > MAXWINDOW_NS
	|| down_post_ns > MAXWINDOW_NS) return 1;
  
  /* Clear window bits */
  FPGA(TEST_LOCOIN_LAUNCH_WIN) &= ~(0x3F3F3F3F);
  /* Set window bits appropriately */
  FPGA(TEST_LOCOIN_LAUNCH_WIN)
    |= ((up_pre_ns/TICKTIME_NS)   &0x3F)
    |  ((up_post_ns/TICKTIME_NS)  &0x3F)<<8
    |  ((down_pre_ns/TICKTIME_NS) &0x3F)<<16
    |  ((down_post_ns/TICKTIME_NS)&0x3F)<<24;
  
  return 0;
}

int hal_FPGA_TEST_spe_lc_enabled(void) {
  return RFPGABIT(TEST_MISC, LOCAL_SPE) ? 1 : 0;
}

void hal_FPGA_TEST_enable_spe_lc(void) {
  FPGA(TEST_MISC) |= FPGABIT(TEST_MISC, LOCAL_SPE);
}

void hal_FPGA_TEST_disable_spe_lc(void) {
  FPGA(TEST_MISC) &= ~FPGABIT(TEST_MISC, LOCAL_SPE);
}
