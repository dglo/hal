#ifndef DOM_MB_FPGA_INCLUDE
#define DOM_MB_FPGA_INCLUDE

/**
 * \file DOM_MB_fpga.h
 *
 * $Revision: 1.36 $
 * $Author: arthur $
 * $Date: 2004-05-12 17:25:59 $
 *
 * \b Usage:
 * \code
#include "hal/DOM_MB_fpga.h"
 * \endcode
 *
 * DOM main board hardware access library interface
 *
 */
#include "hal/DOM_MB_types.h"

/**
 * fpga clock ticks per second
 */
#define FPGA_HAL_TICKS_PER_SEC 40000000

/**
 * check to see if triggered readout is done.
 *
 * \param trigger_mask the triggered channels to readout...
 *
 * \return non-zero if chip is done with readout
 * \see hal_FPGA_TEST_readout
 * \see HAL_FPGA_TEST_TRIGGER_ATWD0
 * \see HAL_FPGA_TEST_TRIGGER_ATWD1
 * \see HAL_FPGA_TEST_TRIGGER_FADC
 */
BOOLEAN
hal_FPGA_TEST_readout_done(int trigger_mask);

/**
 * readout the triggered channels, wait for a readout to be done first.
 *
 * \param ch0 chip 0 channel 0 buffer to be filled, may be NULL (not filled)
 * \param ch1 chip 0 channel 1 buffer to be filled, may be NULL (not filled)
 * \param ch2 chip 0 channel 2 buffer to be filled, may be NULL (not filled)
 * \param ch3 chip 0 channel 3 buffer to be filled, may be NULL (not filled)
 * \param ch4 chip 1 channel 0 buffer to be filled, may be NULL (not filled)
 * \param ch5 chip 1 channel 1 buffer to be filled, may be NULL (not filled)
 * \param ch6 chip 1 channel 2 buffer to be filled, may be NULL (not filled)
 * \param ch7 chip 1 channel 3 buffer to be filled, may be NULL (not filled)
 * \param max max number of atwd words to write (per channel)
 * \param fadc fadc buffer
 * \param nfadc number of fadc words to write
 * \param trigger_mask triggered channels to readout...
 *
 * \return zero ok, non-zero 100ms timeout...
 *
 * \see hal_FPGA_TEST_readout
 * \see HAL_FPGA_TEST_TRIGGER_ATWD0
 * \see HAL_FPGA_TEST_TRIGGER_ATWD1
 * \see HAL_FPGA_TEST_TRIGGER_FADC
 */
int
hal_FPGA_TEST_readout(short *ch0, short *ch1, short *ch2, short *ch3,
		      short *ch4, short *ch5, short *ch6, short *ch7,
		      int max, 
		      short *fadc, int nfadc, int trigger_mask);

/**
 * write triangle wave to communications DAC
 *
 */
void
hal_FPGA_TEST_comm_dac_write(void);

/**
 * read a buffer from the communication ADC
 *
 */
void
hal_FPGA_TEST_comm_adc_read(short *buffer, int max);

/**
 * write a triangle wave to the communications serial driver
 *
 */
void
hal_FPGA_TEST_comm_serial_write(void);

/**
 * read a buffer from the communications serial driver
 *
 * \return number of shorts read
 */
int
hal_FPGA_TEST_comm_serial_read(short *buffer, int max);

/** trigger atwd0 */
#define HAL_FPGA_TEST_TRIGGER_ATWD0      0x01
/** trigger atwd1 */
#define HAL_FPGA_TEST_TRIGGER_ATWD1      0x02
/** trigger flash adc */
#define HAL_FPGA_TEST_TRIGGER_FADC       0x04
/** trigger fe pulser */
#define HAL_FPGA_TEST_TRIGGER_FE_PULSER  0x08
/** trigger led pulser */
#define HAL_FPGA_TEST_TRIGGER_LED_PULSER 0x10
/** trigger r2r triangle */
#define HAL_FPGA_TEST_TRIGGER_R2R_TRI    0x20
/** trigger r2r triangle fe  */
#define HAL_FPGA_TEST_TRIGGER_R2R_TRI_FE 0x40

/**
 * forced launch of the atwd/fadc...
 *
 * \param trigger_mask bitmask of devices to trigger
 *
 * \see HAL_FPGA_TEST_TRIGGER_ATWD0
 * \see HAL_FPGA_TEST_TRIGGER_ATWD1
 * \see HAL_FPGA_TEST_TRIGGER_FADC
 */
