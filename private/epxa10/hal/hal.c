/**
 * \file emu.c, emulate the dom hal.  Should be useful for
 * both the eval board and a standalone emulation dom mb 
 * emulation program
 */
#include "DOM_MB_hal.h"
#include "DOM_PLD_regs.h"

BOOLEAN isSimulationPlatform() { return 0; }
USHORT getHalVersion() { return DOM_HAL_VERSION; }
BOOLEAN isConsolePresent() { return RPLDBIT(UART_STATUS, SERIAL_POWER) != 0; }

void setFlashBoot() {
   PLD(BOOT_CONTROL) =
      DOM_PLD_BOOT_CONTROL_BOOT_FROM_FLASH | 
      RPLDBIT(BOOT_STATUS, ALTERNATE_FLASH);
}

void clrFlashBoot() { 
   PLD(BOOT_CONTROL) = RPLDBIT(BOOT_STATUS, ALTERNATE_FLASH);
}

BOOLEAN flashBootState() { 
   return RPLDBIT(BOOT_STATUS, BOOT_FROM_FLASH)!=0;
}


USHORT readADC(UBYTE channel) {
   int ret;
   
   /* FIXME: eventually we'll need a mutex here...
    */
   PLD(SPI_CHIP_SELECT0) = (1<<(5+channel));

   /* FIXME: talk to adc chip here...
    */
   ret = 1234;
   
   PLD(SPI_CHIP_SELECT0) = 0;
   return ret; 
}

static USHORT daclookup[DOM_HAL_NUM_DAC_CHANNELS];

void writeDAC(UBYTE channel, USHORT value) {
   if (channel>=DOM_HAL_NUM_DAC_CHANNELS) return;
   daclookup[channel] = value;
   /* now clock it out...
    */
   PLD(SPI_CHIP_SELECT0) = 1<<channel;

   /* FIXME: clock out dac value here...
    */
   
   PLD(SPI_CHIP_SELECT0) = 0;
}

USHORT readDAC(UBYTE channel) {
   if (channel>=DOM_HAL_NUM_DAC_CHANNELS) return 0;
   return daclookup[channel];
}

void enableBarometer() {
   PLD(SYSTEM_CONTROL) = 
      PLDBIT(SYSTEM_CONTROL, BAROMETER_ENABLE) |
      RPLDBIT2(SYSTEM_STATUS, HV_PS_ENABLE, FLASHER_ENABLE);
}

void disableBarometer() {
   PLD(SYSTEM_CONTROL) = 
      RPLDBIT2(SYSTEM_STATUS, HV_PS_ENABLE, FLASHER_ENABLE);
}

void enablePMT_HV() { 
   PLD(SYSTEM_CONTROL) = 
      PLDBIT(SYSTEM_CONTROL, HV_PS_ENABLE) |
      RPLDBIT(SYSTEM_STATUS, FLASHER_ENABLE);
}

void disablePMT_HV() { 
   PLD(SYSTEM_CONTROL) = 
      PLDBIT(SYSTEM_CONTROL, HV_PS_ENABLE) |
      RPLDBIT(SYSTEM_STATUS, FLASHER_ENABLE);
}

USHORT readTemp() { 
   return 100; 
}


static USHORT pmtlookup;

void setPMT_HV(USHORT value) { 
   pmtlookup = value; 
}

USHORT readPMT_HV() { 
   return pmtlookup;
}

static UBYTE muxlookup;

static int muxchantoaddr(UBYTE chan) { return chan%4; }
static int muxchantochip(UBYTE chan) { return chan/4; }
void selectAnalogMuxInput(UBYTE chan) { 
   muxlookup = chan;
   PLD(MUX_CONTROL) = muxchantochip(chan)|(muxchantoaddr(chan)<<2);
}

/**
 * FIXME for Gerry:
 *  can we read PS_ENABLE (LED power supply enable)
 *  can we read BAROMETER_ENABLE?
 *  what is PLL_S[01] in MUX_CONTROL?
 */


