#ifndef DOM_MB_PLD_INCLUDE
#define DOM_MB_PLD_INCLUDE

/**
 * \file DOM_MB_pld.h
 *
 * $Revision: 1.32 $
 * $Author: arthur $
 * $Date: 2004-05-18 21:36:29 $
 *
 * \b Usage:
 * \code
#include "hal/DOM_MB_pld.h"
 * \endcode
 *
 * DOM main board hardware access library interface for the PLD device
 *
 */
#include "hal/DOM_MB_types.h"

/**
 * Routine to determine if current runtime hal library is designed to perform 
 * real operations on the hardware execution platform or is performing 
 * simulated operations.
 *
 * Errors: No error conditions apply.
 *
 * \return True indicates simulated operations.
 *
 */
BOOLEAN
halIsSimulationPlatform(void);

/**
 * This routine returns the version number of the firmware at the time
 * the hal was compiled.
 * The version number is incremented by one each time a new feature
 * is added or the api changes.
 *
 * Errors: No error conditions apply.
 *
 * \return library version number
 * \see halGetHWVersion
 * \see halGetBuild
 * \see halGetHWBuild
 *
 */
USHORT
halGetVersion(void);

/** 
 * This routine returns the version number of the firmware that
 * was loaded on the DOM hw
 *
 * \return firmware's api version number
 * \see halGetVersion
 * \see halGetBuild
 * \see halGetHWBuild
 */
USHORT
halGetHWVersion(void);

/**
 * This routine returns the build number of the firmware at the time
 * the hal was compiled.
 * The build number is incremented by one each time the firmware is
 * compiled.
 *
 * Errors: No error conditions apply.
 *
 * \return library version number
 * \see halGetHWBuild
 * \see halGetVersion
 * \see halGetHWVersion
 *
 */
USHORT
halGetBuild(void);

/** 
 * This routine returns the build number of the firmware that
 * was loaded on the DOM.
 *
 * \return firmware's api version number
 */
USHORT
halGetHWBuild(void);

/** 
 * This routine indicates whether a console is connected to the serial port 
 * of the DOM MB.  A true response indicates that a terminal device is 
 * connected to the serial port.  When used in conjunction with a hal 
 * simulation library, this call always returns false.
 *
 * This routine indicates whether the serial port input
 * jumper is selected on the DOM MB -- there may or
 * may not be a console attached to the serial port...
 *
 * \return true if serial port is selected (powered)
 */
BOOLEAN
halIsConsolePresent();

/**
 * Is the DSR line asserted?
 *
 * \return true if serial port DSR is asserted
 */
BOOLEAN
halIsSerialDSR();

/**
 * Is there receive data available?
 *
 * \return true if data can be read from serial port
 */
BOOLEAN
halIsSerialReceiveData();

/**
 * Is there room to transmit serial data?
 *
 * \return true if data can be written to the serial port
 */
BOOLEAN
halIsSerialTransmitData();

/**
 * require DSR for serial communications
 *
 * \see halDisableSerialDSR
 * \see halSerialDSRState
 */
void
halEnableSerialDSR();

/**
 * don't require DSR for serial communications
 *
 * \see halEnableSerialDSR
 * \see halSerialDSRState
 */
void
halDisableSerialDSR();

/**
 * do we require DSR for serial communications
 *
 * \see halEnableSerialDSR
 * \see halDisableSerialDSR
 */
BOOLEAN
halSerialDSRState();

/** 
 * This routine sets a hardware flag that determines Excalibur boot up 
 * operations after the next reset.  When set, the Excalibur will boot up 
 * from the boot section of the selected flash memory.  When this call is 
 * used in conjunction 
 * with a hal simulation library, the internal hal "flashboot" variable will 
 * retain the set state..
 *
 * \see halClrFlashBoot
 * \see halFlashBootState
 */
void
halSetFlashBoot();

/**
 * This routine clears a hardware flag that determines Excalibur boot up 
 * operations after the next reset.  When cleared, the Excalibur will boot 
 * up from its configuration memory.  When this call is used in conjunction 
 * with a hal simulation library, the internal hal "flashboot" variable will 
 * retain the cleared state.
 *
 * \see halSetFlashBoot
 * \see halFlashBootState
 */
void
halClrFlashBoot();

/**
 * This routine returns the state of the hardware flag that determines 
 * Excalibur boot up operations after the next reset.  When used in 
 * conjunction with a hal simulation library, the state of the internal 
 * "flashboot" variable will be returned.
 *
 * \see halSetFlashBoot
 * \see halClrFlashBoot
 *
 */