void
hal_FPGA_TEST_trigger_forced(int trigger_mask);

/** 
 * discriminator launch of the atwd/fadc...
 *
 * \param trigger_mask bitmask of devices to trigger
 *
 * \see HAL_FPGA_TEST_TRIGGER_ATWD0
 * \see HAL_FPGA_TEST_TRIGGER_ATWD1
 * \see HAL_FPGA_TEST_TRIGGER_FADC
 */
void
hal_FPGA_TEST_trigger_disc(int trigger_mask);

/** 
 * LED launch of the atwd/fadc...
 *
 * \param trigger_mask bitmask of devices to trigger
 *
 * \see HAL_FPGA_TEST_TRIGGER_ATWD0
 * \see HAL_FPGA_TEST_TRIGGER_ATWD1
 * \see HAL_FPGA_TEST_TRIGGER_LED_PULSER
 * \see HAL_FPGA_TEST_TRIGGER_FADC
 */
void
hal_FPGA_TEST_trigger_LED(int trigger_mask);

/**
 * send a message over twisted pair communications channel
 *
 * \param type message type
 * \param len message length
 * \param msg is the message to send
 */
int
hal_FPGA_TEST_send(int type, int len, const char *msg);

/**
 * is there a message waiting to be read in the fifo?
 *
 * \return non-zero if message is waiting in fifo.
 */
int
hal_FPGA_TEST_msg_ready(void);

/**
 * receive a message over twister pair communications channel
 *
 * \param type message type
 * \param len message length
 * \param msg preallocated buffer at least 4096 bytes long.
 */
int
hal_FPGA_TEST_receive(int *type, int *len, char *msg);

/**
 * read DOM MB clock and return value
 * 
 * \return long long value of clock
 */
long long hal_FPGA_getClock();

/** 
 * fpga types.
 *
 * \see hal_FPGA_query_versions
 */
typedef enum {
   /** Invalid fpga type */
   DOM_HAL_FPGA_TYPE_INVALID,

   /** Config boot fpga */
   DOM_HAL_FPGA_TYPE_CONFIG,

   /** Iceboot fpga */
   DOM_HAL_FPGA_TYPE_ICEBOOT,

   /** Test fpga 1 */
   DOM_HAL_FPGA_TYPE_STF_NOCOM,

   /** Test fpga 2 */
   DOM_HAL_FPGA_TYPE_STF_COM,

   /** Application fpga */
   DOM_HAL_FPGA_TYPE_DOMAPP
} DOM_HAL_FPGA_TYPES;

/** 
 * fpga functional components.
 *
 * \see hal_FPGA_query_versions
 */
typedef enum {
   /** Communications */
   DOM_HAL_FPGA_COMP_COM_FIFO=0x001,
   DOM_HAL_FPGA_COMP_COM_DP=0x002,

   /** Data acquisition */
   DOM_HAL_FPGA_COMP_DAQ=0x004,

   /** Internal pulser */
   DOM_HAL_FPGA_COMP_PULSERS=0x008,

   /** discriminator rate */
   DOM_HAL_FPGA_COMP_DISCRIMINATOR_RATE=0x010,

   /** Local coincidence */
   DOM_HAL_FPGA_COMP_LOCAL_COINC=0x020,

   /** External flasher board */
   DOM_HAL_FPGA_COMP_FLASHER_BOARD=0x040,

   /** External flasher board */
   DOM_HAL_FPGA_COMP_TRIGGER=0x080,

   /** External flasher board */
   DOM_HAL_FPGA_COMP_LOCAL_CLOCK=0x100,

   /** External flasher board */
   DOM_HAL_FPGA_COMP_SUPERNOVA=0x200,

   /** placeholder, make sure new components go before this one... */
   DOM_HAL_FPGA_COMP_ALL = 0xffffffff,
} DOM_HAL_FPGA_COMPONENTS;

/**
 * check fpga version numbers.
 *
 * \param type fpga type (test, app, ...)
 * \param comps_mask Functional components used bitmask
 * \return 0 if ok, >0 functional components with the wrong version, <0
 *   incorrect fpga type.
 * \see DOM_HAL_FPGA_TYPES
 * \see DOM_HAL_FPGA_COMPONENTS
 */
