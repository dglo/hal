#ifndef DOM_FB_REGS_INCLUDE
#define DOM_FB_REGS_INCLUDE

/**
 * Base address from which flasher board addresses are calculated...
 *
 * \link http://docushare.icecube.wisc.edu/docushare/dsweb/Get/Document-7188/flasherInterface.pdf
 */
#define DOM_FB_BASE (0x60000000)

/* Flasher board layout version, RO */
#define DOM_FB_VERSION       (DOM_FB_BASE + 0x00000000)

/* Addressing resets CPLD; data ignored */
#define DOM_FB_RESET         (DOM_FB_BASE + 0x00000001)

/* CPLD firmware version, 8b RO */
#define DOM_FB_CPLD_VERSION  (DOM_FB_BASE + 0x00000002)

/* 
 * OBSOLETE: only used for CPLD version <= 4
 * Set CPLD working mode (normal, spi, or one-wire). 
 * 2b WO, see state machine in documentation for details.
 */
#define DOM_FB_MODE_SELECT   (DOM_FB_BASE + 0x00000003)

/* SPI control register -- used to set brightness */
/* For versions >= 6, SPI control is internal to PLD */
/* and can just write value to this register */
#define DOM_FB_SPI_CTRL      (DOM_FB_BASE + 0x00000004)
#define DOM_FB_SPI_CTRL_SCLK 0x01
#define DOM_FB_SPI_CTRL_MOSI 0x02
/* Active low! */
#define DOM_FB_SPI_CTRL_CSN  0x04

/* Power control, 1b WO */
#define DOM_FB_DCDC_CTRL     (DOM_FB_BASE + 0x00000005)

/* One-wire control -- used to read ID */
#define DOM_FB_ONE_WIRE      (DOM_FB_BASE + 0x00000006)
#define DOM_FB_ONE_WIRE_PRESENT 0x10
#define DOM_FB_ONE_WIRE_BUSY    0x20
#define DOM_FB_ONE_WIRE_DATA    0x40

/* Local clock counter, low byte */
#define DOM_FB_CLK_LO        (DOM_FB_BASE + 0x00000007)

/* Pulse width adjustment, 8b */
#define DOM_FB_DELAY_ADJUST  (DOM_FB_BASE + 0x00000008)

/* LED enable bits (D1-D4) */ 
#define DOM_FB_LED_EN_4_1    (DOM_FB_BASE + 0x00000009)
#define   DOM_FB_LED_EN_4_1_D1 0x01
#define   DOM_FB_LED_EN_4_1_D2 0x02
#define   DOM_FB_LED_EN_4_1_D3 0x04
#define   DOM_FB_LED_EN_4_1_D4 0x08

/* LED enable bits (D5-D8) */ 
#define DOM_FB_LED_EN_8_5 (DOM_FB_BASE + 0x0000000a)
#define   DOM_FB_LED_EN_8_5_D5 0x01
#define   DOM_FB_LED_EN_8_5_D6 0x02
#define   DOM_FB_LED_EN_8_5_D7 0x04
#define   DOM_FB_LED_EN_8_5_D8 0x08

/* LED enable bits (D9-D12) */ 
#define DOM_FB_LED_EN_12_9 (DOM_FB_BASE + 0x0000000b)
#define   DOM_FB_LED_EN_12_9_D9  0x01
#define   DOM_FB_LED_EN_12_9_D10 0x02
#define   DOM_FB_LED_EN_12_9_D11 0x04
#define   DOM_FB_LED_EN_12_9_D12 0x08

/* Select which LED current is muxed back to mainboard */
/* 4b WO, see documentation for select details */
#define DOM_FB_LED_MUX       (DOM_FB_BASE + 0x0000000c)

/* Enables for LED current mux, 4b WO */
#define DOM_FB_LED_MUX_EN    (DOM_FB_BASE + 0x0000000d)

/* Delay chip enable, 1b WO */
#define DOM_FB_LE_DP         (DOM_FB_BASE + 0x0000000e)

/* Local clock counter, high byte */
#define DOM_FB_CLK_HI        (DOM_FB_BASE + 0x0000000f)

/* usage: FB(DELAY_ADJUST) */
#define FB(a) ( *(volatile UBYTE *) DOM_FB_##a )
#define FBBIT(a, b) (DOM_FB_##a##_##b)
#define FBBIT2(a, b, c) (DOM_FB_##a##_##b | DOM_FB_##a##_##c)

/* usage: RFBBIT(BOOT_STATUS, ALTERNATE_FLASH) */
#define RFBBIT(a, b) ( FB(a) & FBBIT(a, b) )
#define RFBBIT2(a, b, c) ( FB(a) & (FBBIT(a, b) | FBBIT(a, c)) )
#define RFBBIT3(a, b, c, d) ( FB(a) & (FBBIT(a, b) | FBBIT(a, c) | FBBIT(a, d)) )

#endif