BOOLEAN
halFlashBootState();

/**
 * We can reboot into one of two bootloaders.  The config bootloader
 * is used when clrFlashBoot is the last command issued.  The flash
 * bootload is used when setFlashBoot was the last command issued.
 * The default power-up state is clrFlashBoot.
 *
 * \see halSetFlashBoot
 * \see halClrFlashBoot
 */
void 
halBoardReboot();

/**
 * This routine reads and returns a single ADC channel from the set of ADCs 
 * present 
 * on a DOM MB or simulation.  Defined channels are enumerated in 
 * "DOM_MC_hal.h".  Hal simulation libraries are free to return any value 
 * and may attempt to return "reasonable" values based on expected DOM MB 
 * operation.
 *
 * Errors: Requests to read undefined channels will return a value of all 
 * bits, 0xffff.
 *
 * \see DOM_HAL_NUM_ADC_CHANNELS
 */
USHORT
halReadADC(UBYTE channel);

/**
 * This routine reads and returns an ADC value for the PMT high
 * voltage base.
 * Hal simulation libraries are free to return any value 
 * and may attempt to return "reasonable" values based on expected DOM MB 
 * operation.
 */
USHORT
halReadBaseADC(void);

/**
 * This routine writes a value into the DAC channel specified.  Attempts to 
 * write values outside the maximum supported by the addressed DAC will 
 * result in maximum permissible value being written to the selected DAC 
 * channel.  Values written to each DAC channel will be stored, on a channel 
 * by channel basis, within the hal library.  DAC values less than zero
 * will be assigned zero.  Also, we wait a DAC specific amount of time for
 * the analog side of the DAC to settle, depending on the time constant
 * of the circuit it drives (this time is typically 1us but can be as high
 * as 25ms)
 *
 * Errors: Requests to write a value to an undefined DAC channel will result 
 * in no action taken.  No error indication will be given.
 *
 * \param channel channel number
 * \param value   value to write to DAC
 * \see halReadDAC
 * \see DOM_HAL_NUM_DAC_CHANNELS
 */
void
halWriteDAC(UBYTE channel, int value);

/**
 * This routine writes a value into the High Voltage Base DAC channel 
 * specified.  Attempts to 
 * write values outside the maximum supported by the addressed DAC will 
 * result in maximum permissible value being written to the selected DAC 
 * channel.
 *
 * \param value   value to write to DAC
 * \see halReadBaseDAC
 */
void
halWriteActiveBaseDAC(USHORT value);

/**
 * This routine writes a value into the High Voltage Base DAC channel 
 * specified.  Attempts to 
 * write values outside the maximum supported by the addressed DAC will 
 * result in maximum permissible value being written to the selected DAC 
 * channel.
 *
 * \param value   value to write to DAC
 * \see halReadBaseDAC
 */
void
halWritePassiveBaseDAC(USHORT value);

/**
 * This routine writes a value into the High Voltage Base DAC.
 * Attempts to 
 * write values outside the maximum supported by the addressed DAC will 
 * result in maximum permissible value being written to the selected DAC 
 * channel.
 *
 * \param value   value to write to HV base DAC
 * \see halReadBaseDAC
 */
void
halWriteBaseDAC(USHORT value);

/**
 * This routine enables the high voltage on the base.  This
 * is a separate enable from the ability to enable the base power
 * supply.
 *
 * \see halEnableBaseHV
 */
void
halDisableBaseHV(void);

/**
 * This routine disables the high voltage on the base.  This
 * is a separate enable from the ability to enable the base power
 * supply.
 *
 * \see halDisableBaseHV
 * \see halPowerUpBase
 * \see halPowerDownBase
 */
void
halEnableBaseHV(void);

/**
 * This routine returns the last value written to the specified DAC channel.  
 * Since no DACs support read back of set values, the returned value comes 
 * from local storage within the hal library.
 *
 * Errors: Attempts to read back DAC values from undefined channels will 
 * return a value of 0.  No other error indication will be returned.
 *
 * \see halWriteDAC
 * \see DOM_HAL_NUM_DAC_CHANNELS
 */
USHORT
halReadDAC(UBYTE channel);

/**
 * This routine returns the last value written to the high voltage
 * PMT base DAC.
 * Since no DACs support read back of set values, the returned value comes 
 * from local storage within the hal library.
 *
 * \see halWriteBaseDAC
 */
USHORT
halReadBaseDAC(void);

