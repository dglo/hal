/**
 * \file fb-hal.c
 *
 * $Revision: 1.1.1.13 $
 * $Author: arthur $
 * $Date: 2006-06-27 22:04:48 $
 *
 * The DOM flasher board HAL.
 *
 * Modified 2005-1-22 Jacobsen - support adjustable rate and ATWD launch delay 
 *
 */
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "hal/DOM_MB_pld.h"
#include "hal/DOM_MB_fb.h"
#include "hal/DOM_MB_domapp.h"
#include "hal/DOM_MB_fpga.h"
#include "DOM_FB_regs.h"
#include "fb-hal.h"

int getFBclock(void) {
    return (((FB(CLK_HI) & 0xff) << 8) | (FB(CLK_LO) & 0xff));
}

static int fbIsPowered = 0;

int hal_FB_enable(int *config_time, int *valid_time, int *reset_time, DOM_FPGA_TYPE fpga_type) {

    /* Loop timeout limit for ATTN ack sequence, in us */
    int ack_timeout = 50000;

    /* Loop timeout limit for clock validation check, in us */
    int vld_timeout = 500000;

    /* Loop timeout limit for reset ack sequence, in us */
    /* Note must be > ack_timeout + vld_timeout to make sense */
    int rst_timeout = 600000;

    /* Keep track of time required for reset sequence */
    *config_time = 0;
    *valid_time  = 0;
    *reset_time  = 0;

    /* Enable the flasherboard interface in the mainboard CPLD */
    halEnableFlasher();

    unsigned long long reset_start_time;
    if (fpga_type == DOM_FPGA_DOMAPP)
        reset_start_time = hal_FPGA_DOMAPP_get_local_clock();
    else
        reset_start_time = hal_FPGA_TEST_get_local_clock();

    /* Start FPGA ack reset sequence -- aux reset functions independently */
    /* of 20MHz clock to test for CPLD configuration */
    int attn, last_attn;
    int state_cnt = 0;
    int done = 0;

    unsigned long long start_time;
    if (fpga_type == DOM_FPGA_DOMAPP) {
        start_time = hal_FPGA_DOMAPP_get_local_clock();    
        last_attn = hal_FPGA_DOMAPP_FB_get_attn();
    }
    else {
        start_time = hal_FPGA_TEST_get_local_clock();
        last_attn = hal_FPGA_TEST_FB_get_attn();
    }

    while (!done) {
        /* Aux_reset "clock" cycle */
        if (fpga_type == DOM_FPGA_DOMAPP) {
            hal_FPGA_DOMAPP_FB_set_aux_reset();
            hal_FPGA_DOMAPP_FB_clear_aux_reset();
            /* Monitor how long this is taking */
            *config_time = (int)(hal_FPGA_DOMAPP_get_local_clock() - start_time) / 
                (FPGA_HAL_TICKS_PER_SEC / 1000000);
            /* Check for state change on ATTN */
            attn = hal_FPGA_DOMAPP_FB_get_attn();
        }
        else {
            hal_FPGA_TEST_FB_set_aux_reset();
            hal_FPGA_TEST_FB_clear_aux_reset();
            /* Monitor how long this is taking */
            *config_time = (int)(hal_FPGA_TEST_get_local_clock() - start_time) / 
                (FPGA_HAL_TICKS_PER_SEC / 1000000);
            /* Check for state change on ATTN */
            attn = hal_FPGA_TEST_FB_get_attn();
        }

        if (attn != last_attn) 
            state_cnt++;

        /* Watch for 4 state changes, ending in a zero state */
        done = (*config_time > ack_timeout) || ((state_cnt >= 4) && (attn == 0));
    }

    if (*config_time > ack_timeout) {
        if (fpga_type == DOM_FPGA_DOMAPP)
            hal_FPGA_DOMAPP_FB_clear_aux_reset();
        else
            hal_FPGA_TEST_FB_clear_aux_reset();
        hal_FB_disable();
        return FB_HAL_ERR_CONFIG_TIME;
    }

    halUSleep(1000);

    /* Validate that clock is running correctly */
    done = 0;
    if (fpga_type == DOM_FPGA_DOMAPP)
        start_time = hal_FPGA_DOMAPP_get_local_clock();
    else
        start_time = hal_FPGA_TEST_get_local_clock();

    int fb_clk_us;
    int vld_cnt = 0;
    unsigned long long stable_start = 0;
    unsigned long long now, loop_start;
    int stable_time = 0;

    while (!done) {
        if (fpga_type == DOM_FPGA_DOMAPP)
            loop_start = hal_FPGA_DOMAPP_get_local_clock();
        else
            loop_start = hal_FPGA_TEST_get_local_clock();

        /* Reset the flasherboard CPLD, and wait 250us */
        FB(RESET) = 0x1;
        halUSleep(250);

        /* Check the FB counter */
        fb_clk_us = getFBclock() / (FB_HAL_TICKS_PER_SEC / 1000000);
        if (abs(fb_clk_us - 250) < 5) {
            if (vld_cnt == 0)
                stable_start = loop_start;
            vld_cnt++;

        }
        else
            vld_cnt = 0;

        /* Monitor how long this is taking */
        if (fpga_type == DOM_FPGA_DOMAPP)
            now = hal_FPGA_DOMAPP_get_local_clock();
        else
            now = hal_FPGA_TEST_get_local_clock();

        *valid_time = (int)(now - start_time) / (FPGA_HAL_TICKS_PER_SEC / 1000000);        
        stable_time = (int)(now - stable_start) / (FPGA_HAL_TICKS_PER_SEC / 1000000);        
        
        /* Are we done? Check that clock is running and is stable */
        done = (*valid_time > vld_timeout) || (vld_cnt == 8);
    }
    
    if (*valid_time > vld_timeout) {
        if (fpga_type == DOM_FPGA_DOMAPP)
            hal_FPGA_DOMAPP_FB_clear_aux_reset();
        else
            hal_FPGA_TEST_FB_clear_aux_reset();
        
        hal_FB_disable();
        return FB_HAL_ERR_VALID_TIME;
    }

    /* Correct valid time for stable period */
    *valid_time -= stable_time;

    /* Validate that the power-on reset has occurred */
    done = 0;
    int reset_ack = 0;
    while (!done) {

        /* Check the power-on reset ACK bit (active low) */
        reset_ack = ((FB(VERSION) &  DOM_FB_VERSION_RESET_ACKN) == 0);

        /* Monitor how long this is taking */
        if (fpga_type == DOM_FPGA_DOMAPP) {
            *reset_time = (int)(hal_FPGA_DOMAPP_get_local_clock() - reset_start_time) / 
                (FPGA_HAL_TICKS_PER_SEC / 1000000);
        }
        else {
            *reset_time = (int)(hal_FPGA_TEST_get_local_clock() - reset_start_time) / 
                (FPGA_HAL_TICKS_PER_SEC / 1000000);
        }
        /* Are we done? Check that clock is running and is stable */
        done = (*reset_time > rst_timeout) || (reset_ack);
    }

    if (*reset_time > rst_timeout) {
        if (fpga_type == DOM_FPGA_DOMAPP)
            hal_FPGA_DOMAPP_FB_clear_aux_reset();
        else
            hal_FPGA_TEST_FB_clear_aux_reset();
        hal_FB_disable();
        return FB_HAL_ERR_RESET_TIME;
    }    

    /* Wait a bit longer just to be sure clock is stable */
    halUSleep(*valid_time / 5);
    
    /* Enable delay chip */
    FB(LE_DP) = 0x1;

    /* Initialize pulse width to something short */
    hal_FB_set_pulse_width(64);

    /* Toggle ATTN again to clear out LED pulse flip-flops */
    if (fpga_type == DOM_FPGA_DOMAPP) {
        hal_FPGA_DOMAPP_FB_set_aux_reset();
        hal_FPGA_DOMAPP_FB_clear_aux_reset();
    }
    else {
        hal_FPGA_TEST_FB_set_aux_reset();
        hal_FPGA_TEST_FB_clear_aux_reset();
    }

    /* Power on the DC-DC converter */
    hal_FB_set_DCDCen(1);

    /* Initialize mux selects (turn off mux as default) */
    hal_FB_select_mux_input(DOM_FB_MUX_DISABLE);

    /* Initialize brightness to minimum */
    hal_FB_set_brightness(0);

    /* Wait a little while */
    halUSleep(100000);

    fbIsPowered = 1;

    return 0;
}

void hal_FB_enable_min(void) {

    /* Enable the flasherboard interface in the mainboard CPLD */
    halEnableFlasher();
    halUSleep(100000);

    /* Reset the flasherboard CPLD */
    FB(RESET) = 0x1;
    halUSleep(100000);

    fbIsPowered = 1;
}

