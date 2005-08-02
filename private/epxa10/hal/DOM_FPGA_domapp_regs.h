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
 * Systime LSB
 */
#define DOM_FPGA_DOMAPP_SYSTIME_LSB (DOM_FPGA_BASE + 0x440)

/**
 * Systime MSB
 */
#define DOM_FPGA_DOMAPP_SYSTIME_MSB (DOM_FPGA_BASE + 0x444)

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