/**
 * This routine applies power to the analog barometer sensor located on the 
 * DOM MB.  Readout of the actual barometric pressure is accomplished
 * through readADC() calls to the appropriate channels.
 *
 * \see halDisableBarometer
 */
void
halEnableBarometer(void);

/**
 * This routine removes power from the analog barometer sensor located on 
 * the DOM MB.
 *
 * Errors: When powered down, readout values from this sensor are undefined.
 *
 * \see halEnableBarometer
 */
void
halDisableBarometer(void);

/**
 * This routine prepares to read a value from the DOM MB mounted 
 * temperature sensor.  This routine requires a matched call to
 * halFinishReadTemp.
 *
 * \see halReadTemp
 * \see halFinishReadTemp
 */
void
halStartReadTemp(void);

/**
 * This routine checks to see if a previous call to halStartReadTemp
 * had returned a value to the temperature sensor.
 *
 * \see halReadTemp
 * \see halStartReadTemp
 * \see halFinishReadTemp
 */
int
halReadTempDone(void);

/**
 * This routine reads a value from the DOM MB mounted temperature sensor.  
 * Calibration and interpretation of return values is not defined in this 
 * document.  This routine must be called after halStartReadTemp.  Normally,
 * one would use halReadTemp to readout the temperature sensor.
 *
 * \see halStartReadTemp
 * \see halReadTemp
 */
USHORT
halFinishReadTemp(void);

/**
 * This routine reads a value from the DOM MB mounted temperature sensor.  
 * Calibration and interpretation of return values is not defined in this 
 * document.  This routine is equivalent to:
 * halStartReadTemp();
 * halFinishReadTemp();
 *
 * \see halStartReadTemp
 * \see halFinishReadTemp
 */
USHORT
halReadTemp(void);

/**
 * This routine enables operation of the PMT high voltage power supply.  
 * Detailed behavior of the high voltage power supply is defined elsewhere.  
 * But, no voltage will be supplied to the PMT unless the power supply has 
 * been enabled.  By default, the base output voltage is disabled, it can
 * be enabled with halEnableBaseHV
 *
 * \see halPowerDownBase
 * \see halWriteBaseDAC
 * \see halReadBaseADC
 * \see halDisableBaseHV
 * \see halEnableBaseHV
 */
void
halPowerUpBase(void);

/**
 * This routine disables operation of the PMT high voltage power supply.  
 * Detailed descriptions of high voltage power supply operation appears 
 * elsewhere.
 *
 * \see halPowerUpBase
 * \see halWriteBaseDAC
 * \see halReadBaseADC
 * \see halDisableBaseHV
 * \see halEnableBaseHV
 */
void
halPowerDownBase(void);

/**
 * power up flasher board
 *
 * \see halDisableFlasher()
 * \see halFlasherState()
 */
void
halEnableFlasher();

/**
 * power down flasher board
 *
 * \see halEnableFlasher()
 * \see halFlasherState()
 */
void
halDisableFlasher();

/**
 * current state of flasher board power
 *
 * \see halEnableFlasher
 * \see halDisableFlasher
 */
BOOLEAN
halFlasherState();

/**
 * Enable flasher board JTAG programming.  Must also
 * enable FPGA control of flasher board JTAG ports.
 *
 * \see halDisableFlasherJTAG
 */
void
halEnableFlasherJTAG();

/**
 * Disable flasher board JTAG programming.  Must also
 * disable FPGA control of flasher board JTAG ports.
 *
 * \see halEnableFlasherJTAG
 */
void
halDisableFlasherJTAG();

/**
 * power up LED power supply
 *
 * \see halDisableLEDPS()
 * \deprecated This is an obsolete hardware interface.
 */
void
halEnableLEDPS();

/**
 * power down LED power supply
 *
 * \see halEnableLEDPS()
 * \deprecated This is an obsolete hardware interface.
 */
void
halDisableLEDPS();

/**
 * current state of led power supply
 *
 * \see halEnableLEDPS
 * \see halDisableLEDPS
 * \deprecated This is an obsolete hardware interface.
 */
BOOLEAN
halLEDPSState();

/**
 * step LED power supply up
 *
 * \see halStepDownLED
 * \deprecated This is an obsolete hardware interface.
 */
void 
halStepUpLED();

/**
 * step LED power supply down
 *
 * \see halStepUpLED
 * \deprecated This is an obsolete hardware interface.
 */
void 
halStepDownLED();

/**
 * This routine selects one (only) of eight possible analog input sources 
 * for the ATWD channel 0.  It also allows the disabling of all possible 
 * inputs.
 *
 * Errors: Requests for selection of an invalid input source result in no 
 * action taken.
 *
 * \param channel channel from DOM_HAL_MUX_INPUTS enum
 * \see DOM_HAL_MUX_INPUTS
 */
void
halSelectAnalogMuxInput(UBYTE channel);

/**
 * set swap flash memory chips...
 *
 * \see halClrSwapFlashChips
 */
void
halSetSwapFlashChips();

/**
 * clear swap flash memory chips
 *
 * \see halSetSwapFlashChips();
 */
void
halClrSwapFlashChips();

/**
 * are the flash chips swapped?
 *
 * \return true if flash chips are swapped
 * \see halSetSwapFlashChips();
 */
BOOLEAN
halSwapFlashChipsState();

/**
 * get the main board serial number (id)
 *
 * \return board id
 */
const char *
halGetBoardID();

/**
 * get the main board serial number (id) as a 48 bit number
 *
 * \return board id or 0 on error...
 */
unsigned long long
halGetBoardIDRaw(void);

/**
 * busy wait us microseconds.
 *
 * \param us microseconds to busy wait.
 */
void
halUSleep(int us);

/**
 * read high voltage base serial number
 *
 * \return serial number as a string or NULL on error...
 */
const char *
halHVSerial(void);

/**
 * read high voltage base serial number
 *
 * \return serial number as a long long or 0 on error...
 */
unsigned long long
halHVSerialRaw(void);

/**
 * check to see if fpga is loaded
 *
 * \return non-zero if fpga is loaded
 */
int
halIsFPGALoaded(void);

/**
 * check to see if input data is there
 *
 * \return non-zero if there is data to read on stdin
 */
int 
halIsInputData(void);

/**
 * number of dom dac chip select lines...
 */
#define DOM_HAL_NUM_DAC_CS 4

/**
 * number of dom dac channels
 */
#define DOM_HAL_NUM_DAC_CHANNELS (DOM_HAL_NUM_DAC_CS * 4)

/**
 * number of dom adc (slow) channels
 */
#define DOM_HAL_NUM_ADC_CHANNELS 2

/**
 * number of atwd mux inputs
 *
 * \see DOM_HAL_MUX_INPUTS
 * \see halSelectAnalogMuxInput
 */
#define DOM_HAL_NUM_MUX_INPUTS 8

/** 
 * atwd input multiplexor channels
 *
 * \see halSelectAnalogMuxInput
 */
typedef enum {
   /** Toyocom Oscillator Output (distorted sinusoid) */
   DOM_HAL_MUX_OSC_OUTPUT,

   /** 40 MHz square wave (attenuated and level shifted) */
   DOM_HAL_MUX_40MHZ_SQUARE,

   /** PMT LED current */
   DOM_HAL_MUX_PMT_LED_CURRENT,

   /** Flasher board LED current*/
   DOM_HAL_MUX_FLASHER_LED_CURRENT,

   /** Local Coincidence Signal (upper) */
   DOM_HAL_MUX_UPPER_LOCAL_COINCIDENCE,

   /** Local Coincidence Signal (lower) */
   DOM_HAL_MUX_LOWER_LOCAL_COINCIDENCE,

   /** Communications ADC input signal */
   DOM_HAL_MUX_COMM_ADC_INPUT,

   /** Front End Pulser sample */
   DOM_HAL_MUX_FE_PULSER
} DOM_HAL_MUX_INPUTS;

/**
 * These enums define the dac channels to
 * be sent to halWriteDAC in the channel arg
 *
 * \see halWriteDAC
 */
typedef enum {
   /* CS0 */
   /** ATWD0 trigger bias */
   DOM_HAL_DAC_ATWD0_TRIGGER_BIAS,
   /** ATWD0 upper ramp limit */
   DOM_HAL_DAC_ATWD0_RAMP_TOP,
   /** ATWD0 ramp rate */
   DOM_HAL_DAC_ATWD0_RAMP_RATE,
   /** analog voltage reference */
   DOM_HAL_DAC_ATWD_ANALOG_REF,

   /* CS1 */
   /** ATWD1 trigger bias */
   DOM_HAL_DAC_ATWD1_TRIGGER_BIAS,
   /** ATWD1 upper ramp limit */
   DOM_HAL_DAC_ATWD1_RAMP_TOP,
   /** ATWD1 ramp rate */
   DOM_HAL_DAC_ATWD1_RAMP_RATE,
   /** PMT front end pedestal */
   DOM_HAL_DAC_PMT_FE_PEDESTAL,

   /* CS2 */
   /** multiple SPE discriminator threshold */
   DOM_HAL_DAC_MULTIPLE_SPE_THRESH,
   /** single SPE discriminator threshold */
   DOM_HAL_DAC_SINGLE_SPE_THRESH,
   /** fast ADC reference (pedestal shift) */
   DOM_HAL_DAC_FAST_ADC_REF,
   /** internal pulser amplitude */
   DOM_HAL_DAC_INTERNAL_PULSER,

   /* CS3 */
   /** on-board LED brightness control */
   DOM_HAL_DAC_LED_BRIGHTNESS,
   /** front end amp lower clamp voltage -- CURRENTLY UNUSED */
   DOM_HAL_DAC_FE_AMP_LOWER_CLAMP,
   /** flasher board timing pulse offset voltage */
   DOM_HAL_DAC_FL_REF,
   /** Set the DC offset of the ATWD mux input */
   DOM_HAL_DAC_MUX_BIAS
} DOM_HAL_DAC_CHANNELS;

/**
 * These enums define the adc channels to
 * be sent to halReadADC as the channel argument.
 *
 * \see halReadADC
 */
typedef enum {
   /* CS0 */

   /** voltage sum node: -5+(3.3+5)*162/(100+162) volts */
   DOM_HAL_ADC_VOLTAGE_SUM,
   /** 5V power supply value = Reading 0+5*(10K/25K) volts */
   DOM_HAL_ADC_5V_POWER_SUPPLY,
   /** 
    * Pressure -- Value = 
    *  111.11*(DOM_HAL_ADC_5V_POWER_SUPPLY/DOM_HAL_ADC_PRESSURE) 
    */
   DOM_HAL_ADC_PRESSURE,
   /** 5V analog current monitor (10mV/mA) measured on 5V side of switcher */
   DOM_HAL_ADC_5V_CURRENT,
   /** 3.3V analog current monitor (10mV/mA) measured on 5V side of switcher */
   DOM_HAL_ADC_3_3V_CURRENT,
   /** 2.5V analog current monitor (10mV/mA) measured on 5V side of switcher */
   DOM_HAL_ADC_2_5V_CURRENT,
   /** 1.8V analog current monitor (1mV/mA) measured on 5V side of switcher */
   DOM_HAL_ADC_1_8V_CURRENT,
   /** -5V analog current monitor (10mV/mA) measured on 5V side of switcher */
   DOM_HAL_ADC_MINUS_5V_CURRENT,
   /** DISC-OneSPE */
   DOM_HAL_ADC_DISC_ONESPE,
   /** 1.8V analog voltage */
   DOM_HAL_ADC_1_8V_POWER_SUPPLY,
   /** 2.5V analog voltage */
   DOM_HAL_ADC_2_5V_POWER_SUPPLY,
   /** 3.3V analog voltage */
   DOM_HAL_ADC_3_3V_POWER_SUPPLY,
   /** DISC-MultiSPE */
   DOM_HAL_ADC_DISC_MULTISPE,
   /** FADC Reference */
   DOM_HAL_ADC_FADC_0_REF,
   /** Single LED-HV */
   DOM_HAL_ADC_SINGLELED_HV,
   /** DAC0 Channel A */
   DOM_HAL_ADC_ATWDA_TRIGGER_BIAS_CURRENT,
   /** DAC0 Channel B */
   DOM_HAL_ADC_ATWDA_RAMP_TOP_VOLTAGE,
   /** DAC0 Channel C */
   DOM_HAL_ADC_ATWDA_RAMP_BIAS_CURRENT,
   /** Analog Reference */
   DOM_HAL_ADC_ANALOG_REF,
   /** DAC1 Channel A */
   DOM_HAL_ADC_ATWDB_TRIGGER_BIAS_CURRENT,
   /** DAC1 Channel B */
   DOM_HAL_ADC_ATWDB_RAMP_TOP_VOLTAGE,
   /** DAC1 Channel C */
   DOM_HAL_ADC_ATWDB_RAMP_BIAS_CURRENT,
   /** Pedestal Value */
   DOM_HAL_ADC_PEDESTAL,
   /** FE Test Pulse Amplifier */
   DOM_HAL_ADC_FE_TEST_PULSE_AMPL
   

} DOM_HAL_ADC_CHANNELS;



#endif
