#include "hal/DOM_MB_domapp.h"

/* access lookback event at byte index idx */
static inline unsigned *lbmEvent(unsigned idx) {
   return
      (unsigned *) 
      ( (char *) hal_FPGA_DOMAPP_lbm_address() + 
      (idx & HAL_FPGA_DOMAPP_LBM_ACCESS_MASK));
}

/* increment lbm pointer... */
static inline unsigned lbmNextEvent(unsigned idx) {
   return idx + HAL_FPGA_DOMAPP_LBM_EVENT_SIZE;
}

/* compute raw event length... */
static inline unsigned short lbmRawEventLength(unsigned idx) {
   return HAL_FPGA_DOMAPP_LBM_EVENT_SIZE;
}

/* compute compressed event length... */
static inline unsigned short lbmRoadGraderEventLength(unsigned idx) {
   return lbmEvent(idx)[0] & 0x7ff;
}

static inline unsigned short lbmEventFormat(unsigned idx) {
   return (lbmEvent(idx)[0]>>16);
}

static inline int lbmEventCompressed(unsigned idx) {
   return (lbmEvent(idx)[0]&0x8000000)!=0;
}

/* get timestamp from raw data... */
static inline unsigned long long lbmRawTimestamp(unsigned idx) {
   unsigned *lbm = lbmEvent(idx);
   return (((unsigned long long) lbm[1])<<32) | (lbm[0]&0xffff);
}

static inline unsigned short lbmRawTriggerSource(unsigned idx) {
   return lbmEvent(idx)[2]&0xffff;
}

static inline int lbmRawATWDSelected(unsigned idx) {
   return (lbmEvent(idx)[2]>>16)&0x1;
}

static inline int lbmRawATWDAvail(unsigned idx) {
   return (lbmEvent(idx)[2]>>18)&0x1;
}

static inline int lbmRawFADCAvail(unsigned idx) {
   return (lbmEvent(idx)[2]>>17)&0x1;
}
