#ifndef DOM_MB_FB_INCLUDE
#define DOM_MB_FB_INCLUDE

/**
 * \file DOM_MB_fb.h
 *
 * $Revision: 1.10 $
 * $Author: jkelley $
 * $Date: 2005-05-02 20:16:58 $
 *
 * \b Usage:
 * \code
#include "hal/DOM_MB_fb.h"
 * \endcode
 *
 * DOM main board hardware access library interface for the flasher board
 *
 */
#include "hal/DOM_MB_types.h"

/**
 * Flasherboard clock frequency
 */
#define FB_HAL_TICKS_PER_SEC       20000000

/**
 * FPGA types supported
 */
typedef enum {
    /** Test FPGA, for STF and iceboot */
    DOM_FPGA_TEST,
    /** Domapp FPGA */
    DOM_FPGA_DOMAPP
} DOM_FPGA_TYPE;

/**
 * Routine that powers the flasher board and initializes it for
 * operation.
 *
 * \see hal_FB_disable
 *
 * \param config_t pointer to record CPLD configuration time in us
 * \param valid_t pointer to record clock validation time in us
 * \param reset_t pointer to record power-on reset time in us
 * \param fpga_type current FPGA running (DOM_FPGA_TYPE enumeration)
 *
 * \return 0 if success, nonzero on error
 */
int
hal_FB_enable(int *config_t, int *valid_t, int *reset_t, DOM_FPGA_TYPE fpga_type);

/**
 * Routine that powers the flasher board, but doesn't perform
 * any validation that it's operating.  
 *
 * \see hal_FB_disable
 *
 */
void
hal_FB_enable_min(void);

/**
 * Routine that disables and powers down the flasher board.
 *
 * \see hal_FB_enable
 *
 */
void
hal_FB_disable(void);

/**
 * Routine that indicates if the flasherboard is powered up
 * and initialized.
 *
 * \return enabled (1=yes)
 *
 * \see hal_FB_enable
 * \see hal_FB_disable
 */
int 
hal_FB_isEnabled(void);

/**
 * Routine that enables or disables the DC/DC converter on
 * the flasherboard.
 *
 * \param val 1=enable, 0=disable
 *
 * \see hal_FB_get_DCDCen
 */
void 
hal_FB_set_DCDCen(int val);

/**
 * Routine that gets the status of the DC/DC converter on
 * the flasherboard.
 *
 * \return enabled (1=yes)
 *
 * \see hal_FB_set_DCDCen
 */
int  
hal_FB_get_DCDCen(void);

/**
 * Routine that reads the unique serial number of the flasher board.
 *
 * \param id pointer to ID string pointer
 *
 * \return error (0=OK)
 *
 */
int
hal_FB_get_serial(char **id);

/**
 * Routine that reads the firmware version of the flasher
 * board CPLD.
 *
 * \return version
 *
 */
USHORT
hal_FB_get_fw_version(void);

/**
 * Routine that reads the hardware version of the flasher
 * board (layout artwork version).
 *
 * \return version
 *
 */
USHORT
hal_FB_get_hw_version(void);

/**
 * Routine that sets the flash pulse width for all LEDs
 * on the flasher board.
 *
 * \param value width (0-255)
 *
 * \see hal_FB_set_brightness
 * \see hal_FB_enable_LEDs
 *
 */
void
hal_FB_set_pulse_width(UBYTE value);

/**
 * Routine that sets the flash brightness for all LEDs
 * on the flasher board.
 *
 * \param value brightness (0-127)
 *
 * \see hal_FB_set_pulse_width
 * \see hal_FB_enable_LEDs
 *
 */
void
hal_FB_set_brightness(UBYTE value);

/**
 * Routine that enables or disables individual LEDs
 * on the flasherboard.  Each bit in the mask corresponds
 * to one of the 12 LEDs (1=enable, 0=disable).
 *
 * \param enables LED enables
 *
 * \see hal_FB_set_pulse_width
 * \see hal_FB_set_brightness
 *
 */
void
hal_FB_enable_LEDs(USHORT enables);

/**
 * Routine that enables the video mux and selects which 
 * LED's current is sent back to the mainboard (and the ATWDs).
 * Can also select the 3.3v driver pulse.  Also allows for
 * disabling the mux for power saving.
 *
 * \param value mux input from DOM_FB_MUX_INPUTS enum
 * 
 * \see DOM_FB_MUX_INPUTS
 */
void
hal_FB_select_mux_input(UBYTE value);

typedef enum {
    /** Disable the mux */
    DOM_FB_MUX_DISABLE,

    /** LED current mux inputs (1-12) */
    DOM_FB_MUX_LED_1,
    DOM_FB_MUX_LED_2,
    DOM_FB_MUX_LED_3,
    DOM_FB_MUX_LED_4,
    DOM_FB_MUX_LED_5,
    DOM_FB_MUX_LED_6,
    DOM_FB_MUX_LED_7,
    DOM_FB_MUX_LED_8,
    DOM_FB_MUX_LED_9,
    DOM_FB_MUX_LED_10,
    DOM_FB_MUX_LED_11,
    DOM_FB_MUX_LED_12,

    /** 3.3V LED driver pulse */
    DOM_FB_MUX_PULSE

} DOM_FB_MUX_INPUTS;

/**
 * Routine that reprograms the flasher board CPLD through
 * its JTAG port, using XSVF data.
 *
 * \param p pointer to XSVF data
 * \param nbytes length of XSVF data buffer, in bytes
 *
 * \return 0=success, 1=fail
 */
int 
hal_FB_xsvfExecute(int *p, int nbytes);

/**
 * Flasherboard HAL error codes
 */
#define FB_HAL_ERR_CONFIG_TIME     -1
#define FB_HAL_ERR_VALID_TIME      -2
#define FB_HAL_ERR_ID_NOT_PRESENT  -3
#define FB_HAL_ERR_ID_BAD_CRC      -4
#define FB_HAL_ERR_RESET_TIME      -5

#endif