void hal_FB_disable(void) {

    /* Make sure all LEDs are off */
    hal_FB_enable_LEDs(0);

    /* Power off the DC-DC converter */
    hal_FB_set_DCDCen(0);
    halUSleep(5000);

    /* Disable the flasherboard interface in the mainboard CPLD */
    halDisableFlasher();

    /* Wait a while */
    halUSleep(200000);

    fbIsPowered = 0;
}

void hal_FB_set_DCDCen(int val) {
    FB(DCDC_CTRL) = 0x1 & val;
}

int hal_FB_get_DCDCen(void) {
    return FB(DCDC_CTRL);
}

int hal_FB_isEnabled(void) {
    return fbIsPowered;
}

static inline void waitOneWireBusy(void) { while (RFBBIT(ONE_WIRE, BUSY)) ; }

/*
 * Routine to read the unique serial number of the flasher board.
 */
int hal_FB_get_serial(char **id) {
    int i;
    const char *hexdigit = "0123456789abcdef";
    static BOOLEAN read = FALSE;
    static unsigned char t[17];
    unsigned char id_raw[8];

    /* Only read out ID on first call -- DISABLED FOR NOW */
    if (read == FALSE) {
        memset(t, 0, 17);
        memset(id_raw, 0, 8);
        *id = t;

        /* Reset one-wire */
        FB(ONE_WIRE) = 0x0f;
        halUSleep(1000);
        waitOneWireBusy();

        /* Check for presence */
        if (RFBBIT(ONE_WIRE, PRESENT) == 0)
            return FB_HAL_ERR_ID_NOT_PRESENT;
        
        for (i=0; i<8; i++) {
            FB(ONE_WIRE) = ( (0x33>>i) & 1 ) ? 0x9 : 0xa;
            halUSleep(1000);
            waitOneWireBusy();
        }
        
        /* LSB comes out first */
        for (i=0; i<64; i++) {
            FB(ONE_WIRE) = 0xb;
            waitOneWireBusy();
            id_raw[i/8]>>=1;
            if (RFBBIT(ONE_WIRE, DATA)) {
                id_raw[i/8] |= 0x80;
            }
        }

        /* Only copy over result if passes CRC check */
        /* halCheckCRC returns true on fail */
        if (!halCheckCRC(id_raw, 8)) {
            /* Translate binary char to ASCII hex digits */
            for (i=7; i>=0; i--) {
                t[2*(7-i)]   = hexdigit[(int)((id_raw[i]>>4) & 0xf)];
                t[2*(7-i)+1] = hexdigit[(int)(id_raw[i] & 0xf)];
            }

            /* DISABLE -- read out every time for now */
            /* read = TRUE; */
        }
        else {
            /* Bad CRC */
            strcpy(t, "deadbeef");
            return FB_HAL_ERR_ID_BAD_CRC;
        }
    }
    
    return 0;
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
    /* Mask out reset ack bit */
    return (FB(VERSION) & ~DOM_FB_VERSION_RESET_ACKN);
}

/**
 * Routine that sets the flash pulse width for all LEDs
 * on the flasher board.
 */
void hal_FB_set_pulse_width(UBYTE value) {
    FB(DELAY_ADJUST) = value;
}

/**
 * Routine that sets the flash brightness for all LEDs
 * on the flasher board.
 */