int
hal_FPGA_query_versions(DOM_HAL_FPGA_TYPES type, unsigned comps_mask);

/**
 * get fpga build number.
 *
 * \return fpga build number, or -1 on error...
 */
int
hal_FPGA_query_build(void);

/**
 * get fpga type.
 *
 * \return fpga type
 */
DOM_HAL_FPGA_TYPES 
hal_FPGA_query_type(void);

/** 
 * fpga valid pulser rates...
 *
 * \see hal_FPGA_TEST_set_pulser_rate
 */
typedef enum {
   /**  78K Hz pulser rate */
   DOM_HAL_FPGA_PULSER_RATE_78k,
   /**  39K Hz pulser rate */
   DOM_HAL_FPGA_PULSER_RATE_39k,
   /**  19.5K Hz pulser rate */
   DOM_HAL_FPGA_PULSER_RATE_19_5k,
   /**  9.7K Hz pulser rate */
   DOM_HAL_FPGA_PULSER_RATE_9_7k,
   /**  4.8K Hz pulser rate */
   DOM_HAL_FPGA_PULSER_RATE_4_8k,
   /**  2.4K Hz pulser rate */
   DOM_HAL_FPGA_PULSER_RATE_2_4k,
   /**  1.2K Hz pulser rate */
   DOM_HAL_FPGA_PULSER_RATE_1_2k,
   /**  0.6K Hz pulser rate */
   DOM_HAL_FPGA_PULSER_RATE_0_6k,
} DOM_HAL_FPGA_PULSER_RATES;

/**
 * set pulser rate
 *
 * \param rate the pulser rate from DOM_HAL_FPGA_PULSER_RATES
 * \see DOM_HAL_FPGA_PULSER_RATES
 */void
hal_FPGA_TEST_set_pulser_rate(DOM_HAL_FPGA_PULSER_RATES rate);

/**
 * get spe rate
 *
 * \returns the single photon event rate
 */
int
hal_FPGA_TEST_get_spe_rate(void);

/**
 * get mpe rate
 *
 * \returns the current multiple photon event rate
 */
int
hal_FPGA_TEST_get_mpe_rate(void);

/**
 * get local clock readout...
 *
 * \returns the local clock readout
 */
unsigned long long
hal_FPGA_TEST_get_local_clock(void);

/**
 * get ATWD0 clock readout on start of ATWD0 launch...
 *
 * \returns the local clock readout
 */
unsigned long long
hal_FPGA_TEST_get_atwd0_clock(void);

/**
 * get ATWD1 clock readout on start of ATWD1 launch...
 *
 * \returns the atwd1 clock readout
 */
unsigned long long
hal_FPGA_TEST_get_atwd1_clock(void);

/**
 * Starts ATWD ping-pong acquisition, using discriminator trigger.
 *
 * \see hal_FPGA_TEST_disable_ping_pong
 * \see hal_FPGA_TEST_readout_ping_pong
 * \see hal_FPGA_TEST_get_ping_pong_clock
 * \see hal_FPGA_TEST_readout_ping_pong_done
 */
void
hal_FPGA_TEST_enable_ping_pong(void);

/**
 * Waits for an ATWD trigger and reads out data from
 * the appropriate ATWD in ping-pong mode.
 *
 * \param ch0 channel 0 buffer to be filled, may be NULL (not filled)
 * \param ch1 channel 1 buffer to be filled, may be NULL (not filled)
 * \param ch2 channel 2 buffer to be filled, may be NULL (not filled)
 * \param ch3 channel 3 buffer to be filled, may be NULL (not filled)
 * \param max max number of atwd words to write (per channel)
 * \param ch_mask 4-bit mask indicates which channels to read out
 *
 * \see hal_FPGA_TEST_enable_ping_pong
 * \see hal_FPGA_TEST_disable_ping_pong
 * \see hal_FPGA_TEST_get_ping_pong_clock
 * \see hal_FPGA_TEST_readout_ping_pong_done
 * 
 */
void
hal_FPGA_TEST_readout_ping_pong(short *ch0, short *ch1, short *ch2, short *ch3, int max, short ch_mask);

/**
 * Gets appropriate ping-pong ATWD clock readout.
 *
 * \returns the ping-pong atwd clock readout
 *
 * \see hal_FPGA_TEST_enable_ping_pong
 * \see hal_FPGA_TEST_disable_ping_pong
 * \see hal_FPGA_TEST_readout_ping_pong
 * \see hal_FPGA_TEST_readout_ping_pong_done
 */
