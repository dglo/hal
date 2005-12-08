/**
 * \file DOM_FPGA_regs.h Description of the fpga registers...
 */

#include "hal/DOM_FPGA_comm.h"

/** 
 * Base address of the fpga code
 */
#define DOM_FPGA_BASE (0x90000000)

/**
 * Address of versioning info
 */
#define DOM_FPGA_VERSIONING DOM_FPGA_BASE

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
 * \defgroup fpga_test_signal Signal control
 * \ingroup fpga_test_regs
 *
 * \brief We use this register to trigger acquisition
 * of data on the fpga...
 */
/*@{*/
/** Register to trigger acquisition of data */
#define DOM_FPGA_TEST_SIGNAL (DOM_FPGA_TEST_BASE + 0x1000)

/** take data from ATWD 0 */
#define   DOM_FPGA_TEST_SIGNAL_ATWD0        (0x00000001)
/** take data from ATWD 0 trigger by discriminator */
#define   DOM_FPGA_TEST_SIGNAL_ATWD0_DISC   (0x00000002)
/** data read from ATWD 0 data buffer */
#define   DOM_FPGA_TEST_SIGNAL_ATWD0_READ_DONE (0x00000004)
/** Launches the ATWD0 when the on board LED is flashed */
#define   DOM_FPGA_TEST_SIGNAL_ATWD0_LED      (0x00000008)
/** take data from ATWD 1 */
#define   DOM_FPGA_TEST_SIGNAL_ATWD1        (0x00000100)
/** take data from ATWD 1 trigger from discriminator */
#define   DOM_FPGA_TEST_SIGNAL_ATWD1_DISC   (0x00000200)
/** data read from ATWD 1 data buffer */
#define   DOM_FPGA_TEST_SIGNAL_ATWD1_READ_DONE (0x00000400)
/** Launches the ATWD1 when the on board LED is flashed */
#define   DOM_FPGA_TEST_SIGNAL_ATWD1_LED       (0x00000800)
/** acquire data from ATWD0 and ATWD1 in ping pong mode */
#define   DOM_FPGA_TEST_SIGNAL_ATWD_PING_PONG (0x00008000)
/** acquire a fast adc signal */
#define   DOM_FPGA_TEST_SIGNAL_FADC         (0x00010000)
/** acquire a fast adc signal from discriminator */
#define   DOM_FPGA_TEST_SIGNAL_FADC_DISC    (0x00020000)
/** start front end pulser */
#define   DOM_FPGA_TEST_SIGNAL_FE_PULSER    (0x01000000)
/** create a single LED pulser */
#define   DOM_FPGA_TEST_SIGNAL_LED_PULSER   (0x04000000)
/** create a test waveform for debugging -- into channel 3 of ATWD */
#define   DOM_FPGA_TEST_SIGNAL_R2R_TRIANGLE      (0x10000000)
/** create a test waveform for debugging -- into front end, after delay line */
#define   DOM_FPGA_TEST_SIGNAL_R2R_TRIANGLE_FE   (0x40000000)
/*@}*/

/**
 * \defgroup fpga_test_signal_response ATWD and FADC Signal responses
 * \ingroup fpga_test_regs
 * \brief This register is used to check on the
 * status of a signal command
 */
/*@{*/
/** Register to trigger acquisition of data */
#define DOM_FPGA_TEST_SIGNAL_RESPONSE (DOM_FPGA_TEST_BASE + 0x1004)

/** atwd0 done collecting? */
#define   DOM_FPGA_TEST_SIGNAL_RESPONSE_ATWD0     (0x00000001)
/** atwd1 done collecting? */
#define   DOM_FPGA_TEST_SIGNAL_RESPONSE_ATWD1     (0x00000100)
/** acquisition of a fast adc signal is done */
#define   DOM_FPGA_TEST_SIGNAL_RESPONSE_FADC_DONE (0x00010000)
/** presence of local coincidence signal corresp. to ATWD0 */
#define DOM_FPGA_TEST_SIGNAL_RESPONSE_ATWD0_LC (0x00000002)
/** presence of local coincidence signal corresp. to ATWD1 */
#define DOM_FPGA_TEST_SIGNAL_RESPONSE_ATWD1_LC (0x00000200)


/*@}*/

