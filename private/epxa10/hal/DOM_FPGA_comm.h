#ifndef DOM_FPGA_COMM_INCLUDE
#define DOM_FPGA_COMM_INCLUDE

/**
 * \defgroup fpga_test_com_ctrl Communications control
 * \ingroup fpga_test_regs
 *
 * \brief This register contains control for the
 * dom communications channel. bits 8-15 are dudt,
 * bits, 16-25 are CAL_THR -- the calibration threshold.
 */
/*@{*/
/** register addresss */
#define DOM_FPGA_COMM_CTRL (0x90000500)
/** Request a reboot from com firmware */
#define DOM_FPGA_COMM_CTRL_REBOOT_REQUEST     0x00000001
/*@}*/

/**
 * \defgroup fpga_test_com_status Communications status
 * \ingroup fpga_test_regs
 *
 * \brief This register contains status information for the
 * dom communications channel.
 */
/*@{*/
/** register addresss */
#define DOM_FPGA_COMM_STATUS (0x90000504)
/** com firmware grants request for reboot */
#define DOM_FPGA_COMM_STATUS_REBOOT_GRANTED    0x00000001
/** for debugging hardware... */
#define DOM_FPGA_COMM_STATUS_TX_PKT_SENT       0x00000002
/** for debugging hardware */
#define DOM_FPGA_COMM_STATUS_TX_ALMOST_EMPTY   0x00000004
/** is there a packet in the dp mem?... */
#define DOM_FPGA_COMM_STATUS_RX_PKT_RCVD       0x00000008
/** for debugging hardware... */
#define DOM_FPGA_COMM_STATUS_COMM_RESET_RCVD   0x00000010
/** for debugging hardware... */
#define DOM_FPGA_COMM_STATUS_RX_DP_ALMOST_FULL 0x00000020
/** is the comm firmware available? */
#define DOM_FPGA_COMM_STATUS_AVAIL             0x00000040
/*@}*/

/**
 * \defgroup fpga_comm FPGA communications dual ported memory interface.
 * \ingroup fpga_test_regs
 *
 * \brief These registers point into dual ported memory to access the
 * communications data.
 */
/*@{*/
/** 32 bit word pointer to one word past data to send off... */
#define DOM_FPGA_COMM_TX_DPR_WADR (0x90000508)
/** 32 bit word pointer to current tx packet processed... */
#define DOM_FPGA_COMM_TX_DPR_RADR (0x9000050C)
/** 32 bit word pointer to current rx read address... */
#define DOM_FPGA_COMM_RX_DPR_RADR (0x90000510)
/** 32 bit word pointer to current rx processing address... */
#define DOM_FPGA_COMM_RX_ADDR     (0x90000514)
/** rx crc error count... */
#define DOM_FPGA_COMM_ERRORS      (0x90000518)
/*@}*/

/**
 * \defgroup fpga_comm_clev comm level adaption
 * \ingroup fpga_test_regs
 *
 * \brief access the communication level adaption min and max values
 * min: bits 0..9
 * max: bits 16..25
 */
/*@{*/
#define DOM_FPGA_COMM_CLEV    (0x90000520)
/*@}*/

/**
 * \defgroup fpga_comm_thr_del comm threshold and delays
 * \ingroup fpga_test_regs
 *
 * \brief access the communications thresholds and other comm parameters...
 * com_thr: 7..0 
 * dacmax: 9..8
 * rec_del: 23..16
 * send_del: 31..24
 */
/*@{*/
#define DOM_FPGA_COMM_THR_DEL (0x90000524)
/*@}*/

#endif








