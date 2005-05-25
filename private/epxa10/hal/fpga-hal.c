/**
 * \file fpga-hal.c, the fpga dom hal.
 */
#include "hal/DOM_MB_fpga.h"
#include "DOM_FPGA_regs.h"

int
hal_FPGA_TEST_atwd_readout(short *buffer, int max, int chip, int channel) {
   unsigned volatile *data = (unsigned volatile *)
      ((chip==0) ? DOM_FPGA_TEST_ATWD0_DATA : DOM_FPGA_TEST_ATWD1_DATA);
   int i;

   /* wait for done bit...
    */
   if (chip==0) {
      while (RFPGABIT(TEST_LOCAL_STATUS, ATWD0)==0) ;
   }
   else {
      while (RFPGABIT(TEST_LOCAL_STATUS, ATWD1)==0) ;
   }
   
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





