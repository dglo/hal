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
/** register address */
#define DOM_FPGA_TEST_LOCAL (DOM_FPGA_TEST_BASE + 0x1008)

/** switch on local coincidence with dom above */
#define   DOM_FPGA_TEST_LOCAL_UP           (0x00000001)
/** switch on local coincidence with dom below */
#define   DOM_FPGA_TEST_LOCAL_DOWN         (0x00000002)
/** send high pulse to lower dom */
#define   DOM_FPGA_TEST_LOCAL_DOWN_HIGH    (0x00000100)
/** send low pulse to lower dom */
#define   DOM_FPGA_TEST_LOCAL_DOWN_LOW     (0x00000200)
/** send high pulse to upper dom */
#define   DOM_FPGA_TEST_LOCAL_UP_HIGH      (0x00000400)
/** send low pulse to upper dom */
#define   DOM_FPGA_TEST_LOCAL_UP_LOW       (0x00000800)
/** for lower dom: 1 -- tristate A latch, 0 -- clear A latch */
#define   DOM_FPGA_TEST_LOCAL_DOWN_ALATCH  (0x00001000)
/** for lower dom: 1 -- tristate B latch, 0 -- clear B latch */
#define   DOM_FPGA_TEST_LOCAL_DOWN_BLATCH  (0x00002000)
/** for upper dom: 1 -- tristate A latch, 0 -- clear A latch */
#define   DOM_FPGA_TEST_LOCAL_UP_ALATCH    (0x00004000)
/** for upper dom: 1 -- tristate B latch, 0 -- clear B latch */
#define   DOM_FPGA_TEST_LOCAL_UP_BLATCH    (0x00008000)
/** take data from ATWD 0 */
#define   DOM_FPGA_TEST_LOCAL_ATWD0        (0x00010000)
/** take data from ATWD 0 trigger by disc */
#define   DOM_FPGA_TEST_LOCAL_ATWD0_DISC   (0x00020000)
/** take data from ATWD 1 */
#define   DOM_FPGA_TEST_LOCAL_ATWD1        (0x01000000)
/** take data from ATWD 1 trigger from disc */
#define   DOM_FPGA_TEST_LOCAL_ATWD1_DISC   (0x02000000)
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

/** read A on lower dom */
#define   DOM_FPGA_TEST_LOCAL_STATUS_DOWN_A    (0x00000100)
/** read A bar on lower dom */
#define   DOM_FPGA_TEST_LOCAL_STATUS_DOWN_ABAR (0x00000200)
/** read B on lower dom */
#define   DOM_FPGA_TEST_LOCAL_STATUS_DOWN_B    (0x00000400)
/** read B bar on lower dom */
#define   DOM_FPGA_TEST_LOCAL_STATUS_DOWN_BBAR (0x00000800)
/** read A on upper dom */
#define   DOM_FPGA_TEST_LOCAL_STATUS_UP_A      (0x00001000)
/** read A bar on upper dom */
#define   DOM_FPGA_TEST_LOCAL_STATUS_UP_ABAR   (0x00002000)
/** read B on upper dom */
#define   DOM_FPGA_TEST_LOCAL_STATUS_UP_B      (0x00004000)
/** read B bar on upper dom */
#define   DOM_FPGA_TEST_LOCAL_STATUS_UP_BBAR   (0x00008000)
/** atwd done collecting? */
#define   DOM_FPGA_TEST_LOCAL_STATUS_ATWD0     (0x00010000)
/** take data from ATWD 1 */
#define   DOM_FPGA_TEST_LOCAL_STATUS_ATWD1     (0x01000000)
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
 * \defgroup fpga_test_hdv_control Control HDV
 * \ingroup fpga_test_regs
 *
 * \brief Control the RS485 tranceiver
 */
