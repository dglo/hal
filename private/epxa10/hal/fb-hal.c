/**
 * \file fb-hal.c
 *
 * $Revision: 1.3 $
 * $Author: jkelley $
 * $Date: 2004-04-09 20:05:42 $
 *
 * The DOM flasher board HAL.
 *
 */
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "booter/epxa.h"

#include "hal/DOM_MB_fb.h"
#include "hal/DOM_MB_pld.h"
#include "DOM_FB_regs.h"

void hal_FB_enable(void) {

    /* Enable the flasherboard interface in the mainboard CPLD */
    halEnableFlasher();
    halUSleep(5000);

    /* Reset the flasherboard CPLD */
    FB(RESET) = 0x1;
    halUSleep(5000);

    /* Make sure all LEDs are off */
    hal_FB_enable_LEDs(0);

    /* Power on the DC-DC converter */
    FB(DCDC_CTRL) = 0x1;   
    halUSleep(5000);

    /* Initialize mux selects (turn off mux as default) */
    hal_FB_select_mux_input(DOM_FB_MUX_DISABLE);

    /* Turn off all LEDs */
    hal_FB_enable_LEDs(0);

    /* Initialize pulse width to zero */
    hal_FB_set_pulse_width(0);

    /* Enable delay chip */
    FB(LE_DP) = 0x1;
}

void hal_FB_disable(void) {

    /* Make sure all LEDs are off */
    hal_FB_enable_LEDs(0);

    /* Power off the DC-DC converter */
    FB(DCDC_CTRL) = 0x1;   
    halUSleep(5000);

    /* Disable the flasherboard interface in the mainboard CPLD */
    halDisableFlasher();
    halUSleep(5000);

}

static inline void waitOneWireBusy(void) { while (RFBBIT(ONE_WIRE, BUSY)) ; }

/*
 * Routine to read the unique serial number of the flasher board.
 */
const char * hal_FB_get_serial(void) {
    int i;
    const char *hexdigit = "0123456789abcdef";
    const int sz = 64/4+1;    
    static BOOLEAN read = FALSE;
    static char t[17];
    
    /* Only read out ID on first call */
    if (read == FALSE) {
        memset(t, 0, sz);

        /* Make sure we start in normal mode */
        /* There is funniness with state machine right after reset */
        FB(MODE_SELECT) = 0x0;
        halUSleep(5000);
    
        /* Put the CPLD in one-wire mode */
        FB(MODE_SELECT) = 0x2;
        halUSleep(5000);

        /* Reset one-wire */
        FB(ONE_WIRE) = 0xf;
        waitOneWireBusy();
    
        for (i=0; i<8; i++) {
            FB(ONE_WIRE) = ( (0x33>>i) & 1 ) ? 0x9 : 0xa;
            waitOneWireBusy();
        }
    
        /* LSB comes out first, unlike HV id */
        for (i=63; i>=0; i--) {
            FB(ONE_WIRE) = 0xb;
            waitOneWireBusy();
            t[i/4]>>=1;
            if (RFBBIT(ONE_WIRE, DATA)) {
                t[i/4] |= 0x8;
            }
        }
        
        for (i=0; i<64/4; i++) t[i] = hexdigit[(int)t[i]];
        
        /* Put the CPLD back in normal mode */
        FB(MODE_SELECT) = 0x0;
        halUSleep(5000);

        read = TRUE;
    }
    return t;
}

/*
 * Routine to get the CPLD firmware version. 
 */
USHORT hal_FB_get_fw_version(void) {
    return FB(CPLD_VERSION);
}

/*
 * Routine to get the flasher board layout version. 
 */
USHORT hal_FB_get_hw_version(void) {
    return FB(VERSION);
}

/**
 * Routine that sets the flash pulse width for all LEDs
 * on the flasher board.
 */
void hal_FB_set_pulse_width(UBYTE value) {
    FB(DELAY_ADJUST) = value;
}

/* 
 * SPI write: send a up to 32 bit value to the serial port.
 * Flasherboard chip can handle up to 10MHz (~500kHz used).
 */
