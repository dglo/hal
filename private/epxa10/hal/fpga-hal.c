/**
 * \file fpga-hal.c, the fpga dom hal.
 */
#include "hal/DOM_MB_fpga.h"
#include "hal/DOM_MB_pld.h"
#include "DOM_FPGA_regs.h"

BOOLEAN
hal_FPGA_TEST_atwd_readout_done(int chip) {
   return ((chip==0) ?
      RFPGABIT(TEST_LOCAL_STATUS, ATWD0): 
      RFPGABIT(TEST_LOCAL_STATUS, ATWD1)) != 0;
}

int
hal_FPGA_TEST_atwd_readout(short *buffer, int max, int chip, int channel) {
   unsigned volatile *data = (unsigned volatile *)
      ((chip==0) ? DOM_FPGA_TEST_ATWD0_DATA : DOM_FPGA_TEST_ATWD1_DATA);
   int i;

   /* wait for done bit...
    */
   while (!hal_FPGA_TEST_atwd_readout_done(chip)) ;
   
   /* readout data...
    */
   for (i=0; i<128 && i<max; i++) buffer[i] = data[128*channel + i];

   /* clear launch bit...
    */
   FPGA(TEST_LOCAL) = 0;

   return i;
}

void hal_FPGA_TEST_atwd_trigger_forced(int chip) {
   FPGA(TEST_LOCAL) = FPGABIT(TEST_LOCAL, ATWD0);
}

void hal_FPGA_TEST_atwd_trigger_disc(int chip) {
   FPGA(TEST_LOCAL) = FPGABIT(TEST_LOCAL, ATWD0_DISC);
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

int hal_FPGA_TEST_receive(int *type, int *len, char *msg) {
   int i;
   unsigned bytes[2];
   unsigned reg;
   
   /* wait for msg */
   while (RFPGABIT(TEST_COM_STATUS, RX_MSG_READY)==0) ;

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
