/* real-app.c, test program to exercise the "real" (now
 * called domapp) fpga...
 *
 * comm protocol is described in README, all messages are
 * synchronous, i.e. for every send there is a reply...
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

#include "hal/DOM_MB_fpga.h"
#include "hal/DOM_MB_pld.h"
#include "hal/DOM_MB_domapp.h"

/* HACK, our newlib doesn't do %llx...
 */
static const char *fmtHex64(unsigned long long clk) {
   int i;
   static char msg[64/4+1];
   int ldnz = 0;
   int n = 0;
   /* doh!  newlib has no %llu support... */
   const char digit[] = "0123456789abcdef";
   
   for (i=60; i>0; i-=4) {
      const int v = (clk>>i)&0xf;
      if (v!=0) ldnz = 1;
      if (ldnz) { msg[n] = digit[v]; n++; }
   }
   msg[n] = digit[clk&0xf]; n++;
   msg[n] = 0;
   return msg;
}

static const char *timeString(void) { 
   return fmtHex64(hal_FPGA_DOMAPP_get_local_clock());
}

static inline int send(const char *b, int n) { 
   return hal_FPGA_send(0, n, b);
}

/* receive an ASCII message from surface...
 * '\n' terminated [\r is deleted]...
 */
static int receive(char *b) {
   int nr, type;
   hal_FPGA_receive(&type, &nr, b);
   return nr;
}

