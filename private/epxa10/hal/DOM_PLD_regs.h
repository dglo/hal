#ifndef DOM_PLD_REGS_INCLUDE
#define DOM_PLD_REGS_INCLUDE

/**
 * base address from which pld addresses are calculated...
 *
 * \link http://rust.lbl.gov/~gtp/DOM/API/DOM_CPLD_API.html
 */
#define DOM_PLD_BASE (0x50000000)

#define DOM_PLD_SPI_CHIP_SELECT0 (DOM_PLD_BASE + 0x00000004)
#define   DOM_PLD_SPI_CHIP_SELECT0_DAC_CS0 0x01
#define   DOM_PLD_SPI_CHIP_SELECT0_DAC_CS1 0x02
#define   DOM_PLD_SPI_CHIP_SELECT0_DAC_CS2 0x04
#define   DOM_PLD_SPI_CHIP_SELECT0_DAC_CS3 0x08
#define   DOM_PLD_SPI_CHIP_SELECT0_DAC_CS4 0x10
#define   DOM_PLD_SPI_CHIP_SELECT0_ADC_CS0 0x20
#define   DOM_PLD_SPI_CHIP_SELECT0_ADC_CS1 0x40
#define   DOM_PLD_SPI_CHIP_SELECT0_MUX_CS0 0x80

#define DOM_PLD_SPI_CHIP_SELECT1 (DOM_PLD_BASE + 0x00000005)
#define   DOM_PLD_SPI_CHIP_SELECT1_BASE_CS0         0x01
#define   DOM_PLD_SPI_CHIP_SELECT1_BASE_CS1         0x02
#define   DOM_PLD_SPI_CHIP_SELECT1_FLASHER_CS0      0x04

#define DOM_PLD_BASE_CHIP_SELECT (DOM_PLD_BASE + 0x00000005)
#define   DOM_PLD_BASE_CHIP_SELECT_BASE_CS0         0x01
#define   DOM_PLD_BASE_CHIP_SELECT_BASE_CS1         0x02

#define DOM_PLD_SPI_CTRL         (DOM_PLD_BASE + 0x00000006)
#define   DOM_PLD_SPI_CTRL_1_WIRE_ENABLE     0x01
#define   DOM_PLD_SPI_CTRL_SERIAL_CLK        0x02
#define   DOM_PLD_SPI_CTRL_TEMP_SENSOR_CLK   0x02
#define   DOM_PLD_SPI_CTRL_MOSI_DATA0        0x04
#define   DOM_PLD_SPI_CTRL_SC_MOSI           0x04
#define   DOM_PLD_SPI_CTRL_MOSI_DATA1        0x08
#define   DOM_PLD_SPI_CTRL_BASE_MOSI         0x08
#define   DOM_PLD_SPI_CTRL_DAC_CL            0x20
#define   DOM_PLD_SPI_CTRL_SC_CL             0x20
#define   DOM_PLD_SPI_CTRL_1_WIRE_MASTER_OUT 0x80

#define DOM_PLD_SPI_READ_DATA    (DOM_PLD_BASE + 0x00000006)
#define   DOM_PLD_SPI_READ_DATA_1_WIRE_ENABLE       0x01
#define   DOM_PLD_SPI_READ_DATA_SERIAL_CLK          0x02
#define   DOM_PLD_SPI_READ_DATA_MISO_DATA_SC        0x04
#define   DOM_PLD_SPI_READ_DATA_SC_MISO             0x04
#define   DOM_PLD_SPI_READ_DATA_MISO_DATA_BASE      0x08
#define   DOM_PLD_SPI_READ_DATA_BASE_MISO           0x08
#define   DOM_PLD_SPI_READ_DATA_MISO_DATA_FLASHER   0x10
#define   DOM_PLD_SPI_READ_DATA_1_WIRE_BUSY         0x40
#define   DOM_PLD_SPI_READ_DATA_1_WIRE_MASTER_STATE 0x80

#define DOM_PLD_SCRATCHPAD0 (DOM_PLD_BASE + 0x0000007)
#define DOM_PLD_SCRATCHPAD1 (DOM_PLD_BASE + 0x0000008)

#define DOM_PLD_I2C_CONTROL0     (DOM_PLD_BASE + 0x00000003)
#define   DOM_PLD_I2C_CONTROL0_ADC0_CLK   0x01
#define   DOM_PLD_I2C_CONTROL0_ADC0_DATA  0x02
#define   DOM_PLD_I2C_CONTROL0_ADC0_WRITE 0x04
#define   DOM_PLD_I2C_CONTROL0_ADC1_CLK   0x10
#define   DOM_PLD_I2C_CONTROL0_ADC1_DATA  0x20
#define   DOM_PLD_I2C_CONTROL0_ADC1_WRITE 0x40

#define DOM_PLD_I2C_DATA0        (DOM_PLD_BASE + 0x00800003)
#define   DOM_PLD_I2C_DATA0_ADC0_DATA 0x02
#define   DOM_PLD_I2C_DATA0_ADC1_DATA 0x20

#define DOM_PLD_I2C_CONTROL1     (DOM_PLD_BASE + 0x00000004)
#define   DOM_PLD_I2C_CONTROL1_TEMP_SENSOR_DATA  0x80

