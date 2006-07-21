#ifndef DOM_MB_DOMAPP_FPGA_INCLUDE
#define DOM_MB_DOMAPP_FPGA_INCLUDE

/**
 * \file DOM_MB_domapp.h
 *
 * $Revision: 1.1.1.15 $
 * $Author: arthur $
 * $Date: 2006-07-21 19:36:31 $
 *
 * \b Usage:
 * \code
#include "hal/DOM_MB_domapp.h"
* \endcode
*
* DOM main board hardware access library interface
*
*/
#include "hal/DOM_MB_types.h"

/**
 * get local clock readout...
 *
 * \returns the local clock readout
 */
unsigned long long
hal_FPGA_DOMAPP_get_local_clock(void);

/**
 * trigger sources
 *
 * \see hal_FPGA_DOMAPP_trigger_source
 */
typedef enum {
   /** spe trigger, takes precedence over mpe trigger */
   HAL_FPGA_DOMAPP_TRIGGER_SPE = 0x001,
   /** mpe trigger, spe trigger takes precedence */
   HAL_FPGA_DOMAPP_TRIGGER_MPE = 0x002,
   /** forced trigger */
   HAL_FPGA_DOMAPP_TRIGGER_FORCED = 0x004,
   /** frontend pulser */
   HAL_FPGA_DOMAPP_TRIGGER_FE_PULSER = 0x008,
   /** on-board LED */
   HAL_FPGA_DOMAPP_TRIGGER_LED = 0x010,
   /** flasher board */
   HAL_FPGA_DOMAPP_TRIGGER_FLASHER = 0x020,
   /** frontend R2R */
   HAL_FPGA_DOMAPP_TRIGGER_FE_R2R = 0x040,
   /** atwd R2R */
   HAL_FPGA_DOMAPP_TRIGGER_ATWD_R2R = 0x080,
   /** local coincidence up */
   HAL_FPGA_DOMAPP_TRIGGER_LC_UP = 0x100,
   /** local coincidence down */
   HAL_FPGA_DOMAPP_TRIGGER_LC_DOWN = 0x200
} HAL_FPGA_DOMAPP_TRIGGER_SOURCES;

/**
 * set trigger source
 * 
 * \param srcs bitwise OR of HAL_FPGA_DOMAPP_TRIGGER_SOURCES
 * \see HAL_FPGA_DOMAPP_TRIGGER_SOURCES
 */
void 
hal_FPGA_DOMAPP_trigger_source(int srcs);

/**
 * Enable daq data taking
 *
 * \see hal_FPGA_DOMAPP_enable_atwds
 * \see hal_FPGA_DOMAPP_daq_mode
 * \see hal_FPGA_DOMAPP_atwd_mode
 * \see hal_FPGA_DOMAPP_lc_mode
 * \see hal_FPGA_DOMAPP_lbm_mode
 * \see hal_FPGA_DOMAPP_compression_mode
 */
void
hal_FPGA_DOMAPP_enable_daq(void);

/**
 * Disable daq data taking
 *
 * \see hal_FPGA_DOMAPP_enable_atwds
 * \see hal_FPGA_DOMAPP_daq_mode
 * \see hal_FPGA_DOMAPP_atwd_mode
 * \see hal_FPGA_DOMAPP_lc_mode
 * \see hal_FPGA_DOMAPP_lbm_mode
 * \see hal_FPGA_DOMAPP_compression_mode
 */
void
hal_FPGA_DOMAPP_disable_daq(void);

/**
 * atwd bit definitions
 *
 * \see hal_FPGA_DOMAPP_enable_atwds
 */
typedef enum {
   HAL_FPGA_DOMAPP_ATWD_NONE = 0,
   HAL_FPGA_DOMAPP_ATWD_A    = 2,
   HAL_FPGA_DOMAPP_ATWD_B    = 4
} HAL_FPGA_DOMAPP_ATWDS;

/**
 * Enable atwds
 * 
 * \param mask bit mask of atwd to enable, from HAL_FPGA_DOMAPP_ATWDS
 */
void
hal_FPGA_DOMAPP_enable_atwds(int mask);

/**
 * daq modes
 *
 * \see hal_FPGA_DOMAPP_daq_mode
 */
typedef enum {
   /** atwd and fadc (at least one atwd _must_ be enabled) */
   HAL_FPGA_DOMAPP_DAQ_MODE_ATWD_FADC = (0<<8),
   /** fadc only */
   HAL_FPGA_DOMAPP_DAQ_MODE_FADC      = (1<<8),
   /** timestamp only */
   HAL_FPGA_DOMAPP_DAQ_MODE_TS        = (2<<8),
   /** not decided yet */
   HAL_FPGA_DOMAPP_DAQ_MODE_TBD       = (3<<8)
} HAL_FPGA_DOMAPP_DAQ_MODES;