/**
 * \defgroup fpga_test_comm Communications control
 * \ingroup fpga_test_regs
 *
 * \brief We use this register to control communications.
 *  The pulser rate sits in bits 16-19.  When bit bang mode
 *  is set, writes to bits 24-31 get output to the DAC...
 */
/*@{*/
/** register address */
#define DOM_FPGA_TEST_COMM (DOM_FPGA_TEST_BASE + 0x1008)

/** send a triangle wave out comm dac */
#define   DOM_FPGA_TEST_COMM_DAC_TRIANGLE     (0x00000001)
/** send a square wave out comm dac */
#define   DOM_FPGA_TEST_COMM_DAC_SQUARE       (0x00000002)
/** bit bang mode, access DAC directly */
#define   DOM_FPGA_TEST_COMM_DAC_BIT_BANG     (0x00000004)
/** acquire a comm adc signal */
#define   DOM_FPGA_TEST_COMM_ADC              (0x00000010)
/** ??? */
#define   DOM_FPGA_TEST_COMM_RS_485_TX        (0x00000100)
/** ??? */
#define   DOM_FPGA_TEST_COMM_RS_485_RX_ENABLE (0x00000200)
/** ??? */
#define   DOM_FPGA_TEST_COMM_RS_485_TX_ENABLE (0x00000400)
/** ??? */
#define   DOM_FPGA_TEST_COMM_RS_485_ENABLE    (0x00000800)
/*@}*/

/**
 * \defgroup fpga_test_comm_response Communications hardware responses.
 * \ingroup fpga_test_regs
 *
 * \brief We use this register to check on the communications status.
 */
/*@{*/
/** register addresss */
#define DOM_FPGA_TEST_COMM_RESPONSE (DOM_FPGA_TEST_BASE + 0x100c)

/** acquisition of a comm adc signal is done */
#define   DOM_FPGA_TEST_COMM_RESPONSE_ADC_DONE  (0x00000010)
/** ??? */
#define   DOM_FPGA_TEST_COMM_RESPONSE_RS_485_RX (0x00000100)

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
 * \defgroup fpga_test_misc Miscellaneous test fpga bits...
 * \ingroup fpga_test_regs
 *
 * \brief flasher_board sits at bits 24-31...
 */
/*@{*/
/** register addresss */
#define DOM_FPGA_TEST_MISC (DOM_FPGA_TEST_BASE + 0x1018)

/** switch on local coincidence with dom above */
#define   DOM_FPGA_TEST_MISC_LOCAL_UP           (0x00000001)
/** switch on local coincidence with dom below */
#define   DOM_FPGA_TEST_MISC_LOCAL_DOWN         (0x00000002)
/** initiate upper/lower LC pulses when SPE disc. fires */
#define   DOM_FPGA_TEST_MISC_LOCAL_SPE          (0x00000008)
/** enable Rx from lower DOM - must use with DOM_FPGA_TEST_MISC_LOCAL_SPE */
#define   DOM_FPGA_TEST_MISC_LOCAL_RX_LO        (0x00000010)
/** enable Rx from upper DOM - must use with DOM_FPGA_TEST_MISC_LOCAL_SPE */
#define   DOM_FPGA_TEST_MISC_LOCAL_RX_HI        (0x00000020)
/** requires lc Rx from _both_ upper and lower DOMs */
#define   DOM_FPGA_TEST_MISC_LOCAL_REQUIRE_UP_DOWN (0x00000040)
/** send high pulse to lower dom */
#define   DOM_FPGA_TEST_MISC_LOCAL_DOWN_HIGH    (0x00000100)
/** send low pulse to lower dom */
#define   DOM_FPGA_TEST_MISC_LOCAL_DOWN_LOW     (0x00000200)
/** send high pulse to upper dom */
#define   DOM_FPGA_TEST_MISC_LOCAL_UP_HIGH      (0x00000400)
/** send low pulse to upper dom */
#define   DOM_FPGA_TEST_MISC_LOCAL_UP_LOW       (0x00000800)
/** for lower dom: 1 -- tristate A latch, 0 -- clear A latch */
#define   DOM_FPGA_TEST_MISC_LOCAL_DOWN_ALATCH  (0x00001000)
/** for lower dom: 1 -- tristate B latch, 0 -- clear B latch */
#define   DOM_FPGA_TEST_MISC_LOCAL_DOWN_BLATCH  (0x00002000)
/** for upper dom: 1 -- tristate A latch, 0 -- clear A latch */
#define   DOM_FPGA_TEST_MISC_LOCAL_UP_ALATCH    (0x00004000)
/** for upper dom: 1 -- tristate B latch, 0 -- clear B latch */
#define   DOM_FPGA_TEST_MISC_LOCAL_UP_BLATCH    (0x00008000)
/** flasher board trigger */
#define   DOM_FPGA_TEST_MISC_FL_TRIGGER         (0x01000000)
/** flasher board pre-trigger */
#define   DOM_FPGA_TEST_MISC_FL_PRE_TRIGGER     (0x04000000)
/** flasher board tms */
#define   DOM_FPGA_TEST_MISC_FL_TMS             (0x10000000)
/** flasher board tck */
#define   DOM_FPGA_TEST_MISC_FL_TCK             (0x20000000)
/** flasher board tdi */
#define   DOM_FPGA_TEST_MISC_FL_TDI             (0x40000000)
/** enable flasher board jtag signals */
#define   DOM_FPGA_TEST_MISC_FL_EN_JTAG         (0x80000000)
/*@}*/