static void moniMsg(void) {
   char msg[4092];
   int len = snprintf(msg, sizeof(msg), "MONI time=%s", timeString());

#define ADDADC(a) \
   len += snprintf(msg + len, sizeof(msg) - len, \
                   " %s=%d", #a, halReadADC(DOM_HAL_##a))

   ADDADC(ADC_VOLTAGE_SUM);
   ADDADC(ADC_5V_POWER_SUPPLY);
   ADDADC(ADC_PRESSURE);
   ADDADC(ADC_5V_CURRENT);
   ADDADC(ADC_3_3V_CURRENT);
   ADDADC(ADC_2_5V_CURRENT);
   ADDADC(ADC_1_8V_CURRENT);
   ADDADC(ADC_MINUS_5V_CURRENT);
   ADDADC(ADC_DISC_ONESPE);
   ADDADC(ADC_1_8V_POWER_SUPPLY);
   ADDADC(ADC_2_5V_POWER_SUPPLY);
   ADDADC(ADC_3_3V_POWER_SUPPLY);
   ADDADC(ADC_DISC_MULTISPE);
   ADDADC(ADC_FADC_0_REF);
   ADDADC(ADC_SINGLELED_HV);
   ADDADC(ADC_ATWDA_TRIGGER_BIAS_CURRENT);
   ADDADC(ADC_ATWDA_RAMP_TOP_VOLTAGE);
   ADDADC(ADC_ATWDA_RAMP_BIAS_CURRENT);
   ADDADC(ADC_ANALOG_REF);
   ADDADC(ADC_ATWDB_TRIGGER_BIAS_CURRENT);
   ADDADC(ADC_ATWDB_RAMP_TOP_VOLTAGE);
   ADDADC(ADC_ATWDB_RAMP_BIAS_CURRENT);
   ADDADC(ADC_PEDESTAL);
   ADDADC(ADC_FE_TEST_PULSE_AMPL);

#undef ADDADC
   
   len += snprintf(msg + len, sizeof(msg) - len,
                   " baseHV=%d", halReadBaseADC());

   /* FIXME: use last read temp to avoid delays? */
   /* FIXME: format temperature? */
   len += snprintf(msg + len, sizeof(msg) - len,
                   " temperature=%d", halReadTemp());
   len += snprintf(msg + len, sizeof(msg) - len,
                   " spe=%d mpe=%d",
                   hal_FPGA_DOMAPP_spe_rate(),
                   hal_FPGA_DOMAPP_mpe_rate());
   
   msg[len] = '\n';
   send(msg, len+1);
}

static const char *parseNameVal(const char *b, 
                                char *nm, int nlen, char *val, int vlen,
                                char delim) {
   int i;

   /* skip whitespace... */
   while (*b && (*b==' ' || *b=='\t') ) b++;

   /* slurp up name... */
   memset(nm, 0, nlen);
   i=0;
   while (*b && *b!=delim && i<nlen-1) { nm[i]=*b; b++; i++; }
   while (*b && *b!=delim) { b++; }
   if (*b==delim) b++;
   
   /* slurp up value... */
   memset(val, 0, vlen);
   i=0;
   while (*b && *b!=' ' && *b!='\t' && i<vlen-1) { val[i]=*b; b++; i++; }
   while (*b && *b!=' ' && *b!='\t') b++;
   
   return b;
}

/* set trigger modes... */
static int setTrigger(const char *val) {
   int srcs = 0;
   
   while (*val) {
      const char *t = val;
      while (*t && *t!='|') t++;
      if (strncmp(val, "spe", t-val)==0) {
         srcs |= HAL_FPGA_DOMAPP_TRIGGER_SPE;
      }
      else if (strncmp(val, "mpe", t-val)==0) {
         srcs |= HAL_FPGA_DOMAPP_TRIGGER_MPE;
      }
      else if (strncmp(val, "forced", t-val)==0) {
         srcs |= HAL_FPGA_DOMAPP_TRIGGER_FORCED;
      }
      else if (strncmp(val, "fe_pulser", t-val)==0) {
         srcs |= HAL_FPGA_DOMAPP_TRIGGER_FE_PULSER;
      }
      else if (strncmp(val, "led", t-val)==0) {
         srcs |= HAL_FPGA_DOMAPP_TRIGGER_LED;
      }
      else if (strncmp(val, "flasher", t-val)==0) {
         srcs |= HAL_FPGA_DOMAPP_TRIGGER_FLASHER;
      }
      else if (strncmp(val, "fe_r2r", t-val)==0) {
         srcs |= HAL_FPGA_DOMAPP_TRIGGER_FE_R2R;
      }
      else if (strncmp(val, "atwd_r2r", t-val)==0) {
         srcs |= HAL_FPGA_DOMAPP_TRIGGER_ATWD_R2R;
      }
      else if (strncmp(val, "lc_up", t-val)==0) {
         srcs |= HAL_FPGA_DOMAPP_TRIGGER_LC_UP;
      }
      else if (strncmp(val, "lc_down", t-val)==0) {
         srcs |= HAL_FPGA_DOMAPP_TRIGGER_LC_DOWN;
      }
      else if (strncmp(val, "none", t-val)==0) {
         srcs = 0;
      }
      else {
         return 1;
      }
      val = (*t) ? t+1 : t;
   }
   
   hal_FPGA_DOMAPP_trigger_source(srcs);
   return 0;
}

static HAL_FPGA_DOMAPP_DAQ_MODES daq_mode =
   HAL_FPGA_DOMAPP_DAQ_MODE_ATWD_FADC;

static int setDAQMode(const char *val) {
   if (strcmp(val, "atwd_fadc")==0) {
      hal_FPGA_DOMAPP_daq_mode(HAL_FPGA_DOMAPP_DAQ_MODE_ATWD_FADC);
      daq_mode = HAL_FPGA_DOMAPP_DAQ_MODE_ATWD_FADC;
   }
   else if (strcmp(val, "fadc")==0) {
      hal_FPGA_DOMAPP_daq_mode(HAL_FPGA_DOMAPP_DAQ_MODE_FADC);
      daq_mode = HAL_FPGA_DOMAPP_DAQ_MODE_FADC;
   }
   else if (strcmp(val, "ts")==0) {
      hal_FPGA_DOMAPP_daq_mode(HAL_FPGA_DOMAPP_DAQ_MODE_TS);
      daq_mode = HAL_FPGA_DOMAPP_DAQ_MODE_TS;
   }
   else return 1;

   return 0;
}

static int setATWDEnable(const char *val) {
   int atwds = HAL_FPGA_DOMAPP_ATWD_NONE;
   
   while (*val) {
      const char *t = val;
      while (*t && *t!='|') t++;
      if (strncmp(val, "A", t-val)==0) {
         atwds |= HAL_FPGA_DOMAPP_ATWD_A;
      }
      else if (strncmp(val, "B", t-val)==0) {
         atwds |= HAL_FPGA_DOMAPP_ATWD_B;
      }
      else if (strncmp(val, "none", t-val)==0) {
         atwds = HAL_FPGA_DOMAPP_ATWD_NONE;
      }
      else return 1;
      val = (*t) ? t+1 : t;
   }
   hal_FPGA_DOMAPP_enable_atwds(atwds);
   return 0;
}

static int setATWDMode(const char *val) {
   if (strcmp(val, "normal")==0) {
      hal_FPGA_DOMAPP_atwd_mode(HAL_FPGA_DOMAPP_ATWD_MODE_NORMAL);
   }
   else if (strcmp(val, "testing")==0) {
      hal_FPGA_DOMAPP_atwd_mode(HAL_FPGA_DOMAPP_ATWD_MODE_TESTING);
   }
   else if (strcmp(val, "debugging")==0) {
      hal_FPGA_DOMAPP_atwd_mode(HAL_FPGA_DOMAPP_ATWD_MODE_DEBUGGING);
   }
   else return 1;

   return 0;
}

static int setLCMode(const char *val) {
   if (strcmp(val, "off")==0) {
      hal_FPGA_DOMAPP_lc_mode(HAL_FPGA_DOMAPP_LC_MODE_OFF);
   }
   else if (strcmp(val, "soft")==0) {
      hal_FPGA_DOMAPP_lc_mode(HAL_FPGA_DOMAPP_LC_MODE_SOFT);
   }
   else if (strcmp(val, "hard")==0) {
      hal_FPGA_DOMAPP_lc_mode(HAL_FPGA_DOMAPP_LC_MODE_HARD);
   }
   else if (strcmp(val, "flabby")==0) {
      hal_FPGA_DOMAPP_lc_mode(HAL_FPGA_DOMAPP_LC_MODE_FLABBY);
   }
   else return 1;
   
   return 0;
}

static int setLBMMode(const char *val) {
   if (strcmp(val, "wrap")==0) {
      hal_FPGA_DOMAPP_lbm_mode(HAL_FPGA_DOMAPP_LBM_MODE_WRAP);
   }
   else if (strcmp(val, "stop")==0) {
      hal_FPGA_DOMAPP_lbm_mode(HAL_FPGA_DOMAPP_LBM_MODE_STOP);
   }
   else return 1;
   
   return 0;
}

static HAL_FPGA_DOMAPP_COMPRESSION_MODES compression_mode =
   HAL_FPGA_DOMAPP_COMPRESSION_MODE_OFF;

static int setCompMode(const char *val) {
   if (strcmp(val, "off")==0) {
      hal_FPGA_DOMAPP_compression_mode(HAL_FPGA_DOMAPP_COMPRESSION_MODE_OFF);
      compression_mode = HAL_FPGA_DOMAPP_COMPRESSION_MODE_OFF;
   }
   else if (strcmp(val, "on")==0) {
      hal_FPGA_DOMAPP_compression_mode(HAL_FPGA_DOMAPP_COMPRESSION_MODE_ON);
      compression_mode = HAL_FPGA_DOMAPP_COMPRESSION_MODE_ON;
   }
   else if (strcmp(val, "both")==0) {
      hal_FPGA_DOMAPP_compression_mode(HAL_FPGA_DOMAPP_COMPRESSION_MODE_BOTH); 
      compression_mode = HAL_FPGA_DOMAPP_COMPRESSION_MODE_BOTH;
   }
   else return 1;

   return 0;
}

static int setLCEnable(const char *val) {
   int srcs = 0;
   
   while (*val) {
      const char *t = val;
      while (*t && *t!='|') t++;

      if (strncmp(val, "send_up", t-val)==0) {
         srcs |= HAL_FPGA_DOMAPP_LC_ENABLE_SEND_UP;
      }
      else if (strncmp(val, "send_down", t-val)==0) {
         srcs |= HAL_FPGA_DOMAPP_LC_ENABLE_SEND_DOWN;
      }
      else if (strncmp(val, "rcv_up", t-val)==0) {
         srcs |= HAL_FPGA_DOMAPP_LC_ENABLE_RCV_UP;
      }
      else if (strncmp(val, "rcv_down", t-val)==0) {
         srcs |= HAL_FPGA_DOMAPP_LC_ENABLE_RCV_DOWN;
      }
      else return 1;
      
      val = (*t) ? t+1 : t;
   }
   hal_FPGA_DOMAPP_lc_enable(srcs);
   return 0;
}

static int setLCSpan(const char *val) {
   if (strlen(val)!=1 || *val<'1' || *val>'8') return 1;
   hal_FPGA_DOMAPP_lc_span(*val - '0');
   return 0;
}

static int setLCUpCable(const char *val) {
   if (strcmp(val, "short")==0) {
      hal_FPGA_DOMAPP_lc_up_cable(HAL_FPGA_DOMAPP_LC_CABLE_SHORT);
   }
   else if (strcmp(val, "long")==0) {
      hal_FPGA_DOMAPP_lc_up_cable(HAL_FPGA_DOMAPP_LC_CABLE_LONG);
   }
   else return 1;

   return 0;
}

static int setLCDownCable(const char *val) {
   if (strcmp(val, "short")==0) {
      hal_FPGA_DOMAPP_lc_down_cable(HAL_FPGA_DOMAPP_LC_CABLE_SHORT);
   }
   else if (strcmp(val, "long")==0) {
      hal_FPGA_DOMAPP_lc_down_cable(HAL_FPGA_DOMAPP_LC_CABLE_LONG);
   }
   else return 1;

   return 0;
}

static int setLCWindow(const char *v) {
   char pre[64], post[64];
   parseNameVal(v, pre, sizeof(pre), post, sizeof(post), ',');
   return hal_FPGA_DOMAPP_lc_windows(atoi(pre), atoi(post));
}

static int setDAC(const char *v) {
   char name[64], val[32];
   int dval;
   
   parseNameVal(v, name, sizeof(name), val, sizeof(val), ',');
   dval = atoi(val);
   
   {
#define ADDDAC(a) { #a, DOM_HAL_DAC_##a }
      const struct {
         const char *nm;
         DOM_HAL_DAC_CHANNELS ch;
      } map[] = {
         ADDDAC(ATWD0_TRIGGER_BIAS),
         ADDDAC(ATWD0_RAMP_TOP),
         ADDDAC(ATWD0_RAMP_RATE),
         ADDDAC(ATWD_ANALOG_REF),
         ADDDAC(ATWD1_TRIGGER_BIAS),
         ADDDAC(ATWD1_RAMP_TOP),
         ADDDAC(ATWD1_RAMP_RATE),
         ADDDAC(PMT_FE_PEDESTAL),
         ADDDAC(MULTIPLE_SPE_THRESH),
         ADDDAC(SINGLE_SPE_THRESH),
         ADDDAC(FAST_ADC_REF),
         ADDDAC(INTERNAL_PULSER),
         ADDDAC(LED_BRIGHTNESS),
         ADDDAC(FE_AMP_LOWER_CLAMP),
         ADDDAC(FL_REF),
         ADDDAC(MUX_BIAS)
      };
      const int n = sizeof(map)/sizeof(map[0]);
      int i;
      
      for (i=0; i<n; i++) {
         if (strcmp(name, map[i].nm)==0) {
            halWriteDAC(map[i].ch, dval);
            return 0;
         }
      }
#undef ADDDAC
   }

   return 1;
}

static int setCalEnable(const char *val) {
   int srcs = 0;
   
   while (*val) {
      const char *t = val;
      while (*t && *t!='|') t++;
      if (strncmp(val, "forced", t-val)==0) {
         srcs |= HAL_FPGA_DOMAPP_CAL_SOURCE_FORCED;
      }
      else if (strncmp(val, "fe_pulser", t-val)==0) {
         srcs |= HAL_FPGA_DOMAPP_CAL_SOURCE_FE_PULSER;
      }
      else if (strncmp(val, "led", t-val)==0) {
         srcs |= HAL_FPGA_DOMAPP_CAL_SOURCE_LED;
      }
      else if (strncmp(val, "flasher", t-val)==0) {
         srcs |= HAL_FPGA_DOMAPP_CAL_SOURCE_FLASHER;
      }
      else if (strncmp(val, "fe_r2r", t-val)==0) {
         srcs |= HAL_FPGA_DOMAPP_CAL_SOURCE_FE_R2R;
      }
      else if (strncmp(val, "atwd_r2r", t-val)==0) {
         srcs |= HAL_FPGA_DOMAPP_CAL_SOURCE_ATWD_R2R;
      }
      else if (strncmp(val, "none", t-val)==0) {
         srcs = 0;
      }
      else return 1;
      val = (*t) ? t+1 : t;
   }

   hal_FPGA_DOMAPP_cal_source(srcs);
   return 0;
}

static int setCalMode(const char *val) {
   if (strcmp(val, "off")==0) {
      hal_FPGA_DOMAPP_cal_mode(HAL_FPGA_DOMAPP_CAL_MODE_OFF);
   }
   else if (strcmp(val, "repeat")==0) {
      hal_FPGA_DOMAPP_cal_mode(HAL_FPGA_DOMAPP_CAL_MODE_REPEAT);
   }
   else if (strcmp(val, "match")==0) {
      hal_FPGA_DOMAPP_cal_mode(HAL_FPGA_DOMAPP_CAL_MODE_MATCH);
   }
   else if (strcmp(val, "forced")==0) {
      hal_FPGA_DOMAPP_cal_mode(HAL_FPGA_DOMAPP_CAL_MODE_FORCED);
   }
   else return 1;
   
   return 0;
}

static int setCalATWDOffset(const char *val) {
   return hal_FPGA_DOMAPP_cal_atwd_offset(atoi(val));
}

static int setCalPulserRate(const char *val) {
   hal_FPGA_DOMAPP_cal_pulser_rate(atof(val));
   return 0;
}

static int setRateEnable(const char *val) {
   int srcs = 0;
   
   while (*val) {
      const char *t = val;
      while (*t && *t!='|') t++;

      if (strncmp(val, "spe", t-val)==0) {
         srcs |= HAL_FPGA_DOMAPP_RATE_MONITOR_SPE;
      }
      else if (strncmp(val, "mpe", t-val)==0) {
         srcs |= HAL_FPGA_DOMAPP_RATE_MONITOR_MPE;
      }
      else if (strncmp(val, "none", t-val)==0) {
         srcs = 0;
      }
      else return 1;

      val = (*t) ? t+1 : t;
   }
   hal_FPGA_DOMAPP_rate_monitor_enable(srcs);
       return 0;
}

static int setConf(const char *name, const char *val) {
   if (strcmp(name, "trigger")==0)               return setTrigger(val);
   else if (strcmp(name, "daq_mode")==0)         return setDAQMode(val);
   else if (strcmp(name, "atwd_enable")==0)      return setATWDEnable(val);
   else if (strcmp(name, "atwd_mode")==0)        return setATWDMode(val);
   else if (strcmp(name, "lc_mode")==0)          return setLCMode(val);
   else if (strcmp(name, "lbm_mode")==0)         return setLBMMode(val);
   else if (strcmp(name, "compression_mode")==0) return setCompMode(val);
   else if (strcmp(name, "lc_enable")==0)        return setLCEnable(val);
   else if (strcmp(name, "lc_span")==0)          return setLCSpan(val);
   else if (strcmp(name, "lc_up_cable")==0)      return setLCUpCable(val);
   else if (strcmp(name, "lc_down_cable")==0)    return setLCDownCable(val);
   else if (strcmp(name, "lc_window")==0)        return setLCWindow(val);
   else if (strcmp(name, "dac")==0)              return setDAC(val);
   else if (strcmp(name, "cal_enable")==0)       return setCalEnable(val);
   else if (strcmp(name, "cal_mode")==0)         return setCalMode(val);
   else if (strcmp(name, "cal_atwd_offset")==0)  return setCalATWDOffset(val);
   else if (strcmp(name, "cal_pulser_rate")==0)  return setCalPulserRate(val);
   else if (strcmp(name, "rate_enable")==0)      return setRateEnable(val);

   return 1;
}

static const char *fpgaType(DOM_HAL_FPGA_TYPES type) {
   if (type==DOM_HAL_FPGA_TYPE_STF_COM)         { return "stf"; }
   else if (type==DOM_HAL_FPGA_TYPE_DOMAPP)     { return "domapp"; }
   else if (type==DOM_HAL_FPGA_TYPE_CONFIGBOOT) { return "configboot"; }
   else if (type==DOM_HAL_FPGA_TYPE_ICEBOOT)    { return "iceboot"; }
   else if (type==DOM_HAL_FPGA_TYPE_STF_NOCOM)  { return "stf-no-comm"; }
   return "unknown";
}

static void versMsg(void) {
   char b[512];
   const DOM_HAL_FPGA_TYPES type = hal_FPGA_query_type();
#define QRY(a) hal_FPGA_query_component_version(DOM_HAL_FPGA_COMP_##a), \
  hal_FPGA_query_component_expected(type, DOM_HAL_FPGA_COMP_##a)

   int len = snprintf(b, sizeof(b), "VERS");
   len += snprintf(b + len, sizeof(b) - len, " sw-build=%d",
                   ICESOFT_BUILD);
   len += snprintf(b + len, sizeof(b) - len, " fpga-type=%s",
                   fpgaType(hal_FPGA_query_type()));
   len += snprintf(b + len, sizeof(b) - len, " fpga-build=%d",
                   hal_FPGA_query_build());
   len += snprintf(b + len, sizeof(b) - len, " fifo=%d[%d]", QRY(COM_FIFO));
   len += snprintf(b + len, sizeof(b) - len, " dp=%d[%d]", QRY(COM_DP));
   len += snprintf(b + len, sizeof(b) - len, " daq=%d[%d]", QRY(DAQ));
   len += snprintf(b + len, sizeof(b) - len, " pulsers=%d[%d]", QRY(PULSERS));
   len += snprintf(b + len, sizeof(b) - len, " rate=%d[%d]", 
                   QRY(DISCRIMINATOR_RATE));
   len += snprintf(b + len, sizeof(b) - len, " lc=%d[%d]",
                   QRY(LOCAL_COINC));
   len += snprintf(b + len, sizeof(b) - len, " flasher=%d[%d]",
                   QRY(FLASHER_BOARD));
   len += snprintf(b + len, sizeof(b) - len, " trigger=%d[%d]", QRY(TRIGGER));
   len += snprintf(b + len, sizeof(b) - len, " clock=%d[%d]", 
                   QRY(LOCAL_CLOCK));
   len += snprintf(b + len, sizeof(b) - len, " sn=%d[%d]", QRY(SUPERNOVA));
#undef QRY

   len += snprintf(b + len, sizeof(b) - len, " fpga-matches=%s",
                   hal_FPGA_query_versions(type,
                                           DOM_HAL_FPGA_COMP_ALL)?"no":"yes");
   b[len]='\n';
   send(b, len+1);
}

static void confMsg(const char *buf) {
   char msg[4092];
   int len = snprintf(msg, sizeof(msg), "CONF time=%s", timeString());

   /* parse name value pairs and output old/new values... */
   while (*buf) {
      char name[128], val[4096];

      /* parse name/value... */
      buf = parseNameVal(buf, name, sizeof(name), val, sizeof(val), '=');

      /* set the value... */
      if (setConf(name, val)==0) {
         len += snprintf(msg + len, sizeof(msg) - len, " %s=%s", name, val);
      }
      else {
         int n = snprintf(msg, sizeof(msg),
                          "EXCP unable to set '%s' to '%s'\n", name, val);
         send(msg, n);
         return;
      }
   }
   msg[len] = '\n';
   send(msg, len+1);
}

/* pulser message, can be:
 *
 * now
 * or,
 * tick tick tick...
 * (maybe glass sphere model?)
 */
static void pulseMsg(char *buf) {
   if (strncmp(buf+1, "now", 3)==0) {
      char msg[128];
      hal_FPGA_DOMAPP_cal_launch();
      int len = snprintf(msg, sizeof(msg), "PULS now time=%s\n", timeString());
      send(msg, len);
   }
   else {
      /* FIXME: parse clock ticks and queue events... */
   }
}

/* access lookback event at 32bit word index idx */
static inline unsigned *lbmEvent(unsigned idx) {
   return 
      hal_FPGA_DOMAPP_lbm_address() + (idx & HAL_FPGA_DOMAPP_LBM_ACCESS_MASK);
}

static inline unsigned nextEvent(unsigned idx) {
   return (idx + HAL_FPGA_DOMAPP_LBM_EVENT_LEN)&HAL_FPGA_DOMAPP_LBM_MASK;
}

/* congeal all outstanding data that fits in a message -- lbmp is updated */
static void dataMsg(unsigned *lbmp) {
   const int hdrsz = sizeof("DATA 1234\n") - 1;
   char buf[4092], hdr[11];
   const int maxpayload = sizeof(buf) - hdrsz;
   int payloadsize = 0;
   int dataidx = hdrsz;

   /* FIXME: compute max event size based on
    * compression_mode and daq_mode
    */
   while (hal_FPGA_DOMAPP_lbm_pointer()!=*lbmp) {
      /* FIXME: we need to decode header -- find length... */
      const int size = HAL_FPGA_DOMAPP_LBM_EVENT_SIZE;
      
      /* is there space... */
      if (payloadsize+size >= maxpayload) break;

      /* FIXME: this is an awful hack! */
      memcpy(buf + dataidx, lbmEvent(*lbmp), size);
      
      /* increment */
      *lbmp = nextEvent(*lbmp);
      payloadsize += size;
      dataidx += size;
   }

   /* fill hdr */
   snprintf(hdr, sizeof(hdr), "DATA %4hd\n", payloadsize);
   memcpy(buf, hdr, hdrsz);
   send(buf, hdrsz + payloadsize);
}

int main(void) {
   /* ticker(); */
   enum { CONF, RUNNING } state = CONF;
   unsigned lbmp = 0;
   
   /* get into a known state... */
   hal_FPGA_DOMAPP_disable_daq();
   halUSleep(1000); /* make sure atwd is done... */
   hal_FPGA_DOMAPP_lbm_reset();
   
   while (1) {
      char buf[4096];
      int nr;
      
      /* check for messages... */
      nr = receive(buf);

      /* remove trailing newline... */
      while (nr>0 && (buf[nr-1]=='\n' || buf[nr-1]=='\r')) nr--;
      buf[nr] = 0;
      
      if (memcmp(buf, "MONI", 4)==0) {
         moniMsg();
      }
      else if (memcmp(buf, "DATA", 4)==0) {
         /* process a data message... */
         if (state == RUNNING) {
            /* wait for a message... */
            while ( hal_FPGA_DOMAPP_lbm_pointer() == lbmp ) ;
         }
         dataMsg(&lbmp);
      }
      else if (memcmp(buf, "CONF", 4)==0) {
         confMsg(buf+4);
      }
      else if (memcmp(buf, "VERS", 4)==0) {
         versMsg();
      }
      else if (memcmp(buf, "REBT", 4)==0) {
         char msg[] = "REBT\n";
         send(msg, sizeof(msg)-1);
         halBoardReboot();
         /* never returns... */
      }
      else if (memcmp(buf, "ECHO", 4)==0) {
         char msg[] = "REBT\n";
         send(msg, sizeof(msg)-1);
         while (1) {
            const int nr = receive(buf);
            send(buf, nr);
         }
      }
      else if (memcmp(buf, "PULS", 4)==0) {
         pulseMsg(buf+4);
      }
      else if (memcmp(buf, "STRT", 4)==0) {
         if (state!=RUNNING) {
            hal_FPGA_DOMAPP_lbm_reset();
            lbmp = 0;
            /* clear all the lookback memory */
            memset(hal_FPGA_DOMAPP_lbm_address(),
                   0, (HAL_FPGA_DOMAPP_LBM_ACCESS_MASK+1)*4);
            hal_FPGA_DOMAPP_enable_daq();
            state=RUNNING;
         }
         send("STRT\n", sizeof("STRT\n")-1);
      }
      else if (memcmp(buf, "STOP", 4)==0) {
         if (state==RUNNING) {
            hal_FPGA_DOMAPP_disable_daq();
            halUSleep(1000); /* make sure atwd is done... */
            state=CONF;
         }
         send("STOP\n", sizeof("STOP\n")-1);
      }
      else if (memcmp(buf, "STAT", 4)==0) {
         snprintf(buf, sizeof(buf), 
                  "STAT lbm=%u lbmp=%u state=%d flash=%s\n", 
                  hal_FPGA_DOMAPP_lbm_pointer(),
                  lbmp, state,
                  fmtHex64(hal_FPGA_DOMAPP_cal_last_flash()));
         send(buf, strlen(buf));
      }
      else if (memcmp(buf, "DUMP", 4)==0) {
         snprintf(buf, sizeof(buf), 
                  "DUMP"
                  " trigger=%08x" 
                  " daq=%08x"
                  " lbm=%u"
                  " cal_source=%08x"
                  "\n",
                  *(unsigned *) 0x90000400,
                  *(unsigned *) 0x90000410,
                  *(unsigned *) 0x90000424,
                  *(unsigned *) 0x90000460
                  );
         
         send(buf, strlen(buf));
      }
      else {
         char msg[256];
         int n = snprintf(msg, sizeof(msg), 
                          "EXCP invalid header -- "
                          "unrecognized message\n");
         send(msg, n);
      }
   }

   return 0;
}
