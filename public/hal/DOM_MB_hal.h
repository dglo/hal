/** 
 *
 * \mainpage DOM Main Board Hardware Access Library (HAL) Interface
 * \section scope Scope
 *
 *This document describes the C language software interface used in accessing a DOM MB hal library.  It is anticipated that several versions of this library will be written to cover different program and platform execution environments.  One version of this library will target simulations of the DOM MB environment.  A second version will be written to execute on the actual DOM MB platform.  Furthermore, the behavior of each library version will depend on FPGA features present, or simulated, in a particular DOM MB environment.  In all cases, all entry points described in this document will be present and can be called without concern for unexpected program side effects or crashes.  Since some versions of this library are intended to execute in a simulation environment, some of the interfaces described below will return "fictitious" data when used in such an environment.  
 *
 * \section deps PLD and FPGA dependencies
 * Once debugged, the PLD program will be installed on all DOM MBs and can be assumed to be present during execution of all DOM MB programs.  Therefore, all PLD-related calls described below will function properly in all versions of this library - noting that, when using version of this library targeted at a simulation environment, program behavior will "reasonable" but values will be "fictitious".  
 *
 *This is not necessarily true for all FPGA-related routines.  At present, at least two major versions of the FPGA program are anticipated.  One targeted at low level tests and a second targeted at data acquisition operations.  While some FPGA features will be present in all FPGA programs, some portions of the design are mutually exclusive between these two versions.  This is particularly true of ATWD and flashADC related functions.  
 *
 *    Therefore, some of the FPGA-related calls in this library will depend on which FPGA configuration has been loaded into the DOM MB prior to program execution or, in the case of a DOM MB simulation, which version of the hal library has been linked with the simulation program.  Regardless of these version differences, the code implementing these hal library interfaces will be designed so that reliable program execution will not be compromised.  But, it will be the responsibility of programs using these interfaces to determine what FPGA-related operations are possible.
 *
 * \section naming Naming Conventions
 *
 * FPGA-related hal entry points that depend on a particular FPGA version in order to operate properly will be indicated by pre-pending a special tag to their name.  Since we only anticipate two such versions, these tags will be "FPGA_TEST_" and "FPGA_DAQ_".  Thus, a call that reads out test ATWD data from a fifo present in the test version of the FPGA program would be called:
 *
 * USHORT  *FPGA_TEST_atwd_read_fifo();
 * And, a call that configures the behavior of the ATWD?lookback memory data moving state machine might be called:
 * void   FPGA_DAQ_set_dataEngine_state();
 *
 * \section capvec Capability Vector
 * TBD
 */


/**
 * \file DOM_MB_hal.h
 * 
 * DOM main board hardware access library interface
 */

/** 
 * \brief isSimulationPlatform
 *
 * Routine to determine if current runtime hal library is designed to perform 
 * real operations on the hardware execution platform or is performing simulated 
 * operations.
 *
 * Usage
 * \code
#include <DOM_MB_types.h>
#include <DOM_MB_hal.h>
 * \endcode
 *
 * Errors: No error conditions apply.
 *
 * \return True indicates simulated operations.
 *
 */
BOOLEAN
isSimulationPlatform();


/** 
 * \brief getHalVersion
 *
 * This routine returns the version number of the hal access library being used.
 *
 * Usage
 * \code
#include <DOM_MB_types.h>
#include <DOM_MB_hal.h>
 * \endcode
 *
 * Errors: No error conditions apply.
 *
 * \return library version number
 *
 */
USHORT
getHalVersion();

/** 
 * \brief isConsolePresent
 *
 * This routine indicates whether a console is connected to the serial port of the DOM MB.  A true response indicates that a terminal device is connected to the serial port.  When used in conjunction with a hal simulation library, this call always returns false.
 * 
 * Usage
 * \code
#include <DOM_MB_types.h>
#include <DOM_MB_hal.h>
 * \endcode
 *
 */
BOOLEAN
isConsolePresent();


/** 
 * \brief setFlashBoot
 *
 * This routine sets a hardware flag that determines Excalibur boot up operations after the next reset.  When set, the Excalibur will boot up from the boot section of the selected flash memory.  When this call is used in conjunction with a hal simulation library, the internal hal "flashboot" variable will retain the set state..
 *
 * Usage
\code
#include <DOM_MB_types.h>
#include <DOM_MB_hal.h>
\endcode
 *
 * \see clrFlashBoot
 * \see flashBootState
 */
void
setFlashBoot();

/**
 * \brief clrFlashBoot
 *
 * This routine clears a hardware flag that determines Excalibur boot up operations after the next reset.  When cleared, the Excalibur will boot up from its configuration memory.  When this call is used in conjunction with a hal simulation library, the internal hal "flashboot" variable will retain the cleared state.
 *
 * Usage
 * \code
#include <DOM_MB_types.h>
#include <DOM_MB_hal.h>
 * \endcode
 *
 * \see setFlashBoot
 * \see flashBootState
 *
 */
void
clrFlashBoot();

/**
 * \brief flashBootState
 *
 * This routine returns the state of the hardware flag that determines Excalibur boot up operations after the next reset.  When used in conjunction with a hal simulation library, the state of the internal "flashboot" variable will be returned.
 *
 * Usage
 *\code
#include <DOM_MB_types.h>
#include <DOM_MB_hal.h>
 *\endcode
 *
 * \see setFlashBoot
 * \see clrFlashBoot
 *
 */
BOOLEAN
flashBootState();

