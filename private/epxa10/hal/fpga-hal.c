/**
 * \file fpga-hal.c, the fpga dom hal.
 */
#include <stddef.h>
#include <stdio.h>

#include "hal/DOM_MB_fpga.h"
#include "hal/DOM_MB_pld.h"
#include "DOM_FPGA_regs.h"

#include "dom-fpga/fpga-versions.h"

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

int hal_FPGA_TEST_send(int type, int len, const char *msg) {
   int i;

   if (len>4096) return 1;
   
   /* wait for comm to become avail... */
   while (!RFPGABIT(TEST_COM_STATUS, AVAIL)) ;

   /* wait for Tx fifo almost full to be low */
   while (RFPGABIT(TEST_COM_STATUS, TX_FIFO_ALMOST_FULL)) ;
   
   /* send data */
   FPGA(TEST_COM_TX_DATA) = type&0xff; 
   FPGA(TEST_COM_TX_DATA) = (type>>8)&0xff;  
   FPGA(TEST_COM_TX_DATA) = len&0xff; 
   FPGA(TEST_COM_TX_DATA) = (len>>8)&0xff; 

   for (i=0; i<len; i++) FPGA(TEST_COM_TX_DATA) = msg[i];

   return 0;
}

int hal_FPGA_TEST_msg_ready(void) {
   return RFPGABIT(TEST_COM_STATUS, RX_MSG_READY);
}

int hal_FPGA_TEST_receive(int *type, int *len, char *msg) {
   int i;
   unsigned bytes[2];
   unsigned reg;

   /* wait for msg */
   while (!hal_FPGA_TEST_msg_ready()) ;

   /* read it */
   bytes[0] = FPGA(TEST_COM_RX_DATA)&0xff;
   bytes[1] = FPGA(TEST_COM_RX_DATA)&0xff;
   *type = (bytes[1]<<8) | bytes[0];

   bytes[0] = FPGA(TEST_COM_RX_DATA)&0xff;
   bytes[1] = FPGA(TEST_COM_RX_DATA)&0xff;
   *len = (bytes[1]<<8) | bytes[0];
   
   for (i=0; i<*len; i++) msg[i] = FPGA(TEST_COM_RX_DATA)&0xff;

   reg = FPGA(TEST_COM_CTRL);
   FPGA(TEST_COM_CTRL) = reg | FPGABIT(TEST_COM_CTRL, RX_DONE);
   FPGA(TEST_COM_CTRL) = reg & (~FPGABIT(TEST_COM_CTRL, RX_DONE));

   return 0;
}

static int chkcomp(int mask, unsigned comps, 
		   DOM_HAL_FPGA_COMPONENTS cmp,
		   int val) {
   const unsigned *rom = (const unsigned *) DOM_FPGA_VERSIONING;
   if (comps&cmp) {
      if (rom[val]!=expected_versions[rom[0]][val]) {
	 printf("val: %d, rom: %d, exp: %d\r\n", val, rom[val],
		expected_versions[rom[0]][val]);
	 mask|=cmp;
      }
   }
   return mask;
}

int
hal_FPGA_query_versions(DOM_HAL_FPGA_TYPES type, unsigned comps) { 
   const unsigned *rom = (const unsigned *) DOM_FPGA_VERSIONING;
   int mask = 0, i;

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
   else if (type==DOM_HAL_FPGA_DOMAPP) {
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
   const short *rom = (const short *) DOM_FPGA_VERSIONING;
   return rom[1] | rom[2]<<16;
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

void hal_FPGA_TEST_request_reboot(void) { 
   int i;
   unsigned reg = FPGA(TEST_COM_CTRL);

   /* wait for Tx to drain (up to 4096/1e5 ~ 50ms... */
   for (i=0; i<5000 && RFPGABIT(TEST_COM_STATUS, TX_READ_EMPTY)!=0; i++) {
      halUSleep(10);
   }

   FPGA(TEST_COM_CTRL) = reg | FPGABIT(TEST_COM_CTRL, REBOOT_REQUEST); 
}

int  hal_FPGA_TEST_is_reboot_granted(void) {
   return RFPGABIT(TEST_COM_STATUS, REBOOT_GRANTED)!=0;
}

int hal_FPGA_TEST_is_comm_avail(void) {
   return RFPGABIT(TEST_COM_STATUS, AVAIL)!=0;
}

void hal_FPGA_TEST_clear_trigger(void) {
   clearLaunch();
}
