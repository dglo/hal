#ifndef DOM_MB_DOMAPP_FPGA_INCLUDE
#define DOM_MB_DOMAPP_FPGA_INCLUDE

/**
 * \file DOM_MB_domapp.h
 *
 * $Revision: 1.1 $
 * $Author: arthur $
 * $Date: 2004-11-02 23:17:21 $
 *
 * \b Usage:
 * \code
#include "hal/DOM_MB_domapp.h"
* \endcode
*
* DOM main board hardware access library interface
*
*/
#include "hal/DOM_MB_types.h"

/**
 * get local clock readout...
 *
 * \returns the local clock readout
 */
unsigned long long
hal_FPGA_DOMAPP_get_local_clock(void);


#endif