void hal_FB_set_brightness(UBYTE value) {
    
    FB(SPI_CTRL) = value;
    halUSleep(10000);
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

/*************************************************************************/
/* XSVF reprogamming code                                                */
/*                                                                       */
/* This code has been adapted from the Xilinx application note XAPP058,  */
/* code version 5.01, "Xilinx In-System Programming Using an Embedded    */
/* Microprocessor."                                                      */
/*************************************************************************/

/***********************/
/* From ports.c        */
/***********************/

/* Set JTAG output ports.  Usage in micro.c expects 1/2 */
/* cycle wait after setting TCK. */
void setPort(short p,short val) {
    if (p==TMS)
        hal_FPGA_TEST_FB_JTAG_set_TMS((unsigned char)val);
    else if (p==TDI)
        hal_FPGA_TEST_FB_JTAG_set_TDI((unsigned char)val);
    else if (p==TCK) {
        hal_FPGA_TEST_FB_JTAG_set_TCK((unsigned char)val);
        halUSleep(TCK_CYCLE_US/2);
    }
}

static int *xsvf_p;
static int  xsvf_nbytes;
static long xsvf_bytenum;

/* Initialize the pointers and variables for xsvf data reading */
void xsvfReadInit(int *p, int nbytes) {
    xsvf_p = p;
    xsvf_nbytes = nbytes;
    xsvf_bytenum = 0;
}

/* Read in one byte of data from XSVF file (in flash) */
void readByte(unsigned char *data) {

    static int this_word = 0;
    char offset;

    offset = xsvf_bytenum % 4;

    /* Get the next word */
    if (offset == 0)
        this_word = xsvf_p[xsvf_bytenum / 4];

    *data = (unsigned char)((this_word >> (8*offset)) & 0xff);
    xsvf_bytenum++;      
}

/* read the TDO bit from port */
unsigned char readTDOBit() {
    return hal_FPGA_TEST_FB_JTAG_get_TDO();
}

/* Wait at least the specified number of microsec. */
void waitTime(long microsec) {
    /* TCK should be low for wait as used here */
    setPort(TCK,0);
    halUSleep(microsec);
}

/***********************/
/* From lenval.c       */
/***********************/

/*****************************************************************************
* Function:     value
* Description:  Extract the long value from the lenval array.
* Parameters:   plvValue    - ptr to lenval.
* Returns:      long        - the extracted value.
*****************************************************************************/
long value( lenVal*     plvValue )
{
	long    lValue;         /* result to hold the accumulated result */
	short   sIndex;

    lValue  = 0;
	for ( sIndex = 0; sIndex < plvValue->len ; ++sIndex )
	{
		lValue <<= 8;                       /* shift the accumulated result */
		lValue |= plvValue->val[ sIndex];   /* get the last byte first */
	}

	return( lValue );
}

/*****************************************************************************
* Function:     initLenVal
* Description:  Initialize the lenval array with the given value.
*               Assumes lValue is less than 256.
* Parameters:   plv         - ptr to lenval.
*               lValue      - the value to set.
* Returns:      void.
*****************************************************************************/
void initLenVal( lenVal*    plv,
                 long       lValue )
{
	plv->len    = 1;
	plv->val[0] = (unsigned char)lValue;
}

/*****************************************************************************
* Function:     EqualLenVal
* Description:  Compare two lenval arrays with an optional mask.
* Parameters:   plvTdoExpected  - ptr to lenval #1.
*               plvTdoCaptured  - ptr to lenval #2.
*               plvTdoMask      - optional ptr to mask (=0 if no mask).
* Returns:      short   - 0 = mismatch; 1 = equal.
*****************************************************************************/
short EqualLenVal( lenVal*  plvTdoExpected,
                   lenVal*  plvTdoCaptured,
                   lenVal*  plvTdoMask )
{
    short           sEqual;
	short           sIndex;
    unsigned char   ucByteVal1;
    unsigned char   ucByteVal2;
    unsigned char   ucByteMask;

    sEqual  = 1;
    sIndex  = plvTdoExpected->len;

    while ( sEqual && sIndex-- )
    {
        ucByteVal1  = plvTdoExpected->val[ sIndex ];
        ucByteVal2  = plvTdoCaptured->val[ sIndex ];
        if ( plvTdoMask )
        {
            ucByteMask  = plvTdoMask->val[ sIndex ];
            ucByteVal1  &= ucByteMask;
            ucByteVal2  &= ucByteMask;
        }
        if ( ucByteVal1 != ucByteVal2 )
        {
            sEqual  = 0;
        }
    }

	return( sEqual );
}


/*****************************************************************************
* Function:     RetBit
* Description:  return the (byte, bit) of lv (reading from left to right).
* Parameters:   plv     - ptr to lenval.
*               iByte   - the byte to get the bit from.
*               iBit    - the bit number (0=msb)
* Returns:      short   - the bit value.
*****************************************************************************/
short RetBit( lenVal*   plv,
              int       iByte,
              int       iBit )
{
    /* assert( ( iByte >= 0 ) && ( iByte < plv->len ) ); */
    /* assert( ( iBit >= 0 ) && ( iBit < 8 ) ); */
    return( (short)( ( plv->val[ iByte ] >> ( 7 - iBit ) ) & 0x1 ) );
}

/*****************************************************************************
* Function:     SetBit
* Description:  set the (byte, bit) of lv equal to val
* Example:      SetBit("00000000",byte, 1) equals "01000000".
* Parameters:   plv     - ptr to lenval.
*               iByte   - the byte to get the bit from.
*               iBit    - the bit number (0=msb).
*               sVal    - the bit value to set.
* Returns:      void.
*****************************************************************************/
void SetBit( lenVal*    plv,
             int        iByte,
             int        iBit,
             short      sVal )
{
    unsigned char   ucByteVal;
    unsigned char   ucBitMask;

    ucBitMask   = (unsigned char)(1 << ( 7 - iBit ));
    ucByteVal   = (unsigned char)(plv->val[ iByte ] & (~ucBitMask));

    if ( sVal )
    {
        ucByteVal   |= ucBitMask;
    }
    plv->val[ iByte ]   = ucByteVal;
}

/*****************************************************************************
* Function:     AddVal
* Description:  add val1 to val2 and store in resVal;
*               assumes val1 and val2  are of equal length.
* Parameters:   plvResVal   - ptr to result.
*               plvVal1     - ptr of addendum.
*               plvVal2     - ptr of addendum.
* Returns:      void.
*****************************************************************************/
void addVal( lenVal*    plvResVal,
             lenVal*    plvVal1,
             lenVal*    plvVal2 )
{
	unsigned char   ucCarry;
    unsigned short  usSum;
    unsigned short  usVal1;
    unsigned short  usVal2;
	short           sIndex;
	
	plvResVal->len  = plvVal1->len;         /* set up length of result */
	
	/* start at least significant bit and add bytes    */
    ucCarry = 0;
    sIndex  = plvVal1->len;
    while ( sIndex-- )
    {
		usVal1  = plvVal1->val[ sIndex ];   /* i'th byte of val1 */
		usVal2  = plvVal2->val[ sIndex ];   /* i'th byte of val2 */
		
		/* add the two bytes plus carry from previous addition */
		usSum   = (unsigned short)( usVal1 + usVal2 + ucCarry );
		
		/* set up carry for next byte */
		ucCarry = (unsigned char)( ( usSum > 255 ) ? 1 : 0 );
		
        /* set the i'th byte of the result */
		plvResVal->val[ sIndex ]    = (unsigned char)usSum;
    }
}

/*****************************************************************************
* Function:     readVal
* Description:  read from XSVF numBytes bytes of data into x.
* Parameters:   plv         - ptr to lenval in which to put the bytes read.
*               sNumBytes   - the number of bytes to read.
* Returns:      void.
*****************************************************************************/
void readVal( lenVal*   plv,
              short     sNumBytes )
{
    unsigned char*  pucVal;
	
    plv->len    = sNumBytes;        /* set the length of the lenVal        */
    for ( pucVal = plv->val; sNumBytes; --sNumBytes, ++pucVal )
    {
        /* read a byte of data into the lenVal */
		readByte( pucVal );
    }
}

/***********************/
/* From micro.c        */
/***********************/

/*============================================================================
* XSVF Global Variables
============================================================================*/

/* Array of XSVF command functions.  Must follow command byte value order! */
/* If your compiler cannot take this form, then convert to a switch statement*/
TXsvfDoCmdFuncPtr   xsvf_pfDoCmd[]  =
{
    xsvfDoXCOMPLETE,        /*  0 */
    xsvfDoXTDOMASK,         /*  1 */
    xsvfDoXSIR,             /*  2 */
    xsvfDoXSDR,             /*  3 */
    xsvfDoXRUNTEST,         /*  4 */
    xsvfDoIllegalCmd,       /*  5 */
    xsvfDoIllegalCmd,       /*  6 */
    xsvfDoXREPEAT,          /*  7 */
    xsvfDoXSDRSIZE,         /*  8 */
    xsvfDoXSDRTDO,          /*  9 */
    xsvfDoIllegalCmd,       /* 10 */
    xsvfDoIllegalCmd,       /* 11 */
    xsvfDoXSDRBCE,          /* 12 */
    xsvfDoXSDRBCE,          /* 13 */
    xsvfDoXSDRBCE,          /* 14 */
    xsvfDoXSDRTDOBCE,       /* 15 */
    xsvfDoXSDRTDOBCE,       /* 16 */
    xsvfDoXSDRTDOBCE,       /* 17 */
    xsvfDoXSTATE,           /* 18 */
    xsvfDoXENDXR,           /* 19 */
    xsvfDoXENDXR,           /* 20 */
    xsvfDoXSIR2,            /* 21 */
    xsvfDoXCOMMENT,         /* 22 */
    xsvfDoXWAIT             /* 23 */
/* Insert new command functions here */
};

#ifdef  DEBUG_MODE
    char* xsvf_pzCommandName[]  =
    {
        "XCOMPLETE",
        "XTDOMASK",
        "XSIR",
        "XSDR",
        "XRUNTEST",
        "Reserved5",
        "Reserved6",
        "XREPEAT",
        "XSDRSIZE",
        "XSDRTDO",
        "XSETSDRMASKS",
        "XSDRINC",
        "XSDRB",
        "XSDRC",
        "XSDRE",
        "XSDRTDOB",
        "XSDRTDOC",
        "XSDRTDOE",
        "XSTATE",
        "XENDIR",
        "XENDDR",
        "XSIR2",
        "XCOMMENT",
        "XWAIT"
    };

    char*   xsvf_pzErrorName[]  =
    {
        "No error",
        "ERROR:  Unknown",
        "ERROR:  TDO mismatch",
        "ERROR:  TDO mismatch and exceeded max retries",
        "ERROR:  Unsupported XSVF command",
        "ERROR:  Illegal state specification",
        "ERROR:  Data overflows allocated MAX_LEN buffer size"
    };

    char*   xsvf_pzTapState[] =
    {
        "RESET",        /* 0x00 */
        "RUNTEST/IDLE", /* 0x01 */
        "DRSELECT",     /* 0x02 */
        "DRCAPTURE",    /* 0x03 */
        "DRSHIFT",      /* 0x04 */
        "DREXIT1",      /* 0x05 */
        "DRPAUSE",      /* 0x06 */
        "DREXIT2",      /* 0x07 */
        "DRUPDATE",     /* 0x08 */
        "IRSELECT",     /* 0x09 */
        "IRCAPTURE",    /* 0x0A */
        "IRSHIFT",      /* 0x0B */
        "IREXIT1",      /* 0x0C */
        "IRPAUSE",      /* 0x0D */
        "IREXIT2",      /* 0x0E */
        "IRUPDATE"      /* 0x0F */
    };
#endif  /* DEBUG_MODE */

#ifdef DEBUG_MODE
    int xsvf_iDebugLevel = 2;
#else
    int xsvf_iDebugLevel = 0;
#endif

/*============================================================================
* Utility Functions
============================================================================*/

/*****************************************************************************
* Function:     xsvfPrintLenVal
* Description:  Print the lenval value in hex.
* Parameters:   plv     - ptr to lenval.
* Returns:      void.
*****************************************************************************/
#ifdef  DEBUG_MODE
void xsvfPrintLenVal( lenVal *plv )
{
    int i;

    if ( plv )
    {
        printf( "0x" );
        for ( i = 0; i < plv->len; ++i )
        {
            printf( "%02x", ((unsigned int)(plv->val[ i ])) );
        }
    }
}
#endif  /* DEBUG_MODE */


/*****************************************************************************
* Function:     xsvfInfoInit
* Description:  Initialize the xsvfInfo data.
* Parameters:   pXsvfInfo   - ptr to the XSVF info structure.
* Returns:      int         - 0 = success; otherwise error.
*****************************************************************************/
int xsvfInfoInit( SXsvfInfo* pXsvfInfo )
{

    pXsvfInfo->ucComplete       = 0;
    pXsvfInfo->ucCommand        = XCOMPLETE;
    pXsvfInfo->lCommandCount    = 0;
    pXsvfInfo->iErrorCode       = XSVF_ERROR_NONE;
    pXsvfInfo->ucMaxRepeat      = 0;
    pXsvfInfo->ucTapState       = XTAPSTATE_RESET;
    pXsvfInfo->ucEndIR          = XTAPSTATE_RUNTEST;
    pXsvfInfo->ucEndDR          = XTAPSTATE_RUNTEST;
    pXsvfInfo->lShiftLengthBits = 0L;
    pXsvfInfo->sShiftLengthBytes= 0;
    pXsvfInfo->lRunTestTime     = 0L;

    return( 0 );
}

/*****************************************************************************
* Function:     xsvfGetAsNumBytes
* Description:  Calculate the number of bytes the given number of bits
*               consumes.
* Parameters:   lNumBits    - the number of bits.
* Returns:      short       - the number of bytes to store the number of bits.
*****************************************************************************/
short xsvfGetAsNumBytes( long lNumBits )
{
    return( (short)( ( lNumBits + 7L ) / 8L ) );
}

/*****************************************************************************
* Function:     xsvfTmsTransition
* Description:  Apply TMS and transition TAP controller by applying one TCK
*               cycle.
* Parameters:   sTms    - new TMS value.
* Returns:      void.
*****************************************************************************/
void xsvfTmsTransition( short sTms )
{
    setPort( TMS, sTms );
    setPort( TCK, 0 );
    setPort( TCK, 1 );
}

/*****************************************************************************
* Function:     xsvfGotoTapState
* Description:  From the current TAP state, go to the named TAP state.
*               A target state of RESET ALWAYS causes TMS reset sequence.
*               All SVF standard stable state paths are supported.
*               All state transitions are supported except for the following
*               which cause an XSVF_ERROR_ILLEGALSTATE:
*                   - Target==DREXIT2;  Start!=DRPAUSE
*                   - Target==IREXIT2;  Start!=IRPAUSE
* Parameters:   pucTapState     - Current TAP state; returns final TAP state.
*               ucTargetState   - New target TAP state.
* Returns:      int             - 0 = success; otherwise error.
*****************************************************************************/
int xsvfGotoTapState( unsigned char*   pucTapState,
                      unsigned char    ucTargetState )
{
    int i;
    int iErrorCode;

    iErrorCode  = XSVF_ERROR_NONE;
    if ( ucTargetState == XTAPSTATE_RESET )
    {
        /* If RESET, always perform TMS reset sequence to reset/sync TAPs */
        xsvfTmsTransition( 1 );
        for ( i = 0; i < 5; ++i )
        {
            setPort( TCK, 0 );
            setPort( TCK, 1 );
        }
        *pucTapState    = XTAPSTATE_RESET;
        XSVFDBG_PRINTF( 3, "   TMS Reset Sequence -> Test-Logic-Reset\n" );
        XSVFDBG_PRINTF1( 3, "   TAP State = %s\n",
                         xsvf_pzTapState[ *pucTapState ] );
    }
    else if ( ( ucTargetState != *pucTapState ) &&
              ( ( ( ucTargetState == XTAPSTATE_EXIT2DR ) && ( *pucTapState != XTAPSTATE_PAUSEDR ) ) ||
                ( ( ucTargetState == XTAPSTATE_EXIT2IR ) && ( *pucTapState != XTAPSTATE_PAUSEIR ) ) ) )
    {
        /* Trap illegal TAP state path specification */
        iErrorCode      = XSVF_ERROR_ILLEGALSTATE;
    }
    else
    {
        if ( ucTargetState == *pucTapState )
        {
            /* Already in target state.  Do nothing except when in DRPAUSE
               or in IRPAUSE to comply with SVF standard */
            if ( ucTargetState == XTAPSTATE_PAUSEDR )
            {
                xsvfTmsTransition( 1 );
                *pucTapState    = XTAPSTATE_EXIT2DR;
                XSVFDBG_PRINTF1( 3, "   TAP State = %s\n",
                                 xsvf_pzTapState[ *pucTapState ] );
            }
            else if ( ucTargetState == XTAPSTATE_PAUSEIR )
            {
                xsvfTmsTransition( 1 );
                *pucTapState    = XTAPSTATE_EXIT2IR;
                XSVFDBG_PRINTF1( 3, "   TAP State = %s\n",
                                 xsvf_pzTapState[ *pucTapState ] );
            }
        }

        /* Perform TAP state transitions to get to the target state */
        while ( ucTargetState != *pucTapState )
        {
            switch ( *pucTapState )
            {
            case XTAPSTATE_RESET:
                xsvfTmsTransition( 0 );
                *pucTapState    = XTAPSTATE_RUNTEST;
                break;
            case XTAPSTATE_RUNTEST:
                xsvfTmsTransition( 1 );
                *pucTapState    = XTAPSTATE_SELECTDR;
                break;
            case XTAPSTATE_SELECTDR:
                if ( ucTargetState >= XTAPSTATE_IRSTATES )
                {
                    xsvfTmsTransition( 1 );
                    *pucTapState    = XTAPSTATE_SELECTIR;
                }
                else
                {
                    xsvfTmsTransition( 0 );
                    *pucTapState    = XTAPSTATE_CAPTUREDR;
                }
                break;
            case XTAPSTATE_CAPTUREDR:
                if ( ucTargetState == XTAPSTATE_SHIFTDR )
                {
                    xsvfTmsTransition( 0 );
                    *pucTapState    = XTAPSTATE_SHIFTDR;
                }
                else
                {
                    xsvfTmsTransition( 1 );
                    *pucTapState    = XTAPSTATE_EXIT1DR;
                }
                break;
            case XTAPSTATE_SHIFTDR:
                xsvfTmsTransition( 1 );
                *pucTapState    = XTAPSTATE_EXIT1DR;
                break;
            case XTAPSTATE_EXIT1DR:
                if ( ucTargetState == XTAPSTATE_PAUSEDR )
                {
                    xsvfTmsTransition( 0 );
                    *pucTapState    = XTAPSTATE_PAUSEDR;
                }
                else
                {
                    xsvfTmsTransition( 1 );
                    *pucTapState    = XTAPSTATE_UPDATEDR;
                }
                break;
            case XTAPSTATE_PAUSEDR:
                xsvfTmsTransition( 1 );
                *pucTapState    = XTAPSTATE_EXIT2DR;
                break;
            case XTAPSTATE_EXIT2DR:
                if ( ucTargetState == XTAPSTATE_SHIFTDR )
                {
                    xsvfTmsTransition( 0 );
                    *pucTapState    = XTAPSTATE_SHIFTDR;
                }
                else
                {
                    xsvfTmsTransition( 1 );
                    *pucTapState    = XTAPSTATE_UPDATEDR;
                }
                break;
            case XTAPSTATE_UPDATEDR:
                if ( ucTargetState == XTAPSTATE_RUNTEST )
                {
                    xsvfTmsTransition( 0 );
                    *pucTapState    = XTAPSTATE_RUNTEST;
                }
                else
                {
                    xsvfTmsTransition( 1 );
                    *pucTapState    = XTAPSTATE_SELECTDR;
                }
                break;
            case XTAPSTATE_SELECTIR:
                xsvfTmsTransition( 0 );
                *pucTapState    = XTAPSTATE_CAPTUREIR;
                break;
            case XTAPSTATE_CAPTUREIR:
                if ( ucTargetState == XTAPSTATE_SHIFTIR )
                {
                    xsvfTmsTransition( 0 );
                    *pucTapState    = XTAPSTATE_SHIFTIR;
                }
                else
                {
                    xsvfTmsTransition( 1 );
                    *pucTapState    = XTAPSTATE_EXIT1IR;
                }
                break;
            case XTAPSTATE_SHIFTIR:
                xsvfTmsTransition( 1 );
                *pucTapState    = XTAPSTATE_EXIT1IR;
                break;
            case XTAPSTATE_EXIT1IR:
                if ( ucTargetState == XTAPSTATE_PAUSEIR )
                {
                    xsvfTmsTransition( 0 );
                    *pucTapState    = XTAPSTATE_PAUSEIR;
                }
                else
                {
                    xsvfTmsTransition( 1 );
                    *pucTapState    = XTAPSTATE_UPDATEIR;
                }
                break;
            case XTAPSTATE_PAUSEIR:
                xsvfTmsTransition( 1 );
                *pucTapState    = XTAPSTATE_EXIT2IR;
                break;
            case XTAPSTATE_EXIT2IR:
                if ( ucTargetState == XTAPSTATE_SHIFTIR )
                {
                    xsvfTmsTransition( 0 );
                    *pucTapState    = XTAPSTATE_SHIFTIR;
                }
                else
                {
                    xsvfTmsTransition( 1 );
                    *pucTapState    = XTAPSTATE_UPDATEIR;
                }
                break;
            case XTAPSTATE_UPDATEIR:
                if ( ucTargetState == XTAPSTATE_RUNTEST )
                {
                    xsvfTmsTransition( 0 );
                    *pucTapState    = XTAPSTATE_RUNTEST;
                }
                else
                {
                    xsvfTmsTransition( 1 );
                    *pucTapState    = XTAPSTATE_SELECTDR;
                }
                break;
            default:
                iErrorCode      = XSVF_ERROR_ILLEGALSTATE;
                *pucTapState    = ucTargetState;    /* Exit while loop */
                break;
            }
            XSVFDBG_PRINTF1( 3, "   TAP State = %s\n",
                             xsvf_pzTapState[ *pucTapState ] );
        }
    }

    return( iErrorCode );
}

/*****************************************************************************
* Function:     xsvfShiftOnly
* Description:  Assumes that starting TAP state is SHIFT-DR or SHIFT-IR.
*               Shift the given TDI data into the JTAG scan chain.
*               Optionally, save the TDO data shifted out of the scan chain.
*               Last shift cycle is special:  capture last TDO, set last TDI,
*               but does not pulse TCK.  Caller must pulse TCK and optionally
*               set TMS=1 to exit shift state.
* Parameters:   lNumBits        - number of bits to shift.
*               plvTdi          - ptr to lenval for TDI data.
*               plvTdoCaptured  - ptr to lenval for storing captured TDO data.
*               iExitShift      - 1=exit at end of shift; 0=stay in Shift-DR.
* Returns:      void.
*****************************************************************************/
void xsvfShiftOnly( long    lNumBits,
                    lenVal* plvTdi,
                    lenVal* plvTdoCaptured,
                    int     iExitShift )
{
    unsigned char*  pucTdi;
    unsigned char*  pucTdo;
    unsigned char   ucTdiByte;
    unsigned char   ucTdoByte;
    unsigned char   ucTdoBit;
    int             i;

    /* assert( ( ( lNumBits + 7 ) / 8 ) == plvTdi->len ); */

    /* Initialize TDO storage len == TDI len */
    pucTdo  = 0;
    if ( plvTdoCaptured )
    {
        plvTdoCaptured->len = plvTdi->len;
        pucTdo              = plvTdoCaptured->val + plvTdi->len;
    }

    /* Shift LSB first.  val[N-1] == LSB.  val[0] == MSB. */
    pucTdi  = plvTdi->val + plvTdi->len;
    while ( lNumBits )
    {
        /* Process on a byte-basis */
        ucTdiByte   = (*(--pucTdi));
        ucTdoByte   = 0;
        for ( i = 0; ( lNumBits && ( i < 8 ) ); ++i )
        {
            --lNumBits;
            if ( iExitShift && !lNumBits )
            {
                /* Exit Shift-DR state */
                setPort( TMS, 1 );
            }

            /* Set the new TDI value */
            setPort( TDI, (short)(ucTdiByte & 1) );
            ucTdiByte   >>= 1;

            /* Set TCK low */
            setPort( TCK, 0 );

            if ( pucTdo )
            {
                /* Save the TDO value */
                ucTdoBit    = readTDOBit();
                ucTdoByte   |= ( ucTdoBit << i );
            }

            /* Set TCK high */
            setPort( TCK, 1 );
        }

        /* Save the TDO byte value */
        if ( pucTdo )
        {
            (*(--pucTdo))   = ucTdoByte;
        }
    }
}

/*****************************************************************************
* Function:     xsvfShift
* Description:  Goes to the given starting TAP state.
*               Calls xsvfShiftOnly to shift in the given TDI data and
*               optionally capture the TDO data.
*               Compares the TDO captured data against the TDO expected
*               data.
*               If a data mismatch occurs, then executes the exception
*               handling loop upto ucMaxRepeat times.
* Parameters:   pucTapState     - Ptr to current TAP state.
*               ucStartState    - Starting shift state: Shift-DR or Shift-IR.
*               lNumBits        - number of bits to shift.
*               plvTdi          - ptr to lenval for TDI data.
*               plvTdoCaptured  - ptr to lenval for storing TDO data.
*               plvTdoExpected  - ptr to expected TDO data.
*               plvTdoMask      - ptr to TDO mask.
*               ucEndState      - state in which to end the shift.
*               lRunTestTime    - amount of time to wait after the shift.
*               ucMaxRepeat     - Maximum number of retries on TDO mismatch.
* Returns:      int             - 0 = success; otherwise TDO mismatch.
* Notes:        XC9500XL-only Optimization:
*               Skip the waitTime() if plvTdoMask->val[0:plvTdoMask->len-1]
*               is NOT all zeros and sMatch==1.
*****************************************************************************/
int xsvfShift( unsigned char*   pucTapState,
               unsigned char    ucStartState,
               long             lNumBits,
               lenVal*          plvTdi,
               lenVal*          plvTdoCaptured,
               lenVal*          plvTdoExpected,
               lenVal*          plvTdoMask,
               unsigned char    ucEndState,
               long             lRunTestTime,
               unsigned char    ucMaxRepeat )
{
    int             iErrorCode;
    int             iMismatch;
    unsigned char   ucRepeat;
    int             iExitShift;

    iErrorCode  = XSVF_ERROR_NONE;
    iMismatch   = 0;
    ucRepeat    = 0;
    iExitShift  = ( ucStartState != ucEndState );

    XSVFDBG_PRINTF1( 3, "   Shift Length = %ld\n", lNumBits );
    XSVFDBG_PRINTF( 4, "    TDI          = ");
    XSVFDBG_PRINTLENVAL( 4, plvTdi );
    XSVFDBG_PRINTF( 4, "\n");
    XSVFDBG_PRINTF( 4, "    TDO Expected = ");
    XSVFDBG_PRINTLENVAL( 4, plvTdoExpected );
    XSVFDBG_PRINTF( 4, "\n");

    if ( !lNumBits )
    {
        /* Compatibility with XSVF2.00:  XSDR 0 = no shift, but wait in RTI */
        if ( lRunTestTime )
        {
            /* Wait for prespecified XRUNTEST time */
            xsvfGotoTapState( pucTapState, XTAPSTATE_RUNTEST );
            XSVFDBG_PRINTF1( 3, "   Wait = %ld usec\n", lRunTestTime );
            waitTime( lRunTestTime );
        }
    }
    else
    {
        do
        {
            /* Goto Shift-DR or Shift-IR */
            xsvfGotoTapState( pucTapState, ucStartState );

            /* Shift TDI and capture TDO */
            xsvfShiftOnly( lNumBits, plvTdi, plvTdoCaptured, iExitShift );

            if ( plvTdoExpected )
            {
                /* Compare TDO data to expected TDO data */
                iMismatch   = !EqualLenVal( plvTdoExpected,
                                            plvTdoCaptured,
                                            plvTdoMask );
            }

            if ( iExitShift )
            {
                /* Update TAP state:  Shift->Exit */
                ++(*pucTapState);
                XSVFDBG_PRINTF1( 3, "   TAP State = %s\n",
                                 xsvf_pzTapState[ *pucTapState ] );

                if ( iMismatch && lRunTestTime && ( ucRepeat < ucMaxRepeat ) )
                {
                    XSVFDBG_PRINTF( 4, "    TDO Expected = ");
                    XSVFDBG_PRINTLENVAL( 4, plvTdoExpected );
                    XSVFDBG_PRINTF( 4, "\n");
                    XSVFDBG_PRINTF( 4, "    TDO Captured = ");
                    XSVFDBG_PRINTLENVAL( 4, plvTdoCaptured );
                    XSVFDBG_PRINTF( 4, "\n");
                    XSVFDBG_PRINTF( 4, "    TDO Mask     = ");
                    XSVFDBG_PRINTLENVAL( 4, plvTdoMask );
                    XSVFDBG_PRINTF( 4, "\n");
                    XSVFDBG_PRINTF1( 3, "   Retry #%d\n", ( ucRepeat + 1 ) );
                    /* Do exception handling retry - ShiftDR only */
                    xsvfGotoTapState( pucTapState, XTAPSTATE_PAUSEDR );
                    /* Shift 1 extra bit */
                    xsvfGotoTapState( pucTapState, XTAPSTATE_SHIFTDR );
                    /* Increment RUNTEST time by an additional 25% */
                    lRunTestTime    += ( lRunTestTime >> 2 );
                }
                else
                {
                    /* Do normal exit from Shift-XR */
                    xsvfGotoTapState( pucTapState, ucEndState );
                }

                if ( lRunTestTime )
                {
                    /* Wait for prespecified XRUNTEST time */
                    xsvfGotoTapState( pucTapState, XTAPSTATE_RUNTEST );
                    XSVFDBG_PRINTF1( 3, "   Wait = %ld usec\n", lRunTestTime );
                    waitTime( lRunTestTime );
                }
            }
        } while ( iMismatch && ( ucRepeat++ < ucMaxRepeat ) );
    }

    if ( iMismatch )
    {
        XSVFDBG_PRINTF( 1, " TDO Expected = ");
        XSVFDBG_PRINTLENVAL( 1, plvTdoExpected );
        XSVFDBG_PRINTF( 1, "\n");
        XSVFDBG_PRINTF( 1, " TDO Captured = ");
        XSVFDBG_PRINTLENVAL( 1, plvTdoCaptured );
        XSVFDBG_PRINTF( 1, "\n");
        XSVFDBG_PRINTF( 1, " TDO Mask     = ");
        XSVFDBG_PRINTLENVAL( 1, plvTdoMask );
        XSVFDBG_PRINTF( 1, "\n");
        if ( ucMaxRepeat && ( ucRepeat > ucMaxRepeat ) )
        {
            iErrorCode  = XSVF_ERROR_MAXRETRIES;
        }
        else
        {
            iErrorCode  = XSVF_ERROR_TDOMISMATCH;
        }
    }

    return( iErrorCode );
}

/*****************************************************************************
* Function:     xsvfBasicXSDRTDO
* Description:  Get the XSDRTDO parameters and execute the XSDRTDO command.
*               This is the common function for all XSDRTDO commands.
* Parameters:   pucTapState         - Current TAP state.
*               lShiftLengthBits    - number of bits to shift.
*               sShiftLengthBytes   - number of bytes to read.
*               plvTdi              - ptr to lenval for TDI data.
*               lvTdoCaptured       - ptr to lenval for storing TDO data.
*               iEndState           - state in which to end the shift.
*               lRunTestTime        - amount of time to wait after the shift.
*               ucMaxRepeat         - maximum xc9500/xl retries.
* Returns:      int                 - 0 = success; otherwise TDO mismatch.
*****************************************************************************/
int xsvfBasicXSDRTDO( unsigned char*    pucTapState,
                      long              lShiftLengthBits,
                      short             sShiftLengthBytes,
                      lenVal*           plvTdi,
                      lenVal*           plvTdoCaptured,
                      lenVal*           plvTdoExpected,
                      lenVal*           plvTdoMask,
                      unsigned char     ucEndState,
                      long              lRunTestTime,
                      unsigned char     ucMaxRepeat )
{
    readVal( plvTdi, sShiftLengthBytes );
    if ( plvTdoExpected )
    {
        readVal( plvTdoExpected, sShiftLengthBytes );
    }
    return( xsvfShift( pucTapState, XTAPSTATE_SHIFTDR, lShiftLengthBits,
                       plvTdi, plvTdoCaptured, plvTdoExpected, plvTdoMask,
                       ucEndState, lRunTestTime, ucMaxRepeat ) );
}

/*============================================================================
* XSVF Command Functions (type = TXsvfDoCmdFuncPtr)
* These functions update pXsvfInfo->iErrorCode only on an error.
* Otherwise, the error code is left alone.
* The function returns the error code from the function.
============================================================================*/

/*****************************************************************************
* Function:     xsvfDoIllegalCmd
* Description:  Function place holder for illegal/unsupported commands.
* Parameters:   pXsvfInfo   - XSVF information pointer.
* Returns:      int         - 0 = success;  non-zero = error.
*****************************************************************************/
int xsvfDoIllegalCmd( SXsvfInfo* pXsvfInfo )
{
    XSVFDBG_PRINTF2( 0, "ERROR:  Encountered unsupported command #%d (%s)\n",
                     ((unsigned int)(pXsvfInfo->ucCommand)),
                     ((pXsvfInfo->ucCommand < XLASTCMD)
                      ? (xsvf_pzCommandName[pXsvfInfo->ucCommand])
                      : "Unknown") );
    pXsvfInfo->iErrorCode   = XSVF_ERROR_ILLEGALCMD;
    return( pXsvfInfo->iErrorCode );
}

/*****************************************************************************
* Function:     xsvfDoXCOMPLETE
* Description:  XCOMPLETE (no parameters)
*               Update complete status for XSVF player.
* Parameters:   pXsvfInfo   - XSVF information pointer.
* Returns:      int         - 0 = success;  non-zero = error.
*****************************************************************************/
int xsvfDoXCOMPLETE( SXsvfInfo* pXsvfInfo )
{
    pXsvfInfo->ucComplete   = 1;
    return( XSVF_ERROR_NONE );
}

/*****************************************************************************
* Function:     xsvfDoXTDOMASK
* Description:  XTDOMASK <lenVal.TdoMask[XSDRSIZE]>
*               Prespecify the TDO compare mask.
* Parameters:   pXsvfInfo   - XSVF information pointer.
* Returns:      int         - 0 = success;  non-zero = error.
*****************************************************************************/
int xsvfDoXTDOMASK( SXsvfInfo* pXsvfInfo )
{
    readVal( &(pXsvfInfo->lvTdoMask), pXsvfInfo->sShiftLengthBytes );
    XSVFDBG_PRINTF( 4, "    TDO Mask     = ");
    XSVFDBG_PRINTLENVAL( 4, &(pXsvfInfo->lvTdoMask) );
    XSVFDBG_PRINTF( 4, "\n");
    return( XSVF_ERROR_NONE );
}

/*****************************************************************************
* Function:     xsvfDoXSIR
* Description:  XSIR <(byte)shiftlen> <lenVal.TDI[shiftlen]>
*               Get the instruction and shift the instruction into the TAP.
*               If prespecified XRUNTEST!=0, goto RUNTEST and wait after
*               the shift for XRUNTEST usec.
* Parameters:   pXsvfInfo   - XSVF information pointer.
* Returns:      int         - 0 = success;  non-zero = error.
*****************************************************************************/
int xsvfDoXSIR( SXsvfInfo* pXsvfInfo )
{
    unsigned char   ucShiftIrBits;
    short           sShiftIrBytes;
    int             iErrorCode;

    /* Get the shift length and store */
    readByte( &ucShiftIrBits );
    sShiftIrBytes   = xsvfGetAsNumBytes( ucShiftIrBits );
    XSVFDBG_PRINTF1( 3, "   XSIR length = %d\n",
                     ((unsigned int)ucShiftIrBits) );

    if ( sShiftIrBytes > MAX_LEN )
    {
        iErrorCode  = XSVF_ERROR_DATAOVERFLOW;
    }
    else
    {
        /* Get and store instruction to shift in */
        readVal( &(pXsvfInfo->lvTdi), xsvfGetAsNumBytes( ucShiftIrBits ) );

        /* Shift the data */
        iErrorCode  = xsvfShift( &(pXsvfInfo->ucTapState), XTAPSTATE_SHIFTIR,
                                 ucShiftIrBits, &(pXsvfInfo->lvTdi),
                                 /*plvTdoCaptured*/0, /*plvTdoExpected*/0,
                                 /*plvTdoMask*/0, pXsvfInfo->ucEndIR,
                                 pXsvfInfo->lRunTestTime, /*ucMaxRepeat*/0 );
    }

    if ( iErrorCode != XSVF_ERROR_NONE )
    {
        pXsvfInfo->iErrorCode   = iErrorCode;
    }
    return( iErrorCode );
}

/*****************************************************************************
* Function:     xsvfDoXSIR2
* Description:  XSIR <(2-byte)shiftlen> <lenVal.TDI[shiftlen]>
*               Get the instruction and shift the instruction into the TAP.
*               If prespecified XRUNTEST!=0, goto RUNTEST and wait after
*               the shift for XRUNTEST usec.
* Parameters:   pXsvfInfo   - XSVF information pointer.
* Returns:      int         - 0 = success;  non-zero = error.
*****************************************************************************/
int xsvfDoXSIR2( SXsvfInfo* pXsvfInfo )
{
    long            lShiftIrBits;
    short           sShiftIrBytes;
    int             iErrorCode;

    /* Get the shift length and store */
    readVal( &(pXsvfInfo->lvTdi), 2 );
    lShiftIrBits    = value( &(pXsvfInfo->lvTdi) );
    sShiftIrBytes   = xsvfGetAsNumBytes( lShiftIrBits );

    if ( sShiftIrBytes > MAX_LEN )
    {
        iErrorCode  = XSVF_ERROR_DATAOVERFLOW;
    }
    else
    {
        /* Get and store instruction to shift in */
        readVal( &(pXsvfInfo->lvTdi), xsvfGetAsNumBytes( lShiftIrBits ) );

        /* Shift the data */
        iErrorCode  = xsvfShift( &(pXsvfInfo->ucTapState), XTAPSTATE_SHIFTIR,
                                 lShiftIrBits, &(pXsvfInfo->lvTdi),
                                 /*plvTdoCaptured*/0, /*plvTdoExpected*/0,
                                 /*plvTdoMask*/0, pXsvfInfo->ucEndIR,
                                 pXsvfInfo->lRunTestTime, /*ucMaxRepeat*/0 );
    }

    if ( iErrorCode != XSVF_ERROR_NONE )
    {
        pXsvfInfo->iErrorCode   = iErrorCode;
    }
    return( iErrorCode );
}

/*****************************************************************************
* Function:     xsvfDoXSDR
* Description:  XSDR <lenVal.TDI[XSDRSIZE]>
*               Shift the given TDI data into the JTAG scan chain.
*               Compare the captured TDO with the expected TDO from the
*               previous XSDRTDO command using the previously specified
*               XTDOMASK.
* Parameters:   pXsvfInfo   - XSVF information pointer.
* Returns:      int         - 0 = success;  non-zero = error.
*****************************************************************************/
int xsvfDoXSDR( SXsvfInfo* pXsvfInfo )
{
    int iErrorCode;
    readVal( &(pXsvfInfo->lvTdi), pXsvfInfo->sShiftLengthBytes );
    /* use TDOExpected from last XSDRTDO instruction */
    iErrorCode  = xsvfShift( &(pXsvfInfo->ucTapState), XTAPSTATE_SHIFTDR,
                             pXsvfInfo->lShiftLengthBits, &(pXsvfInfo->lvTdi),
                             &(pXsvfInfo->lvTdoCaptured),
                             &(pXsvfInfo->lvTdoExpected),
                             &(pXsvfInfo->lvTdoMask), pXsvfInfo->ucEndDR,
                             pXsvfInfo->lRunTestTime, pXsvfInfo->ucMaxRepeat );
    if ( iErrorCode != XSVF_ERROR_NONE )
    {
        pXsvfInfo->iErrorCode   = iErrorCode;
    }
    return( iErrorCode );
}

/*****************************************************************************
* Function:     xsvfDoXRUNTEST
* Description:  XRUNTEST <uint32>
*               Prespecify the XRUNTEST wait time for shift operations.
* Parameters:   pXsvfInfo   - XSVF information pointer.
* Returns:      int         - 0 = success;  non-zero = error.
*****************************************************************************/
int xsvfDoXRUNTEST( SXsvfInfo* pXsvfInfo )
{
    readVal( &(pXsvfInfo->lvTdi), 4 );
    pXsvfInfo->lRunTestTime = value( &(pXsvfInfo->lvTdi) );
    XSVFDBG_PRINTF1( 3, "   XRUNTEST = %ld\n", pXsvfInfo->lRunTestTime );
    return( XSVF_ERROR_NONE );
}

/*****************************************************************************
* Function:     xsvfDoXREPEAT
* Description:  XREPEAT <byte>
*               Prespecify the maximum number of XC9500/XL retries.
* Parameters:   pXsvfInfo   - XSVF information pointer.
* Returns:      int         - 0 = success;  non-zero = error.
*****************************************************************************/
int xsvfDoXREPEAT( SXsvfInfo* pXsvfInfo )
{
    readByte( &(pXsvfInfo->ucMaxRepeat) );
    XSVFDBG_PRINTF1( 3, "   XREPEAT = %d\n",
                     ((unsigned int)(pXsvfInfo->ucMaxRepeat)) );
    return( XSVF_ERROR_NONE );
}

/*****************************************************************************
* Function:     xsvfDoXSDRSIZE
* Description:  XSDRSIZE <uint32>
*               Prespecify the XRUNTEST wait time for shift operations.
* Parameters:   pXsvfInfo   - XSVF information pointer.
* Returns:      int         - 0 = success;  non-zero = error.
*****************************************************************************/
int xsvfDoXSDRSIZE( SXsvfInfo* pXsvfInfo )
{
    int iErrorCode;
    iErrorCode  = XSVF_ERROR_NONE;
    readVal( &(pXsvfInfo->lvTdi), 4 );
    pXsvfInfo->lShiftLengthBits = value( &(pXsvfInfo->lvTdi) );
    pXsvfInfo->sShiftLengthBytes= xsvfGetAsNumBytes( pXsvfInfo->lShiftLengthBits );
    XSVFDBG_PRINTF1( 3, "   XSDRSIZE = %ld\n", pXsvfInfo->lShiftLengthBits );
    if ( pXsvfInfo->sShiftLengthBytes > MAX_LEN )
    {
        iErrorCode  = XSVF_ERROR_DATAOVERFLOW;
        pXsvfInfo->iErrorCode   = iErrorCode;
    }
    return( iErrorCode );
}

/*****************************************************************************
* Function:     xsvfDoXSDRTDO
* Description:  XSDRTDO <lenVal.TDI[XSDRSIZE]> <lenVal.TDO[XSDRSIZE]>
*               Get the TDI and expected TDO values.  Then, shift.
*               Compare the expected TDO with the captured TDO using the
*               prespecified XTDOMASK.
* Parameters:   pXsvfInfo   - XSVF information pointer.
* Returns:      int         - 0 = success;  non-zero = error.
*****************************************************************************/
int xsvfDoXSDRTDO( SXsvfInfo* pXsvfInfo )
{
    int iErrorCode;
    iErrorCode  = xsvfBasicXSDRTDO( &(pXsvfInfo->ucTapState),
                                    pXsvfInfo->lShiftLengthBits,
                                    pXsvfInfo->sShiftLengthBytes,
                                    &(pXsvfInfo->lvTdi),
                                    &(pXsvfInfo->lvTdoCaptured),
                                    &(pXsvfInfo->lvTdoExpected),
                                    &(pXsvfInfo->lvTdoMask),
                                    pXsvfInfo->ucEndDR,
                                    pXsvfInfo->lRunTestTime,
                                    pXsvfInfo->ucMaxRepeat );
    if ( iErrorCode != XSVF_ERROR_NONE )
    {
        pXsvfInfo->iErrorCode   = iErrorCode;
    }
    return( iErrorCode );
}

/*****************************************************************************
* Function:     xsvfDoXSDRBCE
* Description:  XSDRB/XSDRC/XSDRE <lenVal.TDI[XSDRSIZE]>
*               If not already in SHIFTDR, goto SHIFTDR.
*               Shift the given TDI data into the JTAG scan chain.
*               Ignore TDO.
*               If cmd==XSDRE, then goto ENDDR.  Otherwise, stay in ShiftDR.
*               XSDRB, XSDRC, and XSDRE are the same implementation.
* Parameters:   pXsvfInfo   - XSVF information pointer.
* Returns:      int         - 0 = success;  non-zero = error.
*****************************************************************************/
int xsvfDoXSDRBCE( SXsvfInfo* pXsvfInfo )
{
    unsigned char   ucEndDR;
    int             iErrorCode;
    ucEndDR = (unsigned char)(( pXsvfInfo->ucCommand == XSDRE ) ?
                                pXsvfInfo->ucEndDR : XTAPSTATE_SHIFTDR);
    iErrorCode  = xsvfBasicXSDRTDO( &(pXsvfInfo->ucTapState),
                                    pXsvfInfo->lShiftLengthBits,
                                    pXsvfInfo->sShiftLengthBytes,
                                    &(pXsvfInfo->lvTdi),
                                    /*plvTdoCaptured*/0, /*plvTdoExpected*/0,
                                    /*plvTdoMask*/0, ucEndDR,
                                    /*lRunTestTime*/0, /*ucMaxRepeat*/0 );
    if ( iErrorCode != XSVF_ERROR_NONE )
    {
        pXsvfInfo->iErrorCode   = iErrorCode;
    }
    return( iErrorCode );
}

/*****************************************************************************
* Function:     xsvfDoXSDRTDOBCE
* Description:  XSDRB/XSDRC/XSDRE <lenVal.TDI[XSDRSIZE]> <lenVal.TDO[XSDRSIZE]>
*               If not already in SHIFTDR, goto SHIFTDR.
*               Shift the given TDI data into the JTAG scan chain.
*               Compare TDO, but do NOT use XTDOMASK.
*               If cmd==XSDRTDOE, then goto ENDDR.  Otherwise, stay in ShiftDR.
*               XSDRTDOB, XSDRTDOC, and XSDRTDOE are the same implementation.
* Parameters:   pXsvfInfo   - XSVF information pointer.
* Returns:      int         - 0 = success;  non-zero = error.
*****************************************************************************/
int xsvfDoXSDRTDOBCE( SXsvfInfo* pXsvfInfo )
{
    unsigned char   ucEndDR;
    int             iErrorCode;
    ucEndDR = (unsigned char)(( pXsvfInfo->ucCommand == XSDRTDOE ) ?
                                pXsvfInfo->ucEndDR : XTAPSTATE_SHIFTDR);
    iErrorCode  = xsvfBasicXSDRTDO( &(pXsvfInfo->ucTapState),
                                    pXsvfInfo->lShiftLengthBits,
                                    pXsvfInfo->sShiftLengthBytes,
                                    &(pXsvfInfo->lvTdi),
                                    &(pXsvfInfo->lvTdoCaptured),
                                    &(pXsvfInfo->lvTdoExpected),
                                    /*plvTdoMask*/0, ucEndDR,
                                    /*lRunTestTime*/0, /*ucMaxRepeat*/0 );
    if ( iErrorCode != XSVF_ERROR_NONE )
    {
        pXsvfInfo->iErrorCode   = iErrorCode;
    }
    return( iErrorCode );
}

/*****************************************************************************
* Function:     xsvfDoXSTATE
* Description:  XSTATE <byte>
*               <byte> == XTAPSTATE;
*               Get the state parameter and transition the TAP to that state.
* Parameters:   pXsvfInfo   - XSVF information pointer.
* Returns:      int         - 0 = success;  non-zero = error.
*****************************************************************************/
int xsvfDoXSTATE( SXsvfInfo* pXsvfInfo )
{
    unsigned char   ucNextState;
    int             iErrorCode;
    readByte( &ucNextState );
    iErrorCode  = xsvfGotoTapState( &(pXsvfInfo->ucTapState), ucNextState );
    if ( iErrorCode != XSVF_ERROR_NONE )
    {
        pXsvfInfo->iErrorCode   = iErrorCode;
    }
    return( iErrorCode );
}

/*****************************************************************************
* Function:     xsvfDoXENDXR
* Description:  XENDIR/XENDDR <byte>
*               <byte>:  0 = RUNTEST;  1 = PAUSE.
*               Get the prespecified XENDIR or XENDDR.
*               Both XENDIR and XENDDR use the same implementation.
* Parameters:   pXsvfInfo   - XSVF information pointer.
* Returns:      int         - 0 = success;  non-zero = error.
*****************************************************************************/
int xsvfDoXENDXR( SXsvfInfo* pXsvfInfo )
{
    int             iErrorCode;
    unsigned char   ucEndState;

    iErrorCode  = XSVF_ERROR_NONE;
    readByte( &ucEndState );
    if ( ( ucEndState != XENDXR_RUNTEST ) && ( ucEndState != XENDXR_PAUSE ) )
    {
        iErrorCode  = XSVF_ERROR_ILLEGALSTATE;
    }
    else
    {

    if ( pXsvfInfo->ucCommand == XENDIR )
    {
            if ( ucEndState == XENDXR_RUNTEST )
            {
                pXsvfInfo->ucEndIR  = XTAPSTATE_RUNTEST;
            }
            else
            {
                pXsvfInfo->ucEndIR  = XTAPSTATE_PAUSEIR;
            }
            XSVFDBG_PRINTF1( 3, "   ENDIR State = %s\n",
                             xsvf_pzTapState[ pXsvfInfo->ucEndIR ] );
        }
    else    /* XENDDR */
    {
            if ( ucEndState == XENDXR_RUNTEST )
            {
                pXsvfInfo->ucEndDR  = XTAPSTATE_RUNTEST;
            }
    else
    {
                pXsvfInfo->ucEndDR  = XTAPSTATE_PAUSEDR;
            }
            XSVFDBG_PRINTF1( 3, "   ENDDR State = %s\n",
                             xsvf_pzTapState[ pXsvfInfo->ucEndDR ] );
        }
    }

    if ( iErrorCode != XSVF_ERROR_NONE )
    {
        pXsvfInfo->iErrorCode   = iErrorCode;
    }
    return( iErrorCode );
}

/*****************************************************************************
* Function:     xsvfDoXCOMMENT
* Description:  XCOMMENT <text string ending in \0>
*               <text string ending in \0> == text comment;
*               Arbitrary comment embedded in the XSVF.
* Parameters:   pXsvfInfo   - XSVF information pointer.
* Returns:      int         - 0 = success;  non-zero = error.
*****************************************************************************/
int xsvfDoXCOMMENT( SXsvfInfo* pXsvfInfo )
{
    /* Use the comment for debugging */
    /* Otherwise, read through the comment to the end '\0' and ignore */
    unsigned char   ucText;

    if ( xsvf_iDebugLevel > 0 )
    {
        putchar( ' ' );
    }

    do
    {
        readByte( &ucText );
        if ( xsvf_iDebugLevel > 0 )
        {
            putchar( ucText ? ucText : '\n' );
        }
    } while ( ucText );

    pXsvfInfo->iErrorCode   = XSVF_ERROR_NONE;

    return( pXsvfInfo->iErrorCode );
}

/*****************************************************************************
* Function:     xsvfDoXWAIT
* Description:  XWAIT <wait_state> <end_state> <wait_time>
*               If not already in <wait_state>, then go to <wait_state>.
*               Wait in <wait_state> for <wait_time> microseconds.
*               Finally, if not already in <end_state>, then goto <end_state>.
* Parameters:   pXsvfInfo   - XSVF information pointer.
* Returns:      int         - 0 = success;  non-zero = error.
*****************************************************************************/
int xsvfDoXWAIT( SXsvfInfo* pXsvfInfo )
{
    unsigned char   ucWaitState;
    unsigned char   ucEndState;
    long            lWaitTime;

    /* Get Parameters */
    /* <wait_state> */
    readVal( &(pXsvfInfo->lvTdi), 1 );
    ucWaitState = pXsvfInfo->lvTdi.val[0];

    /* <end_state> */
    readVal( &(pXsvfInfo->lvTdi), 1 );
    ucEndState = pXsvfInfo->lvTdi.val[0];

    /* <wait_time> */
    readVal( &(pXsvfInfo->lvTdi), 4 );
    lWaitTime = value( &(pXsvfInfo->lvTdi) );
    XSVFDBG_PRINTF2( 3, "   XWAIT:  state = %s; time = %ld\n",
                     xsvf_pzTapState[ ucWaitState ], lWaitTime );

    /* If not already in <wait_state>, go to <wait_state> */
    if ( pXsvfInfo->ucTapState != ucWaitState )
    {
        xsvfGotoTapState( &(pXsvfInfo->ucTapState), ucWaitState );
    }

    /* Wait for <wait_time> microseconds */
    waitTime( lWaitTime );

    /* If not already in <end_state>, go to <end_state> */
    if ( pXsvfInfo->ucTapState != ucEndState )
    {
        xsvfGotoTapState( &(pXsvfInfo->ucTapState), ucEndState );
    }

    return( XSVF_ERROR_NONE );
}


/*============================================================================
* Execution Control Functions
============================================================================*/

/*****************************************************************************
* Function:     xsvfInitialize
* Description:  Initialize the xsvf player.
*               Call this before running the player to initialize the data
*               in the SXsvfInfo struct.
*               xsvfCleanup is called to clean up the data in SXsvfInfo
*               after the XSVF is played.
* Parameters:   pXsvfInfo   - ptr to the XSVF information.
* Returns:      int - 0 = success; otherwise error.
*****************************************************************************/
int xsvfInitialize( SXsvfInfo* pXsvfInfo )
{
    /* Initialize values */
    pXsvfInfo->iErrorCode   = xsvfInfoInit( pXsvfInfo );

    if ( !pXsvfInfo->iErrorCode )
    {
        /* Initialize the TAPs */
        pXsvfInfo->iErrorCode   = xsvfGotoTapState( &(pXsvfInfo->ucTapState),
                                                    XTAPSTATE_RESET );
    }

    return( pXsvfInfo->iErrorCode );
}

/*****************************************************************************
* Function:     xsvfRun
* Description:  Run the xsvf player for a single command and return.
*               First, call xsvfInitialize.
*               Then, repeatedly call this function until an error is detected
*               or until the pXsvfInfo->ucComplete variable is non-zero.
*               Finally, call xsvfCleanup to cleanup any remnants.
* Parameters:   pXsvfInfo   - ptr to the XSVF information.
* Returns:      int         - 0 = success; otherwise error.
*****************************************************************************/
int xsvfRun( SXsvfInfo* pXsvfInfo )
{
    /* Process the XSVF commands */
    if ( (!pXsvfInfo->iErrorCode) && (!pXsvfInfo->ucComplete) )
    {
        /* read 1 byte for the instruction */
        readByte( &(pXsvfInfo->ucCommand) );
        ++(pXsvfInfo->lCommandCount);

        if ( pXsvfInfo->ucCommand < XLASTCMD )
        {
            /* Execute the command.  Func sets error code. */
            XSVFDBG_PRINTF1( 2, "  %s\n",
                             xsvf_pzCommandName[pXsvfInfo->ucCommand] );
            /* If your compiler cannot take this form,
               then convert to a switch statement */
            xsvf_pfDoCmd[ pXsvfInfo->ucCommand ]( pXsvfInfo );
        }
        else
        {
            /* Illegal command value.  Func sets error code. */
            xsvfDoIllegalCmd( pXsvfInfo );
        }
    }

    return( pXsvfInfo->iErrorCode );
}

/*============================================================================
* xsvfExecute() - The primary entry point to the XSVF player
============================================================================*/

/*****************************************************************************
* Function:     xsvfExecute
* Description:  Process, interpret, and apply the XSVF commands.
*               See port.c:readByte for source of XSVF data.
* Parameters:   none.
* Returns:      int - Legacy result values:  1 == success;  0 == failed.
*****************************************************************************/
int hal_FB_xsvfExecute(int *p, int nbytes)
{
    SXsvfInfo   xsvfInfo;

    xsvfReadInit(p, nbytes);
    xsvfInitialize( &xsvfInfo );

    while ( !xsvfInfo.iErrorCode && (!xsvfInfo.ucComplete) )
    {
        xsvfRun( &xsvfInfo );
    }

    if ( xsvfInfo.iErrorCode )
    {
        XSVFDBG_PRINTF1( 0, "%s\n", xsvf_pzErrorName[
                         ( xsvfInfo.iErrorCode < XSVF_ERROR_LAST )
                         ? xsvfInfo.iErrorCode : XSVF_ERROR_UNKNOWN ] );
        XSVFDBG_PRINTF2( 0, "ERROR at or near XSVF command #%ld.  See line #%ld in the XSVF ASCII file.\n",
                         xsvfInfo.lCommandCount, xsvfInfo.lCommandCount );
    }
    else
    {
        XSVFDBG_PRINTF( 0, "SUCCESS - Completed XSVF execution.\n" );
    }

    return( XSVF_ERRORCODE(xsvfInfo.iErrorCode) );
}
