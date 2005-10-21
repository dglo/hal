#include "hal/DOM_MB_domapp.h"
#include "DOM_FPGA_domapp_regs.h"

unsigned long long hal_FPGA_DOMAPP_get_local_clock(void) {
   unsigned long long h1 = FPGA(DOMAPP_SYSTIME_MSB);
   unsigned long long l1 = FPGA(DOMAPP_SYSTIME_LSB);
   unsigned long long h2 = FPGA(DOMAPP_SYSTIME_MSB);
   
   if (h1==h2) return (h1<<32)|l1;
   if (l1<0x80000000ULL) return (h2<<32)|l1;
   return (h1<<32)|l1;
}