/**
 * \brief readADC
 *
 * This routine reads and returns a single ADC channel from the set of ADCs present on a DOM MB or simulation.  Defined channels are enumerated in "DOM_MC_hal.h".  Hal simulation libraries are free to return any value and may attempt to return "reasonable" values based on expected DOM MB operation.
 *
 * Usage
 *\code
#include <DOM_MB_types.h>
#include <DOM_MB_hal.h>
 *\endcode
 *
 * Errors: Requests to read undefined channels will return a value of all bits, 0xffff.
 */
USHORT
readADC(UBYTE channel);

/**
 * \brief writeDAC
 *
 * This routine writes a value into the DAC channel specified.  Attempts to write values outside the maximum supported by the addressed DAC will result in maximum permissible value being written to the selected DAC channel.  Values written to each DAC channel will be stored, on a channel by channel basis, within the hal library.
 *
 * Usage
 *\code
#include <DOM_MB_types.h>
#include <DOM_MB_hal.h>
 *\endcode
 *
 * Errors: Requests to write a value to an undefined DAC channel will result in no action taken.  No error indication will be given.
 *
 * \see readDAC
 */
void
writeDAC(UBYTE channel, USHORT value);

/**
 * \brief readDAC
 *
 * This routine returns the last value written to the specified DAC channel.  Since no DACs support read back of set values, the returned value comes from local storage within the hal library.
 *
 * Usage
 * \code
#include <DOM_MB_types.h>
#include <DOM_MB_hal.h>
 *\endcode
 *
 * Errors: Attempts to read back DAC values from undefined channels will return a value of 0.  No other error indication will be returned.
 *
 * \see writeDAC
 */
USHORT
ReadDAC(UBYTE channel);

/**
 * \brief enableBarometer
 *
 *This routine applies power to the analog barometer sensor located on the DOM MB.  Readout of the actual barometric pressure is accomplished through readADC() calls to the appropriate channels (see "DOM_MB_hal.h").
 *
 * Usage
 * \code
#include <DOM_MB_types.h>
#include <DOM_MB_hal.h>
 *\endcode
 *
 * \see disableBarometer
 */
void
enableBarometer();

/**
 * \brief disableBarometer
 *
 * This routine removes power from the analog barometer sensor located on the DOM MB.
 *
 * Usage
 *\code
#include <DOM_MB_types.h>
#include <DOM_MB_hal.h>
 *\endcode
 *
 * Errors: When powered down, readout values from this sensor are undefined.
 *
 * \see enableBarometer
 */
void
disableBarometer();

/**
 * \brief readTemp
 *
 * This routine reads a value from the DOM MB mounted temperature sensor.  Calibration and interpretation of return values is not defined in this document.
 *
 * Usage
 *\code
#include <DOM_MB_types.h>
#include <DOM_MB_hal.h>
 *\endcode
 *
 */
UnsigneShort
readTemp();

/**
 * \brief enablePMT_HV
 *
 * This routine enables operation of the PMT high voltage power supply.  Detailed behavior of the high voltage power supply is defined elsewhere.  But, no voltage will be supplied to the PMT unless the power supply has been enabled.
 *
 * Usage
 *\code
#include <DOM_MB_types.h>
#include <DOM_MB_hal.h>
 *\endcode
 *
 * \see disablePMT_HV
 * \see setPMT_HV
 * \see readPMT_HV
 */
void
enablePMT_HV();

/**
 * \brief disablePMT_HV
 *
 * This routine disables operation of the PMT high voltage power supply.  Detailed descriptions of high voltage power supply operation appears elsewhere.
 *
 * Usage
 *\code
#include <DOM_MB_types.h>
#include <DOM_MB_hal.h>
 *\endcode
 *
 * \see enablePMT_HV
 * \see setPMT_HV
 * \see readPMT_HV
 */
void
disablePMT_HV();

/**
 * \brief setPMT_HV
 *
 * This routine sets the target output value of the PMT high voltage power supply.  Calibrated translation of digital values into power supply output voltages is not part of this interface and will be described elsewhere.  
 *
 * Usage
 * \code
#include <DOM_MB_types.h>
#include <DOM_MB_hal.h>
 * \endcode
 *
 * Errors: Attempts to set the target output value to a value in excess of the maximum set value will result in NO ACTION BEING TAKEN.  Unlike the behavior of the writeDAC() interface, errors of this sort are assumed to indicate incorrect program behavior and are considered invalid requests.
 *
 * \see enablePMT_HV
 * \see disablePMT_HV
 * \see readPMT_HV
 */
USHORT
setPMT_HV(USHORT value);

/** 
 * \brief readPMT_HV
 *
 * This routine reads the current output value of the PMT high voltage power supply.  Calibrated translation of this value into power supply output voltage is not part of this interface and will be described elsewhere.
 *
 * Usage
 * \code
#include <DOM_MB_types.h>
#include <DOM_MB_hal.h>
 * \endcode
 *
 * \see enablePMT_HV
 * \see disablePMT_HV
 * \see setPMT_HV
 */
USHORT
readPMT_HV();

/**
 * \brief selectAnalogMuxInput
 *
 * This routine selects one (only) of eight possible analog input sources for the ATWD channel 0.  It also allows the disabling of all possible inputs.  Valid selection choices are listed and described in "DOM_MB_hal.h".  
 *
 * Usage
 * \code
#include <DOM_MB_types.h>
#include <DOM_MB_hal.h>
 * \endcode
 *
 * Errors: Requests for selection of an invalid input source result in no action taken.
 */
void
selectAnalogMuxInput(UBYTE channel);
