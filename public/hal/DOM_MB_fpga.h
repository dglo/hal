#ifndef DOM_MB_FPGA_INCLUDE
#define DOM_MB_FPGA_INCLUDE

/**
 * \file DOM_MB_fpga.h
 *
 * $Revision: 1.1 $
 * $Author: arthur $
 * $Date: 2003-01-24 21:29:44 $
 *
 * \b Usage:
 * \code
#include "hal/DOM_MB_fpga.h"
 * \endcode
 *
 * DOM main board hardware access library interface
 *
 */
#include "DOM_MB_types.h"

/**
 * readout the atwd
 *
 * \return number of shorts read
 *
 */
int
FPGA_TEST_atwd_readout(short *buffer, int max);

/**
 * readout the flash adc
 *
 * \return number of shorts read
 *
 */
int
FPGA_TEST_flashadc_readout(short *buffer, int max);

/**
 * write triangle wave to communications DAC
 *
 */
void
FPGA_TEST_comm_dac_write(void);

/**
 * read a buffer from the communication ADC
 *
 */
void
FPGA_TEST_comm_adc_read(short *buffer, int max);

/**
 * write a triangle wave to the communications serial driver
 *
 */
void
FPGA_TEST_comm_serial_write(void);


/**
 * read a buffer from the communications serial driver
 *
 * \return number of shorts read
 */
int
FPGA_TEST_comm_serial_read(short *buffer, int max);

#endif