/** 
 * set daq mode
 *
 * \param mode one of HAL_FPGA_DOMAPP_DAQ_MODES
 */
void 
hal_FPGA_DOMAPP_daq_mode(HAL_FPGA_DOMAPP_DAQ_MODES mode);

/**
 * atwd modes
 *
 * \see hal_FPGA_DOMAPP_atwd_mode
 */
typedef enum {
   /** start at channel 0, on overflow go to next channel */
   HAL_FPGA_DOMAPP_ATWD_MODE_NORMAL    = (0<<12),
   /** digitize all 4 channels */
   HAL_FPGA_DOMAPP_ATWD_MODE_TESTING   = (1<<12),
   /** for debugging */
   HAL_FPGA_DOMAPP_ATWD_MODE_DEBUGGING = (2<<12),
   /** undecided */
   HAL_FPGA_DOMAPP_ATWD_MODE_TBD       = (3<<12),
} HAL_FPGA_DOMAPP_ATWD_MODES;

/**
 * set atwd mode
 *
 * \param mode one of HAL_FPGA_DOMAPP_ATWD_MODES
 */
void hal_FPGA_DOMAPP_atwd_mode(HAL_FPGA_DOMAPP_ATWD_MODES mode);

/**
 * local coincidence modes
 *
 * \see hal_FPGA_DOMAPP_lc_mode
 */
typedef enum {
   /** off, no local coincidence */
   HAL_FPGA_DOMAPP_LC_MODE_OFF    = (0<<16),
   /** soft local coincidence */
   HAL_FPGA_DOMAPP_LC_MODE_SOFT   = (1<<16),
   /** hard local coincidence */
   HAL_FPGA_DOMAPP_LC_MODE_HARD   = (2<<16),
   /** flabby local coincidence */
   HAL_FPGA_DOMAPP_LC_MODE_FLABBY = (3<<16),
} HAL_FPGA_DOMAPP_LC_MODES;

/**
 * set lc mode
 *
 * \param mode one of hal_FPGA_DOMAPP_LC_MODES
 */
void
hal_FPGA_DOMAPP_lc_mode(HAL_FPGA_DOMAPP_LC_MODES mode);

/**
 * lookback memory buffer modes...
 *
 * \see hal_FPGA_DOMAPP_lbm_mode
 */
typedef enum {
   /** wrap endlessly -- and indiscriminantly */
   HAL_FPGA_DOMAPP_LBM_MODE_WRAP = (0<<20),
   /** stop when full */
   HAL_FPGA_DOMAPP_LBM_MODE_STOP = (1<<20),
   /** TBD */
   HAL_FPGA_DOMAPP_LBM_MODE_TBD1 = (2<<20),
   /** TBD */
   HAL_FPGA_DOMAPP_LBM_MODE_TBD2 = (3<<20),
} HAL_FPGA_DOMAPP_LBM_MODES;

/**
 * set lookback memory buffer mode
 *
 * \param mode one of HAL_FPGA_DOMAPP_LBM_MODES
 */
void
hal_FPGA_DOMAPP_lbm_mode(HAL_FPGA_DOMAPP_LBM_MODES mode);

/**
 * compression modes
 *
 * \see hal_FPGA_DOMAPP_compression_mode
 */
typedef enum {
   /** off, no compression */
   HAL_FPGA_DOMAPP_COMPRESSION_MODE_OFF  = (0<<24),
   /** on, compression only */
   HAL_FPGA_DOMAPP_COMPRESSION_MODE_ON   = (1<<24),
   /** both, compressed and raw waveforms */
   HAL_FPGA_DOMAPP_COMPRESSION_MODE_BOTH = (2<<24),
   /** TBD */
   HAL_FPGA_DOMAPP_COMPRESSION_MODE_TBD  = (3<<24),
} HAL_FPGA_DOMAPP_COMPRESSION_MODES;

/** 
 * set compression mode
 *
 * \param mode one of HAL_FPGA_DOMAPP_COMPRESSION_MODES
 */
void
hal_FPGA_DOMAPP_compression_mode(HAL_FPGA_DOMAPP_COMPRESSION_MODES mode);

/**
 * lookback memory pointer access mask -- we only use
 * the lower 24 bits (16M bytes) to access memory...
 */