unsigned long long
hal_FPGA_TEST_get_ping_pong_clock(void);

/**
 * Indicates to FPGA that both the ATWD buffer and the ATWD clock 
 * have been read and that ATWD may be re-enabled for ping-ponging.
 *
 * \see hal_FPGA_TEST_enable_ping_pong
 * \see hal_FPGA_TEST_disable_ping_pong
 * \see hal_FPGA_TEST_readout_ping_pong
 * \see hal_FPGA_TEST_get_ping_pong_clock
 */
void
hal_FPGA_TEST_readout_ping_pong_done(void);


/**
 * Disables ping-pong mode.
 *
 * \see hal_FPGA_TEST_enable_ping_pong
 * \see hal_FPGA_TEST_readout_ping_pong
 * \see hal_FPGA_TEST_get_ping_pong_clock
 * \see hal_FPGA_TEST_readout_ping_pong_done
 */
void
hal_FPGA_TEST_disable_ping_pong(void);


/** 
 * enable front end pulser
 *
 * \see hal_FPGA_TEST_set_pulser_rate
 * \see hal_FPGA_TEST_disable_pulser
 */
void 
hal_FPGA_TEST_enable_pulser(void);

/** 
 * disable front end pulser
 *
 * \see hal_FPGA_TEST_set_pulser_rate
 * \see hal_FPGA_TEST_enable_pulser
 */
void 
hal_FPGA_TEST_disable_pulser(void);

/** 
 * Enable on-board LED pulser
 *
 * \see hal_FPGA_TEST_disable_LED
 * \see hal_FPGA_TEST_trigger_LED
 * \see hal_FPGA_TEST_set_atwd_LED_delay
 */
void 
hal_FPGA_TEST_enable_LED(void);

/** 
 * Disable on-board LED pulser
 *
 * \see hal_FPGA_TEST_enable_LED
 * \see hal_FPGA_TEST_trigger_LED
 * \see hal_FPGA_TEST_set_atwd_LED_delay
 */
void 
hal_FPGA_TEST_disable_LED(void);

/**
 * Set the ATWD launch delay from the LED pulse
 *
 * \param delay ATWD launch from LED pulse = (2+delay)*25ns
 *
 * \see hal_FPGA_TEST_enable_LED
 * \see hal_FPGA_TEST_disable_LED
 * \see hal_FPGA_TEST_start_FB_flashing
 * \see hal_FPGA_TEST_stop_FB_flashing
 * \see hal_FPGA_TEST_trigger_LED
 */
void 
hal_FPGA_TEST_set_atwd_LED_delay(int delay);

/**
 * Routine that starts the flasher board flashing.
 * 
 * \see hal_FPGA_TEST_stop_FB_flashing
 * \see hal_FPGA_TEST_trigger_LED
 * \see hal_FPGA_TEST_set_atwd_LED_delay
 *
 */
void
hal_FPGA_TEST_start_FB_flashing(void);

/**
 * Routine that stops the flasher board flashing.
 * 
 * \see hal_FPGA_TEST_start_FB_flashing
 *
 */
void
hal_FPGA_TEST_stop_FB_flashing(void);

/**
 * Routine that enables FB JTAG port control.  Must
 * also enable on flasherboard side via PLD.
 *
 * \see hal_FPGA_TEST_FB_JTAG_disable
 * \see hal_FPGA_TEST_FB_JTAG_set_TCK
 * \see hal_FPGA_TEST_FB_JTAG_set_TMS
 * \see hal_FPGA_TEST_FB_JTAG_set_TDI
 * \see hal_FPGA_TEST_FB_JTAG_get_TOD
 *
 */
void
hal_FPGA_TEST_FB_JTAG_enable(void);

/**
 * Routine that disables FB JTAG port control.  Must
 * also disable on flasherboard side via PLD.
 *
 * \see hal_FPGA_TEST_FB_JTAG_enable
 * \see hal_FPGA_TEST_FB_JTAG_set_TCK
 * \see hal_FPGA_TEST_FB_JTAG_set_TMS
 * \see hal_FPGA_TEST_FB_JTAG_set_TDI
 * \see hal_FPGA_TEST_FB_JTAG_get_TDO
 *
 */
void
hal_FPGA_TEST_FB_JTAG_disable(void);