/**
 * \defgroup fpga_test_misc_response Miscellaneous responses...
 * \ingroup fpga_test_regs
 *
 * \brief coincidence_disc is bits 8-15
 */
/*@{*/
/** register addresss */
#define DOM_FPGA_TEST_MISC_RESPONSE (DOM_FPGA_TEST_BASE + 0x101c)
/** local coincidence down A */
#define DOM_FPGA_TEST_MISC_RESPONSE_COINC_DOWN_A    0x00000100
/** local coincidence down A BAR */
#define DOM_FPGA_TEST_MISC_RESPONSE_COINC_DOWN_ABAR 0x00000200
/** local coincidence down B */
#define DOM_FPGA_TEST_MISC_RESPONSE_COINC_DOWN_B    0x00000400
/** local coincidence down B BAR */
#define DOM_FPGA_TEST_MISC_RESPONSE_COINC_DOWN_BBAR 0x00000800
/** local coincidence up A */
#define DOM_FPGA_TEST_MISC_RESPONSE_COINC_UP_A      0x00001000
/** local coincidence up A BAR */
#define DOM_FPGA_TEST_MISC_RESPONSE_COINC_UP_ABAR   0x00002000
/** local coincidence up B */
#define DOM_FPGA_TEST_MISC_RESPONSE_COINC_UP_B      0x00004000
/** local coincidence up B BAR */
#define DOM_FPGA_TEST_MISC_RESPONSE_COINC_UP_BBAR   0x00008000
/** flasher board read */
#define   DOM_FPGA_TEST_MISC_RESPONSE_FL_ATTN      (0x01000000)
/** flasher board read JTAG TDO */
#define   DOM_FPGA_TEST_MISC_RESPONSE_FL_TDO       (0x10000000)
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
 * master test address (debug only).  
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
 * \defgroup fpga_test_local_clock_low Low bits of Local clock readout register
 * \ingroup fpga_test_regs
 *
 * \brief This register contains the low 32 bits of the 48 bit local
 * clock readout register.
 */
/*@{*/
/** register addresss */
#define DOM_FPGA_TEST_LOCAL_CLOCK_LOW (DOM_FPGA_TEST_BASE + 0x1040)
/*@}*/

/**
 * \defgroup fpga_test_local_clock_high High bits of local clock readout
 * \ingroup fpga_test_regs
 *
 * \brief This register contains the high 32 bits of the 48 bit local
 * clock readout register.
 */
/*@{*/
/** register addresss */
#define DOM_FPGA_TEST_LOCAL_CLOCK_HIGH (DOM_FPGA_TEST_BASE + 0x1044)
/*@}*/

/**
 * \defgroup fpga_test_atwd0_timestamp_low ATWD0 local clock readout (low)
 * \ingroup fpga_test_regs
 *
 * \brief This register contains the low 32 bits of the 48 bit local
 * clock readout register on the start of a atwd0 readout...
 */
/*@{*/
/** register addresss */
#define DOM_FPGA_TEST_ATWD0_TIMESTAMP_LOW (DOM_FPGA_TEST_BASE + 0x1048)
/*@}*/