#define HAL_FPGA_DOMAPP_LBM_ACCESS_MASK ((1<<24)-1)

/**
 * lookback memory event size in bytes, each event is this
 * size...
 */
#define HAL_FPGA_DOMAPP_LBM_EVENT_SIZE (1<<11)

/**
 * lookback memory event size in 32 bit words, each event is this
 * size...
 */
#define HAL_FPGA_DOMAPP_LBM_EVENT_LEN  (HAL_FPGA_DOMAPP_LBM_EVENT_SIZE>>2)

/**
 * LBM base address...
 *
 * to access lbm memory, use:
 *
 * hal_FPGA_DOMAPP_lbm_address()[idx & HAL_FPGA_DOMAPP_LBM_ACCESS_MASK]
 */
unsigned *
hal_FPGA_DOMAPP_lbm_address(void);

/**
 * reset lookback memory pointer...
 */
void
hal_FPGA_DOMAPP_lbm_reset(void);

/**
 * return lookback memory pointer, to access a word in
 * lookback memory
 */
unsigned
hal_FPGA_DOMAPP_lbm_pointer(void);

/**
 * local coincidence enables
 *
 * \see hal_FPGA_DOMAPP_lc_enable
 */
typedef enum {
   /** enable sending up */
   HAL_FPGA_DOMAPP_LC_ENABLE_SEND_UP   = 1,
   /** enable sending down */
   HAL_FPGA_DOMAPP_LC_ENABLE_SEND_DOWN = 2,
   /** enable rcv up */
   HAL_FPGA_DOMAPP_LC_ENABLE_RCV_UP    = 4,
   /** enable rcv down */
   HAL_FPGA_DOMAPP_LC_ENABLE_RCV_DOWN  = 8
} HAL_FPGA_DOMAPP_LC_ENABLES;

/**
 * enable local coincidence
 *
 * \param mask zero or more bits selected from HAL_FPGA_DOMAPP_LC_ENABLES
 */
void
hal_FPGA_DOMAPP_lc_enable(int mask);

/**
 * set lc span
 *
 * \param doms span 1..4 doms
 */
void
hal_FPGA_DOMAPP_lc_span(int doms);

/**
 * set cable length up correction
 *
 * \param dist neighbor distance (0..3)
 * \param ns correction in ns (precise to 25ns)
 * \returns actual ns of correction or -1 on error
 */
int
hal_FPGA_DOMAPP_lc_length_up(int dist, int ns);

/**
 * set cable length down correction
 *
 * \param dist neighbor distance (0..3)
 * \param ns correction in ns (precise to 25ns)
 * \returns actual ns of correction or -1 on error
 */
int
hal_FPGA_DOMAPP_lc_length_down(int dist, int ns);

/**
 * set lc discriminator source to spe
 */
void
hal_FPGA_DOMAPP_lc_disc_spe(void);

/**
 * set lc discriminator source to spe
 */
void
hal_FPGA_DOMAPP_lc_disc_mpe(void);

/**
 * set pre and post discriminator windows...
 *
 * \param pre pre-discriminator window in ns (25ns .. 1600ns)
 * \param post post-discriminator window in ns (25ns .. 1600ns)
 * \return 0 ok, non-zero, invalid window
 */
int
hal_FPGA_DOMAPP_lc_windows(int pre, int post);

/**
 * calibration modes
 *
 * \see hal_FPGA_DOMAPP_cal_mode
 */
typedef enum {
   /** no calibration sources */
   HAL_FPGA_DOMAPP_CAL_MODE_OFF    = (0<<12),
   /** repeating */
   HAL_FPGA_DOMAPP_CAL_MODE_REPEAT = (1<<12),
   /** match a time */
   HAL_FPGA_DOMAPP_CAL_MODE_MATCH  = (2<<12),
   /** cpu forced */
   HAL_FPGA_DOMAPP_CAL_MODE_FORCED = (3<<12),
} HAL_FPGA_DOMAPP_CAL_MODES;

/**
 * set calibration mode
 *
 * \param mode one of HAL_FPGA_DOMAPP_CAL_MODES
 */
void
hal_FPGA_DOMAPP_cal_mode(HAL_FPGA_DOMAPP_CAL_MODES mode);

/**
 * calibration sources
 *
 * \see hal_FPGA_DOMAPP_cal_source
 */