/**
 * Routine that sets the flasherboard JTAG
 * TCK port.
 *
 * \see hal_FPGA_TEST_FB_JTAG_enable
 * \see hal_FPGA_TEST_FB_JTAG_disable
 * \see hal_FPGA_TEST_FB_JTAG_set_TMS
 * \see hal_FPGA_TEST_FB_JTAG_set_TDI
 * \see hal_FPGA_TEST_FB_JTAG_get_TDO
 * 
 * \param val value to write (0/1)
 *
 */
void
hal_FPGA_TEST_FB_JTAG_set_TCK(unsigned char val);

/**
 * Routine that sets the flasherboard JTAG
 * TMS port.
 *
 * \see hal_FPGA_TEST_FB_JTAG_enable
 * \see hal_FPGA_TEST_FB_JTAG_disable
 * \see hal_FPGA_TEST_FB_JTAG_set_TCK
 * \see hal_FPGA_TEST_FB_JTAG_set_TDI
 * \see hal_FPGA_TEST_FB_JTAG_get_TDO
 * 
 * \param val value to write (0/1)
 *
 */
void
hal_FPGA_TEST_FB_JTAG_set_TMS(unsigned char val);

/**
 * Routine that sets the flasherboard JTAG
 * TDI port.
 *
 * \see hal_FPGA_TEST_FB_JTAG_enable
 * \see hal_FPGA_TEST_FB_JTAG_disable
 * \see hal_FPGA_TEST_FB_JTAG_set_TCK
 * \see hal_FPGA_TEST_FB_JTAG_set_TMS
 * \see hal_FPGA_TEST_FB_JTAG_get_TDO
 * 
 * \param val value to write (0/1)
 *
 */
void
hal_FPGA_TEST_FB_JTAG_set_TDI(unsigned char val);

/**
 * Routine that reads the flasherboard JTAG
 * TDO port.
 *
 * \see hal_FPGA_TEST_FB_JTAG_enable
 * \see hal_FPGA_TEST_FB_JTAG_disable
 * \see hal_FPGA_TEST_FB_JTAG_set_TCK
 * \see hal_FPGA_TEST_FB_JTAG_set_TMS
 * \see hal_FPGA_TEST_FB_JTAG_set_TDI
 *
 * \returns value of TDO (0/1)
 *
 */
unsigned char
hal_FPGA_TEST_FB_JTAG_get_TDO(void);

/**
 * request reboot
 *
 * \see hal_FPGA_TEST_is_reboot_granted
 */
void
hal_FPGA_TEST_request_reboot(void);

/**
 * request reboot
 *
 * \returns non-zero if reboot request was granted
 * \see hal_FPGA_TEST_request_reboot
 */
int
hal_FPGA_TEST_is_reboot_granted(void);

/**
 * is communications available?
 *
 * \returns non-zero if communications is running
 */
int
hal_FPGA_TEST_is_comm_avail(void);

/**
 * clear any previous atwd/pulser/fadc trigger...
 */
void 
hal_FPGA_TEST_clear_trigger(void);

/**
 * initialize the fpga, can be used to put the fpga in a known state.
 * yes, i understand there are known knowns, known unknowns and 
 * unknown unknowns.  this is probably a known unknown (if i had
 * to commit).  This is not required to get the fpga to work, it is
 * just a convenience function to try to get it in a known state.
 */
void
hal_FPGA_TEST_init_state(void);

/** 
 * fpga scalar period types.
 *
 * \see hal_FPGA_TEST_set_scalar_period
 */
typedef enum {
   /** 10ms scalar sample period */
   DOM_HAL_FPGA_SCALAR_10MS,

   /** 100ms scalar sample period */
   DOM_HAL_FPGA_SCALAR_100MS
} DOM_HAL_FPGA_SCALAR_PERIODS;

/**
 * set the scalar period
 *
 * \see DOM_HAL_FPGA_SCALAR_PERIODS
 */
void hal_FPGA_TEST_set_scalar_period(DOM_HAL_FPGA_SCALAR_PERIODS );

/**
 * set the atwd deadtime launch delay
 *
 * \param ns nanoseconds of delay (50, 100, 200, 400, ...)
 *
 * \see DOM_HAL_FPGA_SCALAR_PERIODS
 */
void hal_FPGA_TEST_set_deadtime(int ns);

#endif











