/**
 * \file emu.c, emulate the dom hal.  Should be useful for
 * both the eval board and a standalone emulation dom mb 
 * emulation program
 */
#include "hal/DOM_MB_hal.h"

BOOLEAN isSimulationPlatform() { return 1; }
USHORT getHalVersion() { return 1; }
BOOLEAN isConsolePresent() { return 0; }

static BOOLEAN flashboot = 0;

void setFlashBoot() { flashboot = 1; }
void clrFlashBoot() { flashboot = 0; }

BOOLEAN flashBootState() { return flashboot; }
USHORT readADC(UBYTE channel) { return 1234; }

static USHORT daclookup[DOM_HAL_NUM_DAC_CHANNELS];
void writeDAC(UBYTE channel, USHORT value) {
   if (channel>=DOM_HAL_NUM_DAC_CHANNELS) return;
   daclookup[channel] = value;
}

USHORT readDAC(UBYTE channel) {
   if (channel>=DOM_HAL_NUM_DAC_CHANNELS) return 0;
   return daclookup[channel];
}

void enableBarometer() {;}
void disableBarometer() {;}
USHORT readTemp() { return 100; }

void enablePMT_HV() { ; }
void disablePMT_HV() { ; }

static USHORT pmtlookup;

void setPMT_HV(USHORT value) { pmtlookup = value; }
USHORT readPMT_HV() { return pmtlookup; }

static UBYTE muxlookup;

void selectAnalogMuxInput(UBYTE channel) { muxlookup = channel; }

static BOOLEAN swapflash = 0;

void setSwapFlashChips() { swapflash = 1; }
void clrSwapFlashChips() { swapflash = 0; }
BOOLEAN swapFlashChipsState() { return swapflash; }

static BOOLEAN flasher = 0;
void enableFlasher() { flasher = 1; }
void disableFlasher() { flasher = 0; }
BOOLEAN flasherState() { return flasher; }

static BOOLEAN ledps = 0;
void enableLEDPS() { ledps = 1; }
void disableLEDPS() { ledps = 0; }
BOOLEAN LEDPSState() { return ledps; }

void stepUpLED() {}
void stepDownLED() {}

BOOLEAN serialdsr = 0;

BOOLEAN isSerialDSR() { return 0; }
BOOLEAN isSerialReceiveData() { return 0; }
BOOLEAN isSerialTransmitData() { return 1; }
void enableSerialDSR() { serialdsr = 1; }
void disableSerialDSR() { serialdsr = 0; }
BOOLEAN serialDSRState() { return serialdsr; }

void boardReboot() { }

const char *getBoardID(void) { return "linux-sim"; }
