/** 
 *
 * \mainpage DOM Main Board Hardware Access Library (HAL) Interface
 * \section scope Scope
 *
 * This document describes the C language software interface used in 
 * accessing a DOM MB hal library.  It is anticipated that several versions 
 * of this library will be written to cover different program and platform 
 * execution environments.  One version of this library will target 
 * simulations of the DOM MB environment.  A second version will be written 
 * to execute on the 
 * actual DOM MB platform.  Furthermore, the behavior of each library version 
 * will depend on FPGA features present, or simulated, in a particular DOM MB 
 * environment.  In all cases, all entry points described in this document 
 * will be present and can be called without concern for unexpected program 
 * side effects or crashes.  Since some versions of this library are 
 * intended to execute in a simulation environment, some of the interfaces 
 * described below will return "fictitious" data when used in such an 
 * environment.  
 *
 * \section deps PLD and FPGA dependencies
 * Once debugged, the PLD program will be installed on all DOM MBs and can be 
 * assumed to be present during execution of all DOM MB programs.  Therefore, 
 * all PLD-related calls described below will function properly in all 
 * versions of this library - noting that, when using version of this 
 * library targeted at a simulation environment, program behavior will 
 * "reasonable" but values will be "fictitious".  
 *
 * This is not necessarily true for all FPGA-related routines.  At present, at 
 * least two major versions of the FPGA program are anticipated.  One 
 * targeted at low level tests and a second targeted at data acquisition 
 * operations.  While some FPGA features will be present in all FPGA 
 * programs, some portions of the design are mutually exclusive between 
 * these two versions.  This is particularly true of ATWD and flashADC 
 * related functions.  
 *
 *    Therefore, some of the FPGA-related calls in this library will depend on 
 * which FPGA configuration has been loaded into the DOM MB prior to program 
 * execution or, in the case of a DOM MB simulation, which version of the hal 
 * library has been linked with the simulation program.  Regardless of these 
 * version differences, the code implementing these hal library interfaces 
 * will be designed so that reliable program execution will not be 
 * compromised.  But, it will be the responsibility of programs using 
 * these interfaces to determine what FPGA-related operations are possible.
 *
 * \section naming Naming Conventions
 *
 * FPGA-related hal entry points that depend on a particular FPGA version in 
 * order to operate properly will be indicated by pre-pending a special tag to 
 * their name.  Since we only anticipate two such versions, these tags will be 
 * "FPGA_TEST_" and "FPGA_DAQ_".  Thus, a call that reads out test ATWD data 
 * from a fifo present in the test version of the FPGA program would be 
 * called:
 *
 * USHORT  *FPGA_TEST_atwd_read_fifo();
 * And, a call that configures the behavior of the ATWD lookback memory data 
 * moving state machine might be called:
 * void   FPGA_DAQ_set_dataEngine_state();
 *
 * \section capvec Capability Vector
 * TBD
 */
#ifndef DOM_MB_HAL_INCLUDE
#define DOM_MB_HAL_INCLUDE

/**
 * \file DOM_MB_hal.h
 *
 * $Revision: 1.13 $
 * $Author: jkelley $
 * $Date: 2004-03-10 23:40:08 $
 *
 * \b Usage:
 * \code
#include "hal/DOM_MB_hal.h"
 * \endcode
 *
 * DOM main board hardware access library interface
 *
 */
#include "hal/DOM_MB_pld.h"
#include "hal/DOM_MB_fpga.h"
#include "hal/DOM_MB_fb.h"

#endif