typedef enum {
   /** cpu forced */
   HAL_FPGA_DOMAPP_CAL_SOURCE_FORCED    = 0x01,
   /** frontend pulser */
   HAL_FPGA_DOMAPP_CAL_SOURCE_FE_PULSER = 0x02,
   /** onboard led */
   HAL_FPGA_DOMAPP_CAL_SOURCE_LED       = 0x04,
   /** flasher board */
   HAL_FPGA_DOMAPP_CAL_SOURCE_FLASHER   = 0x08,
   /** frontend r2r ladder */
   HAL_FPGA_DOMAPP_CAL_SOURCE_FE_R2R    = 0x10,
   /** atwd r2r ladder */
   HAL_FPGA_DOMAPP_CAL_SOURCE_ATWD_R2R  = 0x20
} HAL_FPGA_DOMAPP_CAL_SOURCES;

/**
 * set calibration sources
 *
 * \param mask zero or more HAL_FPGA_DOMAPP_CAL_SOURCES ored together
 */
void
hal_FPGA_DOMAPP_cal_source(int srcs);

/**
 * set calibration atwd offset
 *
 * \param offset offset in ns for forced ATWD launch from -200ns .. 175ns
 * \return 0 ok, non-zero invalid offset
 */
int
hal_FPGA_DOMAPP_cal_atwd_offset(int offset);

/**
 * set calibration pulser rate -- pick the closest rate not exceeding
 * that which is requested
 *
 * \param rate pulser rate in Hz (0.596Hz .. 78125Hz)
 * \return actual rate set
 */
double
hal_FPGA_DOMAPP_cal_pulser_rate(double rate);

/**
 * calibration match time -- set a clock time for which the
 *  calibration sources will fire
 *
 * \param clk low 32 bits of when calibration srcs should flash
 * next if HAL_FPGA_DOMAPP_CAL_MODE_MATCH is set in 
 * hal_FPGA_DOMAPP_cal_mode
 * \returns 0=ok, 1=queue full, value not accepted...
 */
int
hal_FPGA_DOMAPP_cal_match_time(unsigned clk);

/**
 * launch a calibration event
 */
void
hal_FPGA_DOMAPP_cal_launch(void);

/**
 * clock ticks of last calibration event fired...
 */
unsigned long long
hal_FPGA_DOMAPP_cal_last_flash(void);

/**
 * Routine that sets the flasher board rate.  Pick
 * rate closest to but not exceeding that which is
 * requested.  ~0.6Hz to 610 Hz are supported.
 * 
 * \param rate in Hz
 * \return actual rate set
 */
double
hal_FPGA_DOMAPP_FB_set_rate(double rateHz);

/**
 * Auxiliary reset control for the flasher board.
 * Sets the auxiliary reset bit -- used during the
 * FB CPLD acknowledge sequence.
 * 
 * \see hal_FPGA_DOMAPP_FB_clear_aux_reset
 * \see hal_FPGA_DOMAPP_FB_get_attn
 *
 */
void 
hal_FPGA_DOMAPP_FB_set_aux_reset(void);

/**
 * Auxiliary reset control for the flasher board.
 * Clears the auxiliary reset bit -- used during
 * FB CPLD acknowledge power-up sequence.
 * 
 * \see hal_FPGA_DOMAPP_FB_set_aux_reset
 * \see hal_FPGA_DOMAPP_FB_get_attn
 *
 */
void 
hal_FPGA_DOMAPP_FB_clear_aux_reset(void);

/**
 * Reads the flasher board ATTN bit.  Used during
 * FB CPLD acknowledge power-up sequence.
 * 
 * \see hal_FPGA_DOMAPP_FB_set_aux_reset
 * \see hal_FPGA_DOMAPP_FB_clear_aux_reset
 *
 * \returns value of ATTN bit (0/1)
 */
int 
hal_FPGA_DOMAPP_FB_get_attn(void);

/**
 * rate monitors
 *
 * \see hal_FPGA_DOMAPP_rate_monitor_enable
 */
typedef enum {
   HAL_FPGA_DOMAPP_RATE_MONITOR_OFF = 0,
   HAL_FPGA_DOMAPP_RATE_MONITOR_SPE = 1,
   HAL_FPGA_DOMAPP_RATE_MONITOR_MPE = 2
} HAL_FPGA_DOMAPP_RATE_MONITORS;

/**
 * enable rate monitors
 *
 * \param mask zero or more of HAL_FPGA_DOMAPP_RATE_MONITORS
 */
void
hal_FPGA_DOMAPP_rate_monitor_enable(int mask);

/**
 * set rate monitor artificial deadtime
 *
 * \param time 100ns .. 102400ns in 100ns increments...
 * \return 0 ok, non-zero invalid time
 */
int
hal_FPGA_DOMAPP_rate_monitor_deadtime(int time);

