#ifndef DOM_MB_FPGA_INCLUDE
#define DOM_MB_FPGA_INCLUDE

/**
 * \file DOM_MB_fpga.h
 *
 * $Revision: 1.22 $
 * $Author: arthur $
 * $Date: 2003-07-14 18:14:45 $
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
 * \see hal_FPGA_TEST_readout
 * \see HAL_FPGA_TEST_TRIGGER_ATWD0
 * \see HAL_FPGA_TEST_TRIGGER_ATWD1
 * \see HAL_FPGA_TEST_TRIGGER_FADC
 */
void
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
#define HAL_FPGA_TEST_TRIGGER_ATWD0 1
/** trigger atwd1 */
#define HAL_FPGA_TEST_TRIGGER_ATWD1 2
/** trigger flash adc */
#define HAL_FPGA_TEST_TRIGGER_FADC  4

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
   /** Config boot fpga */
   DOM_HAL_FPGA_TYPE_CONFIG,

   /** Iceboot fpga */
   DOM_HAL_FPGA_TYPE_ICEBOOT,

   /** Test fpga 1 */
   DOM_HAL_FPGA_TYPE_STF_NOCOM,

   /** Test fpga 2 */
   DOM_HAL_FPGA_TYPE_STF_COM,

   /** Application fpga */
   DOM_HAL_FPGA_DOMAPP
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

#endif