/*@{*/
/** register addresss */
#define DOM_FPGA_TEST_HDV_CONTROL (DOM_FPGA_TEST_BASE + 0x1018)
/** */
#define DOM_FPGA_TEST_HDV_CONTROL_In        (0x00000001)
#define DOM_FPGA_TEST_HDV_CONTROL_Rx_ENABLE (0x00000002)
#define DOM_FPGA_TEST_HDV_PULSE             (0x00000010)
#define DOM_FPGA_TEST_HDV_AHB_MASTER_TEST   (0x00000100)
#define DOM_FPGA_TEST_HDV_R2R_TRIANGLE      (0x00010000)
#define DOM_FPGA_TEST_HDV_R2R_TRIANGLE_FE   (0x00100000)
/*@}*/

/**
 * \defgroup fpga_test_hdv_status HDV Status
 * \ingroup fpga_test_regs
 *
 * \brief Status of the RS485 tranceiver
 */
/*@{*/
/** register addresss */
#define DOM_FPGA_TEST_HDV_STATUS (DOM_FPGA_TEST_BASE + 0x101c)
/** Receive data available */
#define DOM_FPGA_TEST_HDV_STATUS_Rx (0x00000001)
#define DOM_FPGA_TEST_HDV_AHB_MASTER_TEST_DONE (0x00000100)
#define DOM_FPGA_TEST_HDV_AHB_MASTER_TEST_BUS_ERROR (0x00000200)
/*@}*/

/**
 * \defgroup fpga_test_single_spe_rate_fpga Single SPE Rate with FPGA Latch
 * \ingroup fpga_test_regs
 *
 * \brief We use this 16 bit register to read the current
 * single SPE count rate using the FPGA to latch.
 */
/*@{*/
/** register addresss */
#define DOM_FPGA_TEST_SINGLE_SPE_RATE_FPGA (DOM_FPGA_TEST_BASE + 0x1020)
/*@}*/

/**
 * \defgroup fpga_test_multi_spe_rate_fpga Multiple SPE Rate with FPGA Latch
 * \ingroup fpga_test_regs
 *
 * \brief We use this 16 bit register to read the current
 * multiple SPE count rate using the FPGA to latch.
 */
/*@{*/
/** register addresss */
#define DOM_FPGA_TEST_MULTIPLE_SPE_RATE_FPGA (DOM_FPGA_TEST_BASE + 0x1024)
/*@}*/

/**
 * \defgroup fpga_test_ahb_master_test Low 16 bits of AHB Master Test Address
 * \ingroup fpga_test_regs
 *
 * \brief We use the low 16 bits of this register to write the current
 * master test address (debug only).  We use bits 16-19 of this register
 * to write the front end pulser rate...
 */
/*@{*/
/** register addresss */
#define DOM_FPGA_TEST_AHB_MASTER_TEST (DOM_FPGA_TEST_BASE + 0x1028)

/**
 * pulser rate values for bits 16-19...
 */
enum DOMPulserRates {
   /** ~78kHz */
   DOMPulserRate78k,
   /** ~39kHz */
   DOMPulserRate39k   = 0x10000,
   /** ~19.5kHz */
   DOMPulserRate19_5k = 0x20000,
   /** ~9.7kHz */
   DOMPulserRate9_7k  = 0x30000,
   /** ~4.8kHz */
   DOMPulserRate4_8k  = 0x40000,
   /** ~2.4kHz */
   DOMPulserRate2_4k  = 0x50000,
   /** ~1.2kHz */
   DOMPulserRate1_2k  = 0x60000,
   /** ~.6kHz */
   DOMPulserRate_6k   = 0x70000
};
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
#define DOM_FPGA_TEST_ATWD0_DATA (DOM_FPGA_TEST_BASE + 0x4000)
/*@}*/

/**
 * \defgroup fpga_test_atwd1_data ATWD1 Data Buffer
 * \ingroup fpga_test_regs
 *
 * \brief This memory address has 512 32 bit words of ATWD0
 * data.
 */
/*@{*/
/** register addresss */
#define DOM_FPGA_TEST_ATWD1_DATA (DOM_FPGA_TEST_BASE + 0x5000)
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