static void write_FB_SPI(int val, int bits) {
    int mask;

    /* Enable chip select (active low) */
    FB(SPI_CTRL) &= ~FBBIT(SPI_CTRL, CSN);
    halUSleep(1);
    
    for (mask=(1<<(bits-1)); mask>0; mask>>=1) {
        const int dr = (val&mask) ? FBBIT(SPI_CTRL, MOSI) : 0;
        
        /* set data bit, clock low...
         */
        FB(SPI_CTRL) = dr;
        halUSleep(1);
        
        /* clock it in...
         */
        FB(SPI_CTRL) = dr | FBBIT(SPI_CTRL, SCLK);
        halUSleep(1);
    }
    
    /* Disable chip select */
    FB(SPI_CTRL) = FBBIT(SPI_CTRL, CSN);
}

/*
 * Write to the MAXIM 5348
 */
static void max5438Write(int val) {           
    /* send 7 bit serial value:
     *
     * data[6..0] (MSB first)
     */
    write_FB_SPI((val&0x7f), 7);
}

/**
 * Routine that sets the flash brightness for all LEDs
 * on the flasher board.
 */
void hal_FB_set_brightness(UBYTE value) {

    /* Make sure CPLD is in normal mode */
    /* This is a workaround for mode-switching weirdness in CPLD */
    FB(MODE_SELECT) = 0x0;

    /* Put the CPLD in SPI mode */
    FB(MODE_SELECT) = 0x1;

    /* Write the value */
    max5438Write(value);

    /* Put the CPLD back in normal mode */
    FB(MODE_SELECT) = 0x0;
    
}

/**
 * Routine that enables or disables individual LEDs
 * on the flasherboard.  Each bit in the mask corresponds
 * to one of the 12 LEDs (1=enable, 0=disable).
 */
void hal_FB_enable_LEDs(USHORT enables) {
    FB(LED_EN_4_1)  = enables & 0xf;
    FB(LED_EN_8_5)  = (enables >> 4) & 0xf;
    FB(LED_EN_12_9) = (enables >> 8) & 0xf;
}

/**
 * Routine that selects which LED's current is selected
 * to send back to the mainboard (and the ATWDs).  
 * Can also select the 3.3v driver pulse.  Also allows for
 * disabling the mux for power saving.
 */
void hal_FB_select_mux_input(UBYTE value) {

    UBYTE enable, select;

    /* See API for documentation on this mux structure */
    /* Note select calculation depends on enum values */
    switch (value) {
    /* all off */
    case DOM_FB_MUX_DISABLE:
        enable = 0xfc;
        select = 0x0;
        break;
    case DOM_FB_MUX_LED_1:
    case DOM_FB_MUX_LED_2:
        enable = 0x18;
        select = 0x0 + ((value-1) & 1);
        break;
    case DOM_FB_MUX_LED_7:
    case DOM_FB_MUX_LED_8:
        enable = 0x18;
        select = 0x2 + ((value-1) & 1);
        break;
    case DOM_FB_MUX_LED_3:
    case DOM_FB_MUX_LED_4:
        enable = 0x14;
        select = 0x4 + ((value-1) & 1);
        break;
    case DOM_FB_MUX_LED_9:
    case DOM_FB_MUX_LED_10:
        enable = 0x14;
        select = 0x6 + ((value-1) & 1);
        break;
    case DOM_FB_MUX_LED_5:
    case DOM_FB_MUX_LED_6:
        enable = 0x0c;
        select = 0x8 + ((value-1) & 1);
        break;
    case DOM_FB_MUX_LED_11:
    case DOM_FB_MUX_LED_12:
        enable = 0x0c;
        select = 0xa + ((value-1) & 1);
        break;
    case DOM_FB_MUX_PULSE:
        enable = 0x1c;
        select = 0xc;
        break;
    default:
        enable = 0xfc;
        select = 0x0;
    }

    FB(LED_MUX_EN) = enable;
    FB(LED_MUX)    = select;
}
