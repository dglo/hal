/**
 * \file DOM_FPGA_domapp_regs.h Description of the domapp fpga registers...
 */

/** 
 * Base address of the fpga code
 */
#define DOM_FPGA_BASE (0x90000000)

/**
 * Address of versioning info
 */
#define DOM_FPGA_VERSIONING DOM_FPGA_BASE

/**
 * Trigger Source
 */
#define DOM_FPGA_TRIGGER_SOURCE (DOM_FPGA_BASE + 0x400)

/**
 * Trigger Setup
 */
#define DOM_FPGA_TRIGGER_SETUP (DOM_FPGA_BASE + 0x404)

/**
 * DAQ -- data acquisition modes
 */
#define DOM_FPGA_DAQ (DOM_FPGA_BASE + 0x410)
/** bit masks for daq fields... */
#  define DOM_FPGA_DAQ_ENABLE_DAQ  0x00000001
#  define DOM_FPGA_DAQ_ENABLE_ATWD 0x00000006
#  define DOM_FPGA_DAQ_ENABLE_FADC 0x00000008
#  define DOM_FPGA_DAQ_DAQ_MODE    0x00000700
#  define DOM_FPGA_DAQ_ATWD_MODE   0x00007000
#  define DOM_FPGA_DAQ_LC_MODE     0x00070000
#  define DOM_FPGA_DAQ_LBM_MODE    0x00700000
#  define DOM_FPGA_DAQ_COMP_MODE   0x3f000000

/**
 * LBM Control -- lookback memory control
 */
#define DOM_FPGA_LBM_CONTROL (DOM_FPGA_BASE + 0x420)
#  define DOM_FPGA_LBM_CONTROL_RESET 1

/**
 * LBM Pointer -- lookback memory pointer
 */
#define DOM_FPGA_LBM_POINTER (DOM_FPGA_BASE + 0x424)

/**
 * Dom Status
 */
#define DOM_FPGA_DOM_STATUS (DOM_FPGA_BASE + 0x430)

/**
 * Systime LSB
 */
#define DOM_FPGA_SYSTIME_LSB (DOM_FPGA_BASE + 0x440)

/**
 * Systime MSB
 */
#define DOM_FPGA_SYSTIME_MSB (DOM_FPGA_BASE + 0x444)

/**
 * Local coincidence control
 */
#define DOM_FPGA_LC_CONTROL (DOM_FPGA_BASE + 0x450)
# define DOM_FPGA_LC_CONTROL_ENABLE             0xf
# define DOM_FPGA_LC_CONTROL_LC_LENGTH         (7<<4)
# define DOM_FPGA_LC_CONTROL_UP_CABLE_LENGTH   (1<<8)
# define DOM_FPGA_LC_CONTROL_DOWN_CABLE_LENGTH (1<<9)
# define DOM_FPGA_LC_CONTROL_PRE_WINDOW        (0x3f<<16)
# define DOM_FPGA_LC_CONTROL_POST_WINDOW       (0x3f<<24)

/**
 * Calibration source control
 */
#define DOM_FPGA_CAL_CONTROL (DOM_FPGA_BASE + 0x460)
# define DOM_FPGA_CAL_CONTROL_ENABLE      (0x3F)
# define DOM_FPGA_CAL_CONTROL_MODE        (7<<12)
# define DOM_FPGA_CAL_CONTROL_ATWD_OFFSET (0xf << 16)
# define DOM_FPGA_CAL_CONTROL_PULSER_RATE (0x1f << 24)

/**
 * Calibration time
 */
#define DOM_FPGA_CAL_TIME (DOM_FPGA_BASE + 0x464)

/**
 * Calibration CPU launch
 */
#define DOM_FPGA_CAL_LAUNCH (DOM_FPGA_BASE + 0x468)

/**
 * Calibration last flash time LSB
 */
#define DOM_FPGA_CAL_LAST_FLASH_LSB (DOM_FPGA_BASE + 0x46c)

/**
 * Calibration last flash time MSB
 */
#define DOM_FPGA_CAL_LAST_FLASH_MSB (DOM_FPGA_BASE + 0x470)

/**
 * Rate monitor control
 */
#define DOM_FPGA_RATE_CONTROL (DOM_FPGA_BASE + 0x480)
# define DOM_FPGA_RATE_CONTROL_ENABLE (3)
# define DOM_FPGA_RATE_CONTROL_DEADTIME (0x3ff << 16)

/**
 * Rate monitor SPE
 */
#define DOM_FPGA_RATE_SPE (DOM_FPGA_BASE + 0x484)

/**
 * Rate monitor MPE
 */
#define DOM_FPGA_RATE_MPE (DOM_FPGA_BASE + 0x488)

/**
 * Supernova meter control
 */
#define DOM_FPGA_SN_CONTROL (DOM_FPGA_BASE + 0x4a0)
#define DOM_FPGA_SN_CONTROL_ENABLE     3
#define DOM_FPGA_SN_CONTROL_GATE_TIME (0x3f<<8)
#define DOM_FPGA_SN_CONTROL_DEAD_TIME (0x7<<16)

/**
 * Supernova meter control
 */
#define DOM_FPGA_SN_DATA (DOM_FPGA_BASE + 0x4a4)

/**
 * Compression control
 */
#define DOM_FPGA_COMP_CONTROL (DOM_FPGA_BASE + 0x540)

/**
 * FADC compression thresholds
 */
#define DOM_FPGA_FADC_THRESHOLD (DOM_FPGA_BASE + 0x544)

/**
 * ATWD compression thresholds
 */
#define DOM_FPGA_ATWD_A_01_THRESHOLD (DOM_FPGA_BASE + 0x548)
#define DOM_FPGA_ATWD_A_23_THRESHOLD (DOM_FPGA_BASE + 0x54c)
#define DOM_FPGA_ATWD_B_01_THRESHOLD (DOM_FPGA_BASE + 0x550)
#define DOM_FPGA_ATWD_B_23_THRESHOLD (DOM_FPGA_BASE + 0x554)

/**
 * pong
 */
#define DOM_FPGA_PONG (DOM_FPGA_BASE + 0x7f8)

/**
 * fw debugging
 */
#define DOM_FPGA_FW_DEBUGGING (DOM_FPGA_BASE + 0x7fc)

/**
 * R2R ladder pattern
 */
#define DOM_FPGA_R2R_LADDER (DOM_FPGA_BASE + 0xc00)

/**
 * ATWD pedestal
 */
#define DOM_FPGA_ATWD_PEDESTAL (DOM_FPGA_BASE + 0x1000)

/**
 * convenience macros
 */
#define FPGA(a) ( *(volatile unsigned *) DOM_FPGA_##a )
#define FPGABIT(a, b) (DOM_FPGA_##a##_##b)

/**
 * read fpga bits...
 */
#define RFPGABIT(a, b) ( FPGA(a) & FPGABIT(a, b) )
#define RFPGABIT2(a, b, c) ( FPGA(a) & (FPGABIT(a, b) | FPGABIT(a, c)) )
#define RFPGABIT3(a, b, c, d) ( FPGA(a) & \
  (FPGABIT(a, b) | FPGABIT(a, c) | FPGABIT(a, d)) )






