#ifndef DOM_MB_FB_INCLUDE
#define DOM_MB_FB_INCLUDE

/**
 * \file DOM_MB_fb.h
 *
 * $Revision: 1.4 $
 * $Author: jkelley $
 * $Date: 2004-06-02 19:39:58 $
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
 * Routine that powers the flasher board and initializes it for
 * operation.
 *
 * \see hal_FB_disable
 *
 */
void
hal_FB_enable(void);

/**
 * Routine that disables and powers down the flasher board.
 *
 * \see hal_FB_enable
 *
 */
void
hal_FB_disable(void);

/**
 * Routine that reads the unique serial number of the flasher board.
 *
 * \return serial number (as hex string)
 *
 */
const char *
hal_FB_get_serial(void);

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

#endif