/**
 * is there an spe rate value ready to be read?
 */
int 
hal_FPGA_DOMAPP_spe_rate_ready(void);

/**
 * is there an mpe rate value ready?
 */
int
hal_FPGA_DOMAPP_mpe_rate_ready(void);

/**
 * put the oldest spe rate value in spe
 *
 * \param spe pointer to memory to be filled
 * \returns zero=OK, non-zero=Buffer overrun (spe is still filled)
 */
int hal_FPGA_DOMAPP_spe_rate(unsigned *spe);

/**
 * put the oldest mpe rate value in mpe
 *
 * \param mpe pointer to memory to be filled
 * \returns zero=OK, non-zero=Buffer overrun (mpe is still filled)
 */
int hal_FPGA_DOMAPP_mpe_rate(unsigned *mpe);

/**
 * get spe rate count -- OBSOLETE
 */
unsigned 
hal_FPGA_DOMAPP_spe_rate_immediate(void);

/**
 * get mpe rate -- OBSOLETE
 */
unsigned
hal_FPGA_DOMAPP_mpe_rate_immediate(void);

/**
 * supernova modes
 *
 * \see hal_FPGA_DOMAPP_sn_mode
 */
typedef enum {
   /** no supernova meter */
   HAL_FPGA_DOMAPP_SN_MODE_OFF = 0,
   /** use spe counts */
   HAL_FPGA_DOMAPP_SN_MODE_SPE = 1,
   /** use mpe counts */
   HAL_FPGA_DOMAPP_SN_MODE_MPE = 2,
   /** tbd */
   HAL_FPGA_DOMAPP_SN_MODE_TBD = 3,
} HAL_FPGA_DOMAPP_SN_MODES;

/**
 * set supernova mode
 */
void
hal_FPGA_DOMAPP_sn_mode(HAL_FPGA_DOMAPP_SN_MODES mode);

/**
 * set supernova dead time, this function should only be
 * called when xxx_sn_mode is XXX_SN_MODE_OFF...
 *
 * \param time dead time in ns, 6400ns .. 512000ns in 6400ns increments
 * \return 0 ok, non-zero, invalid dead time
 */
int
hal_FPGA_DOMAPP_sn_dead_time(int time);

/**
 * is there a sn event ready
 */
int
hal_FPGA_DOMAPP_sn_ready(void);

/**
 * sn event structure...
 */
typedef struct SNEventStruct {
   unsigned counts;
   unsigned long long ticks;
} SNEvent;

/**
 * get the latest sn event or wait until one is ready
 *
 * \returns 0 ok, <0 error (buffer overflow)...
 */
int
hal_FPGA_DOMAPP_sn_event(SNEvent *evt);

/**
 * set pedestal pattern
 *
 * \param atwd atwd to use (0=A, 1=B)
 * \param channel atwd channel (0..3)
 * \param pattern 128 values of the pedestal pattern
 */
void 
hal_FPGA_DOMAPP_pedestal(int atwd, int channel, const short *pattern);

/**
 * set R2R ladder pattern for both ATWD R2R and frontend R2R -- only
 * one can be used at a time.
 *
 * \param pattern 256 8 bit values
 */
void
hal_FPGA_DOMAPP_R2R_ladder(const unsigned char *pattern);

/**
 * roadgrader: set all thresholds to zero
 */
void
hal_FPGA_DOMAPP_RG_set_zero_threshold(void);

/**
 * roadgrader: clear all thresholds to zero
 */
void
hal_FPGA_DOMAPP_RG_clear_zero_threshold(void);

/**
 * roadgrader, compress only the last ATWD channel
 */
void
hal_FPGA_DOMAPP_RG_compress_last_only(void);

/**
 * roadgrader, compress all ATWD channels
 */
void
hal_FPGA_DOMAPP_RG_compress_all(void);

/**
 * roadgrader, set FADC threshold
 *
 * \param thresh threshold (0..1023)
 */
void
hal_FPGA_DOMAPP_RG_fadc_threshold(short thresh);

/**
 * roadgrader, set ATWD thresholds
 *
 * \param chip (0=A..1=B)
 * \param channel (0..3)
 * \param thresh threshold (0..1023)
 */
void
hal_FPGA_DOMAPP_RG_atwd_threshold(short chip, short channel, short thresh);

typedef struct HALDOMAPPRegStruct {
   const char *name;
   unsigned reg;
} HALDOMAPPReg;

/**
 * debugging only...
 */
int
hal_FPGA_DOMAPP_dump_regs(HALDOMAPPReg *regs, int n);

#endif
