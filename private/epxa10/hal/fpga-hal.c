/**
 * \file fpga-hal.c, the fpga dom hal.
 */
#include <stddef.h>

#include "hal/DOM_MB_fpga.h"
#include "hal/DOM_MB_pld.h"
#include "DOM_FPGA_regs.h"

#include "dom-fpga/fpga-versions.h"

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

void
hal_FPGA_TEST_readout(short *ch0, short *ch1, short *ch2, short *ch3,
		      short *ch4, short *ch5, short *ch6, short *ch7,
		      int max, 
		      short *fadc, int fadclen, 
		      int trigger_mask) {
   short *chs[] = { ch0, ch1, ch2, ch3, ch4, ch5, ch6, ch7 };
   int i, j;

   /* wait for done...
    */
   while (!hal_FPGA_TEST_readout_done(trigger_mask)) ;
   
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
	    ch[i] = data[j*128 + i];
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
   
   /* clear launch bit...
    */
   FPGA(TEST_SIGNAL) = 0;
}

void hal_FPGA_TEST_trigger_forced(int trigger_mask) {
   unsigned reg = 0;
   if (trigger_mask & HAL_FPGA_TEST_TRIGGER_ATWD0) 
      reg |= FPGABIT(TEST_SIGNAL, ATWD0);
   if (trigger_mask & HAL_FPGA_TEST_TRIGGER_ATWD1) 
      reg |= FPGABIT(TEST_SIGNAL, ATWD1);
   if (trigger_mask & HAL_FPGA_TEST_TRIGGER_FADC) 
      reg |= FPGABIT(TEST_SIGNAL, FADC);

   FPGA(TEST_SIGNAL) = reg;
}

void hal_FPGA_TEST_trigger_disc(int trigger_mask) {
   unsigned reg = 0;
   if (trigger_mask & HAL_FPGA_TEST_TRIGGER_ATWD0) 
      reg |= FPGABIT(TEST_SIGNAL, ATWD0_DISC);
   if (trigger_mask & HAL_FPGA_TEST_TRIGGER_ATWD1) 
      reg |= FPGABIT(TEST_SIGNAL, ATWD1_DISC);
   if (trigger_mask & HAL_FPGA_TEST_TRIGGER_FADC) 
      reg |= FPGABIT(TEST_SIGNAL, FADC_DISC);

   FPGA(TEST_SIGNAL) = reg;
}

int hal_FPGA_TEST_send(int type, int len, const char *msg) {
   int cnt = 0;
   int i;
   const int us = 1;

   if (len>4096) {
      printf("send: invalid message length!\r\n");
      return 1;
   }
   
   /* wait for Tx fifo almost full to be low */
   while (RFPGABIT(TEST_COM_STATUS, TX_FIFO_ALMOST_FULL)) {
      halUSleep(1);
      cnt++;
      if (cnt==1000000) {
	 printf("send: timeount!\r\n");
	 return 1;
      }
   }
   
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
   const short *rom = (const short *) DOM_FPGA_VERSIONING;
   if (comps&cmp) {
      if (rom[val]!=expected_versions[rom[0]][val])
	 mask|=cmp;
   }
   return mask;
}

int
hal_FPGA_query_versions(DOM_HAL_FPGA_TYPES type, unsigned comps) { 
   const unsigned *rom = (const unsigned *) DOM_FPGA_VERSIONING;
   int mask = 0, i;

   /* get type...
    */
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

   FPGA(TEST_AHB_MASTER_TEST) = bits;
}

int hal_FPGA_TEST_get_spe_rate(void) {
   return FPGA(TEST_SINGLE_SPE_RATE);
}

int hal_FPGA_TEST_get_mpe_rate(void) {
   return FPGA(TEST_MULTIPLE_SPE_RATE);
}

unsigned long long hal_FPGA_TEST_get_local_clock(void) {
   unsigned long long h1 = FPGA(TEST_LOCAL_CLOCK_HIGH);
   unsigned long long l1 = FPGA(TEST_LOCAL_CLOCK_LOW);
   unsigned long long h2 = FPGA(TEST_LOCAL_CLOCK_HIGH);
   
   if (h1==h2) return (h1<<32)|l1;

   if (l1<0x80000000ULL) return (h2<<32)|l1;
   
   return (h1<<32)|l1;
}

void hal_FPGA_TEST_enable_pulser(void) {
   unsigned reg = FPGA(TEST_SIGNAL);
   reg|=FPGABIT(TEST_SIGNAL, FE_PULSER);
   FPGA(TEST_SIGNAL) = reg;
}

void hal_FPGA_TEST_disable_pulser(void) {
   unsigned reg = FPGA(TEST_SIGNAL);
   reg&=~FPGABIT(TEST_SIGNAL, FE_PULSER);
   FPGA(TEST_SIGNAL) = reg;
}

void hal_FPGA_TEST_request_reboot(void) { 
   unsigned reg = FPGA(TEST_COM_CTRL);
   FPGA(TEST_COM_CTRL) = reg | FPGABIT(TEST_COM_CTRL, REBOOT_REQUEST); 
}

int  hal_FPGA_TEST_is_reboot_granted(void) {
   return RFPGABIT(TEST_COM_STATUS, REBOOT_GRANTED)!=0;
}




