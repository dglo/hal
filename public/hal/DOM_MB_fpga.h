#ifndef DOM_MB_FPGA_INCLUDE
#define DOM_MB_FPGA_INCLUDE

/**
 * \file DOM_MB_fpga.h
 *
 * $Revision: 1.8 $
 * $Author: mcp $
 * $Date: 2003-05-15 00:05:11 $
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
 * readout the atwd, wait for a readout to be done.
 *
 * \param chip chip 0 or 1
 *
 * \return non-zero if chip is done with readout
 * \see hal_FPGA_TEST_atwd_readout_done
 */
BOOLEAN
hal_FPGA_TEST_atwd_readout_done(int chip);

/**
 * readout the atwd, wait for a readout to be done.
 *
 * \param buffer buffer to be filled
 * \param max max number of words to write
 * \param chip chip 0 or 1
 * \param channel channel 0-3
 *
 * \return number of shorts read
 * \see hal_FPGA_TEST_atwd_readout_done
 */
int
hal_FPGA_TEST_atwd_readout(short *buffer, int max, int chip, int channel);

/**
 * readout the flash adc
 *
 * \return number of shorts read
 *
 */
int
hal_FPGA_TEST_flashadc_readout(short *buffer, int max);

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

/**
 * forced launch of the atwd...
 */
void
hal_FPGA_TEST_atwd_trigger_forced(int chip);

/** 
 * discriminator launch of the atwd...
 */
void
hal_FPGA_TEST_atwd_trigger_disc(int chip);

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
#endif