#define DOM_PLD_I2C_DATA1        (DOM_PLD_BASE + 0x00000004)
#define   DOM_PLD_I2C_DATA1_TEMP_SENSOR_DATA 0x80

#define DOM_PLD_SYSTEM_CONTROL   (DOM_PLD_BASE + 0x00000009)
#define   DOM_PLD_SYSTEM_CONTROL_HV_PS_ENABLE     0x01
#define   DOM_PLD_SYSTEM_CONTROL_BAROMETER_ENABLE 0x04
#define   DOM_PLD_SYSTEM_CONTROL_FLASHER_ENABLE   0x10
#define   DOM_PLD_SYSTEM_CONTROL_PS_DOWN          0x20
#define   DOM_PLD_SYSTEM_CONTROL_PS_UP            0x40
#define   DOM_PLD_SYSTEM_CONTROL_PS_ENABLE        0x80

#define DOM_PLD_SYSTEM_STATUS0    (DOM_PLD_BASE + 0x00000009)
#define   DOM_PLD_SYSTEM_STATUS0_HV_PS_ENABLE   0x01
#define   DOM_PLD_SYSTEM_STATUS0_FLASHER_ENABLE 0x10

#define DOM_PLD_MUX_CONTROL      (DOM_PLD_BASE + 0x0000000a)
#define   DOM_PLD_MUX_CONTROL_MUX_ENABLE0   0x01
#define   DOM_PLD_MUX_CONTROL_MUX_ENABLE1   0x02
#define   DOM_PLD_MUX_CONTROL_SELECT_ADDR0  0x04
#define   DOM_PLD_MUX_CONTROL_SELECT_ADDR1  0x08
#define   DOM_PLD_MUX_CONTROL_PLL_S0        0x10
#define   DOM_PLD_MUX_CONTROL_PLL_S1        0x20
#define   DOM_PLD_MUX_CONTROL_AUX_CTL_WRITE 0x40
#define   DOM_PLD_MUX_CONTROL_AUX_IO        0x80

#define DOM_PLD_SYSTEM_STATUS1      (DOM_PLD_BASE + 0x0000000a)
#define   DOM_PLD_SYSTEM_STATUS1_AUX_IO 0x80

#define DOM_PLD_UART_CONTROL     (DOM_PLD_BASE + 0x0000000b)
#define   DOM_PLD_UART_CONTROL_DSR_CONTROL 0x02

#define DOM_PLD_UART_STATUS      (DOM_PLD_BASE + 0x0000000b)
#define   DOM_PLD_UART_STATUS_SERIAL_POWER         0x01
#define   DOM_PLD_UART_STATUS_SERIAL_DSR           0x02
#define   DOM_PLD_UART_STATUS_SERIAL_RECEIVE_DATA  0x03
#define   DOM_PLD_UART_STATUS_SERIAL_TRANSMIT_DATA 0x04

#define DOM_PLD_ONE_WIRE (DOM_PLD_BASE + 0x000000c)
#define   DOM_PLD_ONE_WIRE_BUSY 0x20
#define   DOM_PLD_ONE_WIRE_DATA 0x40

#define DOM_PLD_FPGA_PLD (DOM_PLD_BASE + 0x000000d)

#define DOM_PLD_REBOOT_CONTROL   (DOM_PLD_BASE + 0x0000000e)
#define   DOM_PLD_REBOOT_CONTROL_INITIATE_REBOOT 0x01

#define DOM_PLD_BOOT_CONTROL     (DOM_PLD_BASE + 0x0000000f)
#define   DOM_PLD_BOOT_CONTROL_ALTERNATE_FLASH 0x01
#define   DOM_PLD_BOOT_CONTROL_BOOT_FROM_FLASH 0x02
#define   DOM_PLD_BOOT_CONTROL_NCONFIG         0x08

#define DOM_PLD_BOOT_STATUS      (DOM_PLD_BASE + 0x0000000f)
#define   DOM_PLD_BOOT_STATUS_ALTERNATE_FLASH 0x01
#define   DOM_PLD_BOOT_STATUS_BOOT_FROM_FLASH 0x02
#define   DOM_PLD_BOOT_STATUS_INIT_DONE       0x03
#define   DOM_PLD_BOOT_STATUS_NCONFIG         0x04


/* usage: PLD(BOOT_STATUS) */
#define PLD(a) ( *(volatile UBYTE *) DOM_PLD_##a )
#define PLDBIT(a, b) (DOM_PLD_##a##_##b)
#define PLDBIT2(a, b, c) (DOM_PLD_##a##_##b | DOM_PLD_##a##_##c)

/* usage: RPLDBIT(BOOT_STATUS, ALTERNATE_FLASH) */
#define RPLDBIT(a, b) ( PLD(a) & PLDBIT(a, b) )
#define RPLDBIT2(a, b, c) ( PLD(a) & (PLDBIT(a, b) | PLDBIT(a, c)) )
#define RPLDBIT3(a, b, c, d) ( PLD(a) & (PLDBIT(a, b) | PLDBIT(a, c) | PLDBIT(a, d)) )

/* FIXME: howto input EB_nOE, EB_nWE?
 */
#endif