#ifndef DOM_MB_PLD_INCLUDE
#define DOM_MB_PLD_INCLUDE

/**
 * \file DOM_MB_pld.h
 *
 * $Revision: 1.17 $
 * $Author: arthur $
 * $Date: 2003-07-11 15:02:41 $
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
halIsSimulationPlatform();

/** 
 * This routine returns the version number of the hal access library being 
 * used.
 *
 * Errors: No error conditions apply.
 *
 * \return library version number
 *
 */
USHORT
halGetVersion();

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
 * by channel basis, within the hal library.
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
halWriteDAC(UBYTE channel, USHORT value);

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
halEnableBarometer();

/**
 * This routine removes power from the analog barometer sensor located on 
 * the DOM MB.
 *
 * Errors: When powered down, readout values from this sensor are undefined.
 *
 * \see halEnableBarometer
 */
void
halDisableBarometer();

/**
 * This routine reads a value from the DOM MB mounted temperature sensor.  
 * Calibration and interpretation of return values is not defined in this 
 * document.
 *
 */
USHORT
halReadTemp();

/**
 * This routine enables operation of the PMT high voltage power supply.  
 * Detailed behavior of the high voltage power supply is defined elsewhere.  
 * But, no voltage will be supplied to the PMT unless the power supply has 
 * been enabled.
 *
 * \see halDisablePMT_HV
 * \see halSetPMT_HV
 * \see halReadPMT_HV
 */
void
halEnablePMT_HV();

/**
 * This routine disables operation of the PMT high voltage power supply.  
 * Detailed descriptions of high voltage power supply operation appears 
 * elsewhere.
 *
 * \see halEnablePMT_HV
 * \see halSetPMT_HV
 * \see halReadPMT_HV
 */
void
halDisablePMT_HV();

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
 * This routine sets the target output value of the PMT high voltage power 
 * supply.  Calibrated translation of digital values into power supply 
 * output voltages is not part of this interface and will be described 
 * elsewhere.  
 *
 * Errors: Attempts to set the target output value to a value in excess of the 
 * maximum set value will result in NO ACTION BEING TAKEN.  Unlike the 
 * behavior of the writeDAC() interface, errors of this sort are assumed 
 * to indicate incorrect program behavior and are considered invalid requests.
 *
 * \see halEnablePMT_HV
 * \see halDisablePMT_HV
 * \see halReadPMT_HV
 */
void
halSetPMT_HV(USHORT value);

/** 
 * This routine reads the current output value of the PMT high voltage power 
 * supply.  Calibrated translation of this value into power supply output 
 * voltage is not part of this interface and will be described elsewhere.
 *
 * \see halEnablePMT_HV
 * \see halDisablePMT_HV
 * \see halSetPMT_HV
 */
USHORT
halReadPMT_HV();

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
 * check to see if fpga is loaded
 *
 * \return non-zero if fpga is loaded
 */
int 
halIsFPGALoaded(void);

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
 * current version number of this library...
 */
#define DOM_HAL_VERSION 1

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
   /** on-board LED brightness control */
   DOM_HAL_DAC_LED_BRIGHTNESS,
   /** fast ADC reference (pedestal shift) */
   DOM_HAL_DAC_FAST_ADC_REF,

   /* CS3 */
   /** internal pulser amplitude */
   DOM_HAL_DAC_INTERNAL_PULSER,
   /** front end amp lower clamp voltage */
   DOM_HAL_DAC_FE_AMP_LOWER_CLAMP,
   /** spare 10 bit ADC output 0 */
   DOM_HAL_DAC_SPARE_ADC0,
   /** spare 10 bit ADC output 1 */
   DOM_HAL_DAC_SPARE_ADC1
} DOM_HAL_DAC_CHANNELS;

/**
 * These enums define the adc channels to
 * be sent to halReadADC as the channel argument.
 *
 * \see halReadADC
 */
typedef enum {
   /* CS0 */

   /** voltage sum node: -(-5/2.5)-(+3.3+2.5+1.8)/4 volts */
   DOM_HAL_ADC_VOLTAGE_SUM,
   /** 5V power supply value = Reading (2.5/4095)*(10K/34.9K) */
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
} DOM_HAL_ADC_CHANNELS;



#endif
