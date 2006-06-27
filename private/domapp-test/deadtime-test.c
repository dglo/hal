/* deadtime-test.c, test scaler deadtime on domapp fpga...
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <ctype.h>

#include "hal/DOM_MB_fpga.h"
#include "hal/DOM_MB_pld.h"
#include "hal/DOM_MB_domapp.h"

static inline int send(const char *b, int n) { 
   return hal_FPGA_send(0, n, b);
}

int main(void) {
   /* halWriteDAC(DOM_HAL_DAC_PMT_FE_PEDESTAL, 2130); */
   halWriteDAC(DOM_HAL_DAC_SINGLE_SPE_THRESH, 600);
   halWriteDAC(DOM_HAL_DAC_INTERNAL_PULSER, 1000);
   hal_FPGA_DOMAPP_cal_mode(HAL_FPGA_DOMAPP_CAL_MODE_REPEAT);
   hal_FPGA_DOMAPP_cal_source(HAL_FPGA_DOMAPP_CAL_SOURCE_FE_PULSER);
   printf("deadtime: %d\n",
          hal_FPGA_DOMAPP_rate_monitor_deadtime(12900));

   printf("460=%08x, 480=%08x\n", *(unsigned *) 0x90000460, 
          *(unsigned *) 0x90000480);

   hal_FPGA_DOMAPP_rate_monitor_enable(HAL_FPGA_DOMAPP_RATE_MONITOR_SPE);
   printf("pulser rate: %f\n", hal_FPGA_DOMAPP_cal_pulser_rate(10e6));

   printf("460=%08x, 480=%08x\n", *(unsigned *) 0x90000460, 
          *(unsigned *) 0x90000480);

   while (1) {
      /* check for messages... */
      if (hal_FPGA_DOMAPP_spe_rate_ready()) {
         unsigned spe;
         const int ret = hal_FPGA_DOMAPP_spe_rate(&spe);
         printf("rate: %u, ret=%d\n", spe, ret);
      }
   }

   return 0;
}