/**
 * \defgroup fpga_test_atwd0_timestamp_high ATWD0 local clock readout (high)
 * \ingroup fpga_test_regs
 *
 * \brief This register contains the high 16 bits of the 48 bit local
 * clock readout register on the start of a atwd0 readout...
 */
/*@{*/
/** register addresss */
#define DOM_FPGA_TEST_ATWD0_TIMESTAMP_HIGH (DOM_FPGA_TEST_BASE + 0x104c)
/*@}*/

/**
 * \defgroup fpga_test_atwd1_timestamp_low ATWD1 local clock readout (low)
 * \ingroup fpga_test_regs
 *
 * \brief This register contains the low 32 bits of the 48 bit local
 * clock readout register on the start of a atwd1 readout...
 */
/*@{*/
/** register addresss */
#define DOM_FPGA_TEST_ATWD1_TIMESTAMP_LOW (DOM_FPGA_TEST_BASE + 0x1050)
/*@}*/

/**
 * \defgroup fpga_test_atwd1_timestamp_high ATWD1 local clock readout (high)
 * \ingroup fpga_test_regs
 *
 * \brief This register contains the high 16 bits of the 48 bit local
 * clock readout register on the start of a atwd1 readout...
 */
/*@{*/
/** register addresss */
#define DOM_FPGA_TEST_ATWD1_TIMESTAMP_HIGH (DOM_FPGA_TEST_BASE + 0x1054)
/*@}*/



/**
 * \defgroup fpga_test_domid_low low 32 bit of the dom id
 * \ingroup fpga_test_regs
 *
 * \brief This register contains the low 32 bits of the 48 bit domid
 */
/*@{*/
/** register addresss */
#define DOM_FPGA_TEST_DOMID_LOW (DOM_FPGA_TEST_BASE + 0x1058)
/*@}*/

/**
 * \defgroup fpga_test_domid_high high 16 bits of the dom id
 * \ingroup fpga_test_regs
 *
 * \brief This register contains the low 32 bits of the 48 bit domid,
 * bit 16 of this register is a flag that must set to 1 to indicate
 * that the registers are now valid (both fpga_test_domid_high and
 * fpga_test_domid_low).
 */
/*@{*/
/** register addresss */
#define DOM_FPGA_TEST_DOMID_HIGH (DOM_FPGA_TEST_BASE + 0x105c)
/*@}*/

/**
 * \defgroup fpga_test_led_atwd_delay Launch delay from on board led to ATWD
 * \ingroup fpga_test_regs
 *
 * \brief Low 4 bits are launch delay from on board led to ATWD.  bits
 * 12..15 are used for ATWD deadtime.  bit 8 is used to determine whether
 * the scalars are in fast mode or not...
 *
 * Delay is (2+LED_ATWD_DELAY)*25ns
 *
 * \see DOM_FPGA_TEST_Deadtimes
 */
/*@{*/
/** register addresss */
#define DOM_FPGA_TEST_LED_ATWD_DELAY (DOM_FPGA_TEST_BASE + 0x1060)

/** set scalars to 10ms sample period (100ms is the default) */
#define DOM_FPGA_TEST_LED_ATWD_DELAY_FAST_SCALAR (0x00000100)
/*@}*/

/**
 * \defgroup fpga_test_lcoin_launch_win Local Coin. window
 * \ingroup fpga_test_regs
 *
 * \brief This memory address consists of 4 6-bit words defining
 * local coincidence windows (in 50 nsec clock ticks)
 * LC_up_pre_window: bits 5 - 0
 * LC_up_post_window: bits 13 - 8
 * LC_down_pre_window: bits 21 - 16
 * LC_down_post_window: bits 29 - 24
 */
/*@{*/
/** register address */
#define DOM_FPGA_TEST_LOCOIN_LAUNCH_WIN (DOM_FPGA_TEST_BASE + 0x1068)
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
#define DOM_FPGA_COMM_CLEV    (DOM_FPGA_TEST_BASE + 0x1080)
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
#define DOM_FPGA_COMM_THR_DEL (DOM_FPGA_TEST_BASE + 0x1084)
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
 * \defgroup fpga_domapp_regs FPGA domapp Registers
 *
 * \brief Base address of the dom app registers
 */
/*@{*/
#define DOM_FPGA_DOMAPP_BASE (DOM_FPGA_BASE + 0x0)
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

