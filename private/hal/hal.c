/**
 * \file emu.c, emulate the dom hal.  Should be useful for
 * both the eval board and a standalone emulation dom mb 
 * emulation program
 */
//#include <stdio.h>
#include "hal/DOM_MB_hal_simul.h"

BOOLEAN halIsSimulationPlatform() { return 1; }
USHORT halGetHalVersion() { return 1; }
BOOLEAN halIsConsolePresent() { return 0; }

static BOOLEAN flashboot = 0;

void halSetFlashBoot() { flashboot = 1; }
void halClrFlashBoot() { flashboot = 0; }

BOOLEAN halFlashBootState() { return flashboot; }
USHORT halReadADC(UBYTE channel) { return 1234; }

static USHORT daclookup[DOM_HAL_NUM_DAC_CHANNELS];
void halWriteDAC(UBYTE channel, USHORT value) {
   if (channel>=DOM_HAL_NUM_DAC_CHANNELS) return;
   daclookup[channel] = value;
}

USHORT halReadDAC(UBYTE channel) {
   if (channel>=DOM_HAL_NUM_DAC_CHANNELS) return 0;
   return daclookup[channel];
}

void halEnableBarometer() {;}
void halDisableBarometer() {;}
USHORT halReadTemp() { return 100; }

void halEnablePMT_HV() { ; }
void halDisablePMT_HV() { ; }

static USHORT pmtlookup;

void halSetPMT_HV(USHORT value) { pmtlookup = value; }
USHORT halReadPMT_HV() { return pmtlookup; }

static UBYTE muxlookup;

void halSelectAnalogMuxInput(UBYTE channel) { muxlookup = channel; }

static BOOLEAN swapflash = 0;

void halSetSwapFlashChips() { swapflash = 1; }
void halClrSwapFlashChips() { swapflash = 0; }
BOOLEAN halSwapFlashChipsState() { return swapflash; }

static BOOLEAN flasher = 0;
void halEnableFlasher() { flasher = 1; }
void halDisableFlasher() { flasher = 0; }
BOOLEAN halFlasherState() { return flasher; }

static BOOLEAN ledps = 0;
void halEnableLEDPS() { ledps = 1; }
void halDisableLEDPS() { ledps = 0; }
BOOLEAN halLEDPSState() { return ledps; }

void halStepUpLED() {}
void halStepDownLED() {}

BOOLEAN serialdsr = 0;

BOOLEAN halIsSerialDSR() { return 0; }
BOOLEAN halIsSerialReceiveData() { return 0; }
BOOLEAN halIsSerialTransmitData() { return 1; }
void halEnableSerialDSR() { serialdsr = 1; }
void halDisableSerialDSR() { serialdsr = 0; }
BOOLEAN halSerialDSRState() { return serialdsr; }

void halBoardReboot() { }

UBYTE boardID[6]={0,0,0,0,0,0};
char *halGetBoardID(void) {
    // make string static to keep it around after returning
    static char ID[13];
    int i;
    UBYTE b;
    for (i=0;i<6;i++) {
	b = ((boardID[i]>>4) & 0xf) + 0x30;
	if (b > 0x39) {
	    ID[i*2] = b + 0x7;
	}
	else {
	    ID[i*2]=b;
	}
	b = (boardID[i] & 0xf) + 0x30;
	if (b > 0x39) {
	    ID[i*2+1] = b + 0x7;
	} else {
	    ID[i*2+1]=b;
	}
    }
    // make sure we null terminate the string;
    ID[12]=0;

    return ID;
}
void halSetBoardID(char *ID) { 
    int i;
    UBYTE b;
    char temp[32];

    if(strlen(ID) < 12) {
	// ID is too short, make something up
	strcpy(temp, "123456123456");
    } else if(strlen(ID) > 12) {
	// ID is too long, make something up
	strcpy(temp, "123456123456");
    } else {
	strcpy(temp, ID);
    }


    for (i=0;i<6;i++) {
	b=(UBYTE)temp[i*2];

	// make sure its upper case.
	if(b > 0x60) {
	    b-=0x20;
	}
	// deal with hex digits
   	if(b > 0x40) {
	    b-=0x37;
	} else {
	    b-=0x30;
	}
	boardID[i]=b<<4;
    	
	b=(UBYTE)temp[i*2+1];
	if(b >0x60) {
	    b-=0x20;
	}
	if(b > 0x40) {
	    b-=0x37;
	} else {
	    b-=0x30;
	}
	boardID[i]+=b;
    }
}

char boardName[128]="none";
char *halGetBoardName(void) {return boardName; }
void halSetBoardName(char *name) { strcpy(boardName,name);}

















