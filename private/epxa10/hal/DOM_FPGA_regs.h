/**
 * \file DOM_FPGA_regs.h Description of the fpga registers...
 */

/** 
 * Base address of the fpga code
 */
#define DOM_FPGA_BASE (0x90000000)

/**
 * \defgroup fpga_test_regs FPGA Test Registers
 *
 * \brief Base address of the fpga test code.
 */
/*@{*/
/** 
 * Base address of test registers.  This address will
 * not overlap with the "real" (dom-app) fpga registers.
 */
#define DOM_FPGA_TEST_BASE (DOM_FPGA_BASE + 0x00080000) 
/*@}*/

/**
 * \defgroup fpga_test_acquire Data Acquisition Trigger
 * \ingroup fpga_test_regs
 *
 * \brief We use this register to trigger acquisition
 * of data on the fpga...
 */
/*@{*/
/** Register to trigger acquisition of data */
#define DOM_FPGA_TEST_ACQUIRE (DOM_FPGA_TEST_BASE + 0x1000)
/** send a triangle wave out comm dac */
#define   DOM_FPGA_TEST_ACQUIRE_COMM_DAC_TRIANGLE (0x00000001)
/** send a square wave out comm dac */
#define   DOM_FPGA_TEST_ACQUIRE_COMM_DAC_SQUARE   (0x00000002)
/** acquire a comm adc signal */
#define   DOM_FPGA_TEST_ACQUIRE_COMM_ADC          (0x00000100)
/** acquire a fast adc signal */
#define   DOM_FPGA_TEST_ACQUIRE_FADC              (0x00010000)
/** acquire a front end pulser */
#define   DOM_FPGA_TEST_ACQUIRE_FE_PULSER         (0x01000000)
/** acquire a single LED pulser */
#define   DOM_FPGA_TEST_ACQUIRE_LED_PULSER        (0x10000000)
/*@}*/

/**
 * \defgroup fpga_test_acquire_status Data Acquisition Trigger Status
 * \ingroup fpga_test_regs
 * \brief This register is used to check on the
 * status of an acquisition
 */
/*@{*/
/** Register to trigger acquisition of data */
#define DOM_FPGA_TEST_ACQUIRE_STATUS (DOM_FPGA_TEST_BASE + 0x1004)
/** acquisition of a comm adc signal is done */
#define   DOM_FPGA_TEST_ACQUIRE_COMM_ADC_DONE     (0x00000100)
/** acquisition of a fast adc signal is done */
#define   DOM_FPGA_TEST_ACQUIRE_FADC_DONE         (0x00010000)
/*@}*/

/**
 * \defgroup fpga_test_local Local Coincidence Control
 * \ingroup fpga_test_regs
 *
 * \brief We use this register to set the local coincidence mode.
 */
/*@{*/
/** register addresss */
#define DOM_FPGA_TEST_LOCAL (DOM_FPGA_TEST_BASE + 0x1008)
/** switch on local coincidence with DOM above */
#define   DOM_FPGA_TEST_LOCAL_UP    (0x00000001)
/** switch on local coincidence with DOM below */
#define   DOM_FPGA_TEST_LOCAL_DOWN  (0x00000002)
/** take data from ATWD 0 */
#define   DOM_FPGA_TEST_LOCAL_ATWD0 (0x00000100)
/** take data from ATWD 1 */
#define   DOM_FPGA_TEST_LOCAL_ATWD1 (0x00010000)
/*@}*/

/**
 * \defgroup fpga_test_local_status Local Coincidence Status
 * \ingroup fpga_test_regs
 *
 * \brief We use this register to check on the  local coincidence status.
 */
/*@{*/
/** register addresss */
#define DOM_FPGA_TEST_LOCAL_STATUS (DOM_FPGA_TEST_BASE + 0x100c)
/** atwd done collecting? */
#define   DOM_FPGA_TEST_LOCAL_STATUS_ATWD0 (0x00000100)
/** take data from ATWD 1 */
#define   DOM_FPGA_TEST_LOCAL_STATUS_ATWD1 (0x00010000)
/*@}*/

/**
 * \defgroup fpga_test_single_spe_rate Single SPE Rate
 * \ingroup fpga_test_regs
 *
 * \brief We use this 16 bit register to read the current
 * single SPE count rate.
 */
/*@{*/
/** register addresss */
#define DOM_FPGA_TEST_SINGLE_SPE_RATE (DOM_FPGA_TEST_BASE + 0x1010)
/*@}*/

/**
 * \defgroup fpga_test_multi_spe_rate Multiple SPE Rate
 * \ingroup fpga_test_regs
 *
 * \brief We use this 16 bit register to read the current
 * multiple SPE count rate.
 */
/*@{*/
/** register addresss */
#define DOM_FPGA_TEST_MULTIPLE_SPE_RATE (DOM_FPGA_TEST_BASE + 0x1014)
/*@}*/

/**
 * \defgroup fpga_test_rom_data ROM Configuration Data
 * \ingroup fpga_test_regs
 *
 * \brief This memory address has some number of 32 bit words
 * used for configuration information.
 */
/*@{*/
/** register addresss */
#define DOM_FPGA_TEST_ROM_DATA (DOM_FPGA_TEST_BASE + 0x0000)
/*@}*/

/**
 * \defgroup fpga_test_comm_adc_data Communications ADC Data Buffer
 * \ingroup fpga_test_regs
 *
 * \brief This memory address has 512 32 bit words of comm adc
 * data.
 */
/*@{*/
/** register addresss */
#define DOM_FPGA_TEST_COMM_ADC_DATA (DOM_FPGA_TEST_BASE + 0x2000)
/*@}*/

/**
 * \defgroup fpga_test_fast_adc_data Fast ADC Data Buffer
 * \ingroup fpga_test_regs
 *
 * \brief This memory address has 512 32 bit words of fast adc
 * data.
 */
/*@{*/
/** register addresss */
#define DOM_FPGA_TEST_FAST_ADC_DATA (DOM_FPGA_TEST_BASE + 0x3000)
/*@}*/

/**
 * \defgroup fpga_test_atwd0_data ATWD0 Data Buffer
 * \ingroup fpga_test_regs
 *
 * \brief This memory address has 512 32 bit words of ATWD0
 * data.
 */
/*@{*/
/** register addresss */
#define DOM_FPGA_TEST_FAST_ADC_DATA (DOM_FPGA_TEST_BASE + 0x4000)
/*@}*/

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
