/**
 * \file hal.c, the cpld dom hal.
 */
#include <stddef.h>

#include "hal/DOM_MB_hal.h"
#include "hal/DOM_PLD_regs.h"

#include "booter/epxa.h"

#include "dom-cpld/pld-version.h"

static void max5250Write(int chan, int val);
static void max525Write(int chan, int val);
static void max534Write(int chan, int val);

static int max146Read(int chan);

static void startI2C(void);
static void stopI2C(void);
static void writeI2CByte(int val);
static int readI2CByte(int ack);
static void writeDS1631Config(int val);
static int readDS1631Config(void);
static void convertDS1631(void);
static int readDS1631Temp(void);
static void waitus(int us);
static void writeLTC1257(int val);
static void writeLTC1451(int val);
static int readLTC1286(void);
static int max1139Read(int cs, int ch);

BOOLEAN halIsSimulationPlatform() { return 0; }

USHORT halGetVersion(void) { return PLD_VERSION_API_NUM; }
USHORT halGetHWVersion(void) { return PLD(API_LOW)|(PLD(API_HIGH)<<8); }
USHORT halGetBuild(void) { return PLD_VERSION_BUILD_NUM; }
USHORT halGetHWBuild(void) { return PLD(BUILD_LOW)|(PLD(BUILD_HIGH)<<8); }

static int quickHack() {
#if defined(CPLD_ADDR)
   volatile unsigned char *p = (volatile unsigned char *) 0x50000009;
   return *p & 0x80;
#else
   return 0;
#endif
}

BOOLEAN halIsConsolePresent() { 
#if defined(CPLD_ADDR)
   return !quickHack() && RPLDBIT(UART_STATUS, SERIAL_POWER) != 0;
#else
   return 1;
#endif
}

void halSetFlashBoot() {
   PLD(BOOT_CONTROL) =
      DOM_PLD_BOOT_CONTROL_BOOT_FROM_FLASH | 
      RPLDBIT(BOOT_STATUS, ALTERNATE_FLASH);
}

void halClrFlashBoot() { 
   PLD(BOOT_CONTROL) = RPLDBIT(BOOT_STATUS, ALTERNATE_FLASH);
}

BOOLEAN halFlashBootState() { 
   return RPLDBIT(BOOT_STATUS, BOOT_FROM_FLASH)!=0;
}

USHORT halReadBaseADC(void) { return readLTC1286(); }

void halDisableBaseHV(void) {
   PLD(MISC) = PLD(MISC) | PLDBIT(MISC, BASE_HV_DISABLE);
}

void halEnableBaseHV(void) {
   PLD(MISC) = PLD(MISC) & ~PLDBIT(MISC, BASE_HV_DISABLE);
}

USHORT halReadADC(UBYTE channel) {
   /* HACK!!!! HACK!!!!
    *
    * FIXME: first read from the ADC gives
    * the wrong value.  here we just throw it
    * away, we should fix this!!!
    */
   static int init = 0;
   if (!init) { max1139Read(channel/12, channel % 12); init = 1; }
   return max1139Read(channel/12, channel % 12);
}

static int daclookup[DOM_HAL_NUM_DAC_CHANNELS];

static const int dacMaxValues[] = {
   /* 0 */ 4095,
   /* 1 */ 4095,
   /* 2 */ 4095,
   /* 3 */ 4095,
   /* 4 */ 4095,
   /* 5 */ 4095,
   /* 6 */ 4095,
   /* 7 */ 4095,
   /* 8 */ 1023,
   /* 9 */ 1023,
   /* 10 */ 1023,
   /* 11 */ 1023,
   /* 12 */ 1023,
   /* 13 */ 1023,
   /* 14 */ 1023,
   /* 15 */ 1023,
};

static int fixupDACValue(int ch, int value) {
   if (value<0) return 0;
   if (value>dacMaxValues[ch]) return dacMaxValues[ch];
   return value;
}

void halWriteDAC(UBYTE channel, int value) {
   const int cs = channel/4;
   const int dev = channel%4;

   if (channel>=DOM_HAL_NUM_DAC_CHANNELS) return;

   daclookup[channel] = fixupDACValue(channel, value);

   /* now clock it out...
    */
   PLD(SPI_CHIP_SELECT0) = ~(1<<cs);

   if (cs==0 || cs==1) {
      /* ATWD0/1 */
      max525Write(dev, value);
   }
   else if (cs==2 || cs==3) {
      /* U23, U28 on file 3, pg 3 == max5250 */
      max5250Write(dev, value);
   }
   else if (cs==4) {
      /* spares */
      max534Write(dev, value);
   }

   /* sleep until DAC has settled... */
   if (channel==DOM_HAL_DAC_MULTIPLE_SPE_THRESH ||
       channel==DOM_HAL_DAC_SINGLE_SPE_THRESH) {
      halUSleep(1200 * 5);
   }
   else if (channel==DOM_HAL_DAC_FAST_ADC_REF) {
      halUSleep(5000 * 5);
   }
   else if (channel==DOM_HAL_DAC_FL_REF) {
      halUSleep(500 * 5);
   }
   else if (channel==DOM_HAL_DAC_MUX_BIAS) {
      halUSleep(1000 * 5);
   }
   else if (channel==DOM_HAL_DAC_ATWD_ANALOG_REF) {
      halUSleep(1250 * 5);
   }
   else if (channel==DOM_HAL_DAC_PMT_FE_PEDESTAL) {
      halUSleep(6250 * 2);
   }
   else {
      halUSleep(1);
   }

   PLD(SPI_CHIP_SELECT0) = ~0;
}

static USHORT baseDAC = 0;

void halWriteActiveBaseDAC(USHORT value) {
   writeLTC1257(value);
   baseDAC = value;
}

void halWritePassiveBaseDAC(USHORT value) {
   writeLTC1451(value);
   baseDAC = value;
}

void halWriteBaseDAC(USHORT value) {
   halWriteActiveBaseDAC(value);
}

USHORT halReadBaseDAC(void) {
   return baseDAC;
}

USHORT halReadDAC(UBYTE channel) {
   if (channel>=DOM_HAL_NUM_DAC_CHANNELS) return 0;
   return daclookup[channel];
}

void halStartReadTemp(void) {
   static int chk = 1;
   
   if (chk) {
      /* read configuration byte if it's not what we want,
       * set it up...
       */
      int cr = readDS1631Config();

      if ( (cr&0xf) != 0xf) {
	 writeDS1631Config(0x0f);

	 /* wait for done...
	  */
	 while (1) {
	    if (((cr=readDS1631Config())&0xf) == 0xf) break;
	 }
      }

      chk = 0;
   }

   /* request a conversion take place (we're in one-shot
    * mode for power savings)...
    */
   convertDS1631();
}

int halReadTempDone(void) { return (readDS1631Config()&0x80) != 0; }

USHORT halFinishReadTemp(void) {
   while (!halReadTempDone()) ;
   return readDS1631Temp();
}

USHORT halReadTemp(void) { 
   halStartReadTemp();
   return halFinishReadTemp();
}

static UBYTE muxlookup;

void halSelectAnalogMuxInput(UBYTE chan) {
   const int ch = chan%8;
   const int cs = (ch/4) ? 2 : 1;
   const int addr = ch%4;
   muxlookup = ch;
   PLD(MUX_CONTROL) = cs | (addr<<2);
}

void halSetSwapFlashChips(void) {
   PLD(BOOT_CONTROL) = 
      PLDBIT(BOOT_CONTROL, ALTERNATE_FLASH) |
      RPLDBIT(BOOT_STATUS, BOOT_FROM_FLASH);
}

void halClrSwapFlashChips(void) {
   PLD(BOOT_CONTROL) = RPLDBIT(BOOT_STATUS, BOOT_FROM_FLASH);
}

BOOLEAN halSwapFlashChipsState(void) {
   return RPLDBIT(BOOT_CONTROL, ALTERNATE_FLASH)!=0;
}

/* system control i/o
 */
void halEnableBarometer(void) {
   PLD(SYSTEM_CONTROL) = 
      PLD(SYSTEM_CONTROL) | PLDBIT(SYSTEM_CONTROL, BAROMETER_ENABLE);
}

void halDisableBarometer(void) {
   PLD(SYSTEM_CONTROL) =
      PLD(SYSTEM_CONTROL) & (~PLDBIT(SYSTEM_CONTROL, BAROMETER_ENABLE));
}

static int hvIsPowered = 0;

void halPowerUpBase(void) {
   /* ltc1257 requires cs0 to be low on power up...
    */
   PLD(SPI_CHIP_SELECT1) = 0; 

   /* disable must be low when powering on... */
   halEnableBaseHV();
 
   PLD(SYSTEM_CONTROL) = 
      PLD(SYSTEM_CONTROL) | PLDBIT(SYSTEM_CONTROL, HV_PS_ENABLE);
   halUSleep(5000);

   /* DAC -> zero */
   halWriteBaseDAC(0);

   /* after 500ms disable goes high... */
   halUSleep(495000);
   halDisableBaseHV();

   /* chip selects are normally high...
    */
   PLD(SPI_CHIP_SELECT1) = PLDBIT2(SPI_CHIP_SELECT1, BASE_CS0, BASE_CS1); 

   hvIsPowered = 1;
}

void halPowerDownBase(void) {
   /* drop disable */
   halEnableBaseHV();
   
   /* drop power */
   PLD(SYSTEM_CONTROL) = 
      PLD(SYSTEM_CONTROL) & (~PLDBIT(SYSTEM_CONTROL, HV_PS_ENABLE));

   hvIsPowered = 0;
}

void halEnableFlasher(void) {
   PLD(SYSTEM_CONTROL) = 
      PLD(SYSTEM_CONTROL) | PLDBIT(SYSTEM_CONTROL, FLASHER_ENABLE);
}

void halDisableFlasher(void) {
   PLD(SYSTEM_CONTROL) = 
      PLD(SYSTEM_CONTROL) & (~PLDBIT(SYSTEM_CONTROL, FLASHER_ENABLE));
}

void halEnableFlasherJTAG(void) {
   PLD(SYSTEM_CONTROL) = 
      PLD(SYSTEM_CONTROL) | PLDBIT(SYSTEM_CONTROL, FLASHER_JTAGEN);
}

void halDisableFlasherJTAG(void) {
   PLD(SYSTEM_CONTROL) = 
      PLD(SYSTEM_CONTROL) & (~PLDBIT(SYSTEM_CONTROL, FLASHER_JTAGEN));
}

BOOLEAN halFlasherState() {
   return RPLDBIT(SYSTEM_STATUS0, FLASHER_ENABLE)!=0; 
}

void halEnableLEDPS() {
   PLD(SYSTEM_CONTROL) =
      PLD(SYSTEM_CONTROL) | PLDBIT(SYSTEM_CONTROL, SINGLE_LED_ENABLE);
}

void halDisableLEDPS() {
   PLD(SYSTEM_CONTROL) =
      PLD(SYSTEM_CONTROL) & (~PLDBIT(SYSTEM_CONTROL, SINGLE_LED_ENABLE));
}

BOOLEAN halLEDPSState() {
   return (PLD(SYSTEM_CONTROL) & PLDBIT(SYSTEM_CONTROL, SINGLE_LED_ENABLE))!=0;
}

void halStepUpLED() {}
void halStepDownLED() {}

BOOLEAN halIsSerialDSR() { return RPLDBIT(UART_STATUS, SERIAL_DSR); }
BOOLEAN halIsSerialReceiveData() { 
   return RPLDBIT(UART_STATUS, SERIAL_RECEIVE_DATA); 
}
BOOLEAN halIsSerialTransmitData() { 
   return RPLDBIT(UART_STATUS, SERIAL_TRANSMIT_DATA); 
}

void halEnableSerialDSR() {
   PLD(UART_CONTROL) = PLD(UART_CONTROL) | PLDBIT(UART_CONTROL, DSR_CONTROL);
}

void halDisableSerialDSR() {
   PLD(UART_CONTROL) = 
      PLD(UART_CONTROL) & (~PLDBIT(UART_CONTROL, DSR_CONTROL));
}

BOOLEAN halSerialDSRState() { return RPLDBIT(UART_STATUS, SERIAL_DSR); }

void halBoardReboot() {
   if (halIsFPGALoaded() && hal_FPGA_TEST_is_comm_avail()) {
      hal_FPGA_TEST_request_reboot();

      /* we can't reboot unless we're granted a reboot...
       */
      while (!hal_FPGA_TEST_is_reboot_granted()) ;
   }
   
   PLD(REBOOT_CONTROL) = PLDBIT(REBOOT_CONTROL, INITIATE_REBOOT);
}

static unsigned short crc16Once(unsigned short crcval, unsigned char cval) {
   int i;
   const unsigned short poly16 = 0x1021;
   crcval ^= cval << 8;
   for (i = 8; i--; )
      crcval = crcval & 0x8000 ? (crcval << 1) ^ poly16 : crcval << 1;
   return crcval;
}

static unsigned crc32Once(unsigned crcval, unsigned char cval) {
   int i;
   const unsigned poly32 = 0x04C11DB7;
   crcval ^= cval << 24;
   for (i = 8; i--; )
      crcval = crcval & 0x80000000 ? (crcval << 1) ^ poly32 : crcval << 1;
   return crcval;
}

static unsigned short crc16(unsigned char *b, int n) {
   int i;
   unsigned short crc = 0;
   for (i=0; i<n; i++) crc = crc16Once(crc, b[i]);
   return crc;
}

static unsigned crc32(unsigned char *b, int n) {
   int i;
   unsigned crc = 0;
   for (i=0; i<n; i++) crc = crc32Once(crc, b[i]);
   return crc;
}

/* translate 64 bit unique id -> 48 bit psuedo unique id... */
static unsigned long long translateID(unsigned long long id) {
   unsigned long long nid = crc32((unsigned char *) &id, 8);
   nid<<=16;
   nid |= crc16((unsigned char *) &id, 8);
   return nid;
}

unsigned long long halGetBoardIDRaw(void) {
   static char isInit = 0;
   static unsigned long long id;

   if (!isInit) {
      int i;
      volatile unsigned short *flash = 
	 (volatile unsigned short *) (0x41400000);

      id = 0;
      
      /* read configuration register...
       */
      *flash = 0x90;

      /* get the protection register contents and cat them to id...
       */
      for (i=0; i<4; i++) {
	 const unsigned short pr = *(flash + (0x81+i));
         id <<= 16;
         id |= pr;
      }

      /* hash the 64 bit number... */
      id = translateID(id);

      /* back to read-array mode... */
      *flash = 0xff;

      /* we're initialized... */
      isInit = 1;
   }
   return id;
}

const char *halGetBoardID(void) {
   static char id[256];
   static int isInit = 0;
   if (!isInit) {
      int shift = 44, i = 0;
      const unsigned long long bid = halGetBoardIDRaw();
      while (shift>=0) {
         const char *digit = "0123456789abcdef";
         id[i] = digit[(bid>>shift)&0xf];
         shift -= 4;
         i++;
      }
      isInit = 1;
   }
   return id;
}

/*
 * FIXME for Gerry:
 *  what is PLL_S[01] in MUX_CONTROL?
 *  chip select DAC_CS[0-4] does this mean we can index 5 dacs or 32 dacs?
 *  same with ADC_CS?
 *  why is adc flasher separate from adc cs[0-1]
 *  default (power up) flasher value is off?
 *  default (power up) barometer value is off?
 *  default (power up) scratchpads 0?
 *  does step_ps need a pulse?  transition?  or just a write?
 */

/* chips support...
 *
 * we need to support (from gerry):
 * 
 * DACs:  MAX525,  MAX5250, LTC1257 and MAX534
 * (the max534 may get deleted from the design in January unless we discover 
 *  we need it)
 *
 * ADCs:  MAX146, LTC1286, LTC1594
 * (the LTC1594 may get replaced)
 *
 * The LTC1957 and LTC1286 are from Linear Technology. They are in the 
 * preliminary ISEG PMT HV supply.
 *
 * The MAX... parts are Maxim-ic.com or can be found at
 * http://rust.lbl.gov/~gtp/DOM/dataSheets
 * 
 * Future:
 * 
 * The MAX1139 might be used as a replacement for the max146 and ltc1594, 
 * however, it is an I2C device instead of an SPI/Microwire part.
 */
static void waitus(int us) {
   const unsigned v = *(volatile unsigned *)(REGISTERS + 0x328);
   unsigned clksperus = AHB1/1000000;
   unsigned ticks = us * clksperus;
   while ( (*(volatile unsigned *)(REGISTERS + 0x328)) - v < ticks ) ;
}

void halUSleep(int us) { waitus(us); }

/* send a up to 32 bit value to the serial port...
 * assume chips can handle 1MHz
 */
static void writeSPI(int val, int bits) {
   int mask;
   const int reg = PLDBIT(SPI_CTRL, DAC_CL);

   for (mask=(1<<(bits-1)); mask>0; mask>>=1) {
      const int dr = reg | ((val&mask) ? PLDBIT(SPI_CTRL, MOSI_DATA0) : 0);

      /* set data bit, clock low...
       */
      PLD(SPI_CTRL) = dr;
      waitus(1);

      /* clock it in...
       */
      PLD(SPI_CTRL) = dr | PLDBIT(SPI_CTRL, SERIAL_CLK);
      waitus(1);
   }

   PLD(SPI_CTRL) = reg;
}

/* read up to 32 bit value from the spi, assume chips can handle 166KHz
 *
 * data is read after falling edge...
 */
static int readSPI(int bits, int msk) {
   int mask;
   const int reg = PLDBIT(SPI_CTRL, DAC_CL);
   int ret = 0;

   PLD(SPI_CTRL) = reg;
   waitus(3);
   
   for (mask=(1<<(bits-1)); mask>0; mask>>=1) {
      PLD(SPI_CTRL) = reg | PLDBIT(SPI_CTRL, SERIAL_CLK);
      waitus(3);
      
      PLD(SPI_CTRL) = reg;
      waitus(3);

      ret = (ret<<1) | ((PLD(SPI_READ_DATA) & msk) ? 1 : 0);
   }

   /* reset...
    */
   PLD(SPI_CTRL) = reg;

   return ret;
}

/* max525: 4 12bit DACS
 *
 * load input register update dacs
 */
static void max525Write(int chan, int val) {
   /* send 16 bit serial value:
    *
    * a[1..0] c[1..0] data[11..0] (MSB first)
    */
   writeSPI( ((chan&3)<<14) | (0x3<<12) | (val&0xfff), 16);
}

/* max5250: 4 10 bit DACS
 */
static void max5250Write(int chan, int val) {
   /* send 16 bit serial value:
    *
    * a[1..0] c[1..0] data[9..0] zeros[1..0]
    */
   writeSPI( ((chan&3)<<14) | (0x3<<12) | ((val&0x3ff)<<2) , 16);
}

static void max534Write(int chan, int val) {
   /* send 12 bit serial value:
    *
    * a[1..0] c[1..0] data[7..0]
    */
   writeSPI( ((chan&3)<<10) | (0x3<<8) | (val&0xff), 12);
}

/* read from max146...
 *
 * 8 channel 12-bit ADC
 */
static int max146Read(int chan) {
   const int cw = 0x80 | ((chan&7)<<4) | 0xe;

   /* write control word...
    */
   writeSPI(cw, 8);

   /* make sure conversion is done...
    */
   waitus(10);

   /* read out results...
    */
   return readSPI(16, PLDBIT(SPI_READ_DATA, MISO_DATA_SC))>>4; 
}

/* i2c start condition...
 *
 * no assumptions about clock
 *
 * lv data low and clock high...
 */
static void startI2C(void) {
   /* make sure clock is high...
    *
    * SERIAL_CLK is in spi_ctrl
    * TEMP_SENSOR_DATA is in spi_chip_select1... 
    */
   const int creg = PLDBIT(SPI_CTRL, DAC_CL);
   const int dreg = 
      PLDBIT(SPI_CHIP_SELECT1, BASE_CS0) | PLDBIT(SPI_CHIP_SELECT1, BASE_CS1) |
      PLDBIT(SPI_CHIP_SELECT1, FLASHER_CS0);

   PLD(I2C_CONTROL1) = dreg | PLDBIT(I2C_CONTROL1, TEMP_SENSOR_DATA);
   PLD(SPI_CTRL) = creg | PLDBIT(SPI_CTRL, TEMP_SENSOR_CLK);
   waitus(2);
   
   /* drop data... */
   PLD(I2C_CONTROL1) = dreg;
   waitus(2);

   /* drop clock... */
   PLD(SPI_CTRL) = creg;
   waitus(2);
}

/* i2c stop condition...
 *
 * assume clock is low...
 *
 * lv clock and data high...
 */
static void stopI2C(void) {
   /* make sure data/clock are high...
    */
   const int creg = PLDBIT(SPI_CTRL, DAC_CL);
   const int dreg = 
      PLDBIT(SPI_CHIP_SELECT1, BASE_CS0) | PLDBIT(SPI_CHIP_SELECT1, BASE_CS1) |
      PLDBIT(SPI_CHIP_SELECT1, FLASHER_CS0);

   /* data goes low...
    */
   PLD(I2C_CONTROL1) = dreg;
   waitus(2);

   /* now clock goes high...
    */
   PLD(SPI_CTRL) = creg | PLDBIT(SPI_CTRL, TEMP_SENSOR_CLK);
   waitus(2);

   /* now raise data to signal stop...
    */
   PLD(I2C_CONTROL1) = dreg | PLDBIT(I2C_CONTROL1, TEMP_SENSOR_DATA);
   waitus(2);
}

/* i2c routines...
 *
 * to write a byte we assume:
 *   clock is low
 *   data is undefined...
 * 
 * we transition data on clock low...
 * we lv clock low...
 */
static void writeI2CByte(int val) {
   int mask = 0x80, i, ack;
   const int creg = PLDBIT(SPI_CTRL, DAC_CL);
   const int dreg = 
      PLDBIT(SPI_CHIP_SELECT1, BASE_CS0) | PLDBIT(SPI_CHIP_SELECT1, BASE_CS1) |
      PLDBIT(SPI_CHIP_SELECT1, FLASHER_CS0);

   for (i=0; i<8; i++, mask>>=1) {
      const int bit = 
	 (val&mask) ? PLDBIT(I2C_CONTROL1, TEMP_SENSOR_DATA) : 0;
      
      /* set data, raise clock and wait -- data are sample here...
       */
      PLD(I2C_CONTROL1) = dreg | bit;
      PLD(SPI_CTRL) = creg | PLDBIT(SPI_CTRL, SERIAL_CLK);
      waitus(2);

      /* drop the clock and wait...
       */
      PLD(SPI_CTRL) = creg;
      waitus(2);
   }

   /* get ack, drop clock, wait, raise clock, wait, sample...
    *
    * we're open drain so make sure temp_sensor_data is high when
    * we sample...
    */
   PLD(I2C_CONTROL1) = dreg | PLDBIT(I2C_CONTROL1, TEMP_SENSOR_DATA);
   PLD(SPI_CTRL) = creg | PLDBIT(SPI_CTRL, SERIAL_CLK);
   waitus(2);
   
   ack = RPLDBIT(I2C_DATA1, TEMP_SENSOR_DATA);
 
   /* FIXME: check ack?  should be 0
    */
   PLD(SPI_CTRL) = creg;
   waitus(2);
   
}

/* assume clock is low on entry
 *
 * we lv clock low and data high...
 */
static int readI2CByte(int ack) {
   int val = 0;
   const int creg = PLDBIT(SPI_CTRL, DAC_CL);
   const int dreg = 
      PLDBIT(SPI_CHIP_SELECT1, BASE_CS0) | PLDBIT(SPI_CHIP_SELECT1, BASE_CS1) |
      PLDBIT(SPI_CHIP_SELECT1, FLASHER_CS0);
   int i;

   /* make sure we are high Z
    */
   PLD(I2C_CONTROL1) = dreg | PLDBIT(I2C_CONTROL1, TEMP_SENSOR_DATA);
   
   for (i=0; i<8; i++) {
      /* raise clock... */
      PLD(SPI_CTRL) = creg | PLDBIT(SPI_CTRL, SERIAL_CLK);
      waitus(2);
      
      /* read data... */
      val = (val<<1) | ( RPLDBIT(I2C_DATA1, TEMP_SENSOR_DATA) ? 1 : 0 );

      /* drop clock... */
      PLD(SPI_CTRL) = creg;
      waitus(2);
   }
   
   /* send nack...
    */
   if (ack==0) PLD(I2C_CONTROL1) = dreg;

   PLD(SPI_CTRL) = creg | PLDBIT(SPI_CTRL, SERIAL_CLK);
   waitus(2);

   PLD(SPI_CTRL) = creg;
   waitus(2);

   return val;
}

/* write the ds1631 configuration register...
 */
static void writeDS1631Config(int val) {
   startI2C();
   writeI2CByte(0x90); /* control byte */
   writeI2CByte(0xac); /* command byte */
   writeI2CByte(val);  /* data */
   stopI2C();
}

/* read ds1631 configuration register...
 */
static int readDS1631Config(void) {
   int ret = 0;

   startI2C();
   writeI2CByte(0x90);  /* control byte */
   writeI2CByte(0xac);  /* command byte */
   startI2C();
   writeI2CByte(0x91);  /* control byte -- read */
   ret = readI2CByte(1); /* data */
   stopI2C();
   return ret;
}

/* ask DS1631 to perform a temperature conversion...
 */
static void convertDS1631(void) {
   startI2C();
   writeI2CByte(0x90);  /* control byte */
   writeI2CByte(0x51);  /* command byte */
   stopI2C();
}

/* get temperature from ds1631...
 */
static int readDS1631Temp(void) {
   int ret = 0;

   startI2C();
   writeI2CByte(0x90);  /* control byte */
   writeI2CByte(0xaa);  /* command byte */
   startI2C();
   writeI2CByte(0x91);  /* control byte -- read */
   ret = readI2CByte(0); /* data */
   ret = (ret<<8) | readI2CByte(1); /* data */
   stopI2C();
   return ret;
}

/* send a up to 32 bit value to the serial port...
 * assume chips can handle 1MHz
 */
static void writeBase(int val, int bits, int csmask) {
   int mask;
   const int reg = PLDBIT(SPI_CTRL, DAC_CL);

   for (mask=(1<<(bits-1)); mask>0; mask>>=1) {
      const int dr = reg | ((val&mask) ? PLDBIT(SPI_CTRL, BASE_MOSI) : 0);

      /* set data bit, clock low...
       */
      PLD(SPI_CTRL) = dr;
      waitus(1);

      /* clock it in...
       */
      PLD(SPI_CTRL) = dr | PLDBIT(SPI_CTRL, SERIAL_CLK);
      waitus(1);

      if (mask==1) {
	 /* on the last bit, load* gets asserted...
	  */
	 PLD(SPI_CHIP_SELECT1) = csmask;
	 waitus(1);
      }
   }
   PLD(SPI_CTRL) = reg;
   waitus(1);
}

static void writeLTC1257(int val) {
   PLD(SPI_CHIP_SELECT1) = PLDBIT2(SPI_CHIP_SELECT1, BASE_CS0, BASE_CS1);
   writeBase(val, 12, PLDBIT(SPI_CHIP_SELECT1, BASE_CS1));
   PLD(SPI_CHIP_SELECT1) = PLDBIT2(SPI_CHIP_SELECT1, BASE_CS0, BASE_CS1);
}

static void writeLTC1451(int val) {
   const int reg = PLDBIT(SPI_CTRL, DAC_CL);

   /* start with clock low... */
   PLD(SPI_CTRL) = reg;
   waitus(1);

   /* raise clock and cs */
   PLD(SPI_CTRL) = reg | PLDBIT(SPI_CTRL, SERIAL_CLK);
   PLD(SPI_CHIP_SELECT1) = PLDBIT2(SPI_CHIP_SELECT1, BASE_CS1, BASE_CS0);
   waitus(1);

   /* drop clock */
   PLD(SPI_CTRL) = reg;
   waitus(1);

   /* drop chip select */
   PLD(SPI_CHIP_SELECT1) = PLDBIT(SPI_CHIP_SELECT1, BASE_CS1);
   waitus(1);
   
   /* start clocking data... */
   writeBase(val, 12, PLDBIT2(SPI_CHIP_SELECT1, BASE_CS0, BASE_CS1));

   /* drop cs again... */
   PLD(SPI_CHIP_SELECT1) = PLDBIT(SPI_CHIP_SELECT1, BASE_CS1);
}

static int readLTC1286(void) {
   const int reg = PLDBIT(SPI_CTRL, DAC_CL);
   int ret;

   /* cs high...
    */
   PLD(SPI_CHIP_SELECT1) = PLDBIT2(SPI_CHIP_SELECT1, BASE_CS0, BASE_CS1);
   
   /* clock low...
    */
   PLD(SPI_CTRL) = reg;
   waitus(3);

   /* cs low...
    */
   PLD(SPI_CHIP_SELECT1) = PLDBIT(SPI_CHIP_SELECT1, BASE_CS0);
   waitus(3);

   /* read out results...
    */
   ret = readSPI(14, PLDBIT(SPI_READ_DATA, MISO_DATA_BASE));
   waitus(3);

   /* chip select goes high again...
    */
   PLD(SPI_CHIP_SELECT1) = PLDBIT2(SPI_CHIP_SELECT1, BASE_CS0, BASE_CS1);
   waitus(3);

   /* we only use bottom 12 bits... */
   return ret&0x0fff;
}

static void ows8(int b) {
   int i;
   for (i=0; i<8; i++) {
      PLD(ONE_WIRE) = ( (b>>i) & 1 ) ? 0x9 : 0xa;
      halUSleep(100);
   }
}

static void waitBusy(void) { while (RPLDBIT(ONE_WIRE, BUSY)) ; }

/* use halHVSerialRaw (when it's ready)...
 */
const char *halHVSerial(void) {
   int i;
   const char *hexdigit = "0123456789abcdef";
   static char t[64/4+1];
   static int isInit = 0;
   
   if (!isInit && hvIsPowered) {
      PLD(ONE_WIRE) = 0xf;
      waitBusy();
      
      for (i=0; i<8; i++) {
         PLD(ONE_WIRE) = ( (0x33>>i) & 1 ) ? 0x9 : 0xa;
         waitBusy();
      }
      
      for (i=0; i<64; i++) {
         PLD(ONE_WIRE) = 0xb;
         waitBusy();
         t[i/4]<<=1;
         if (RPLDBIT(ONE_WIRE, DATA)) {
            t[i/4] |= 1;
         }
      }
      
      for (i=0; i<64/4; i++) t[i] = hexdigit[(int)t[i]];
      
      isInit = 1;
   }
   
   return t;
}

unsigned long long halHVSerialRaw(void) {
   static unsigned long long id;
   static int isInit = 0;
   
   if (!isInit) {
      int i;

      PLD(ONE_WIRE) = 0xf;
      waitBusy();

      for (i=0; i<8; i++) {
         PLD(ONE_WIRE) = ( (0x33>>i) & 1 ) ? 0x9 : 0xa;
         waitBusy();
      }
      
      id = 0;
      for (i=0; i<64; i++) {
         id<<=1;
         PLD(ONE_WIRE) = 0xb;
         if (RPLDBIT(ONE_WIRE, DATA)) id |= 1;
      }
      
      isInit = 1;
   }
   
   return id;
}

int halIsFPGALoaded(void) { return RPLDBIT(MISC, FPGA_LOADED)==0; }

static unsigned char adcCSLow(int cs) { return ~(1<<(cs+5)); }
static unsigned char adcCSHigh(int cs) { return ~0; }
static unsigned char adcCLKLow(void)  { return PLDBIT(SPI_CTRL, DAC_CL); }
static unsigned char adcCLKHigh(void) { 
   return PLDBIT(SPI_CTRL, DAC_CL)|PLDBIT(SPI_CTRL, SERIAL_CLK); 
}

/* i2c start condition...
 *
 * no assumptions about clock
 *
 * lv data low and clock high...
 */
static void startMax1139(int cs) {
   /* both high... */
   PLD(SPI_CHIP_SELECT0) = adcCSHigh(cs);
   PLD(SPI_CTRL) = adcCLKHigh();
   waitus(1);
   
   /* drop data... */
   PLD(SPI_CHIP_SELECT0) = adcCSLow(cs);
   waitus(1);

   /* drop clock... */
   PLD(SPI_CTRL) = adcCLKLow();
   waitus(1);
}

/* i2c stop condition...
 *
 * assume clock is low...
 *
 * lv clock and data high...
 */
static void stopMax1139(int cs) {
   /* data goes low...
    */
   PLD(SPI_CHIP_SELECT0) = adcCSLow(cs);
   waitus(1);

   /* now clock goes high...
    */
   PLD(SPI_CTRL) = adcCLKHigh();
   waitus(1);

   /* now raise data to signal stop...
    */
   PLD(SPI_CHIP_SELECT0) = adcCSHigh(cs);
   waitus(1);
}

/* i2c routines...
 *
 * to write a byte we assume:
 *   clock is low
 *   data is undefined...
 * 
 * we transition data on clock low...
 * we lv clock low...
 */
static void writeMax1139Byte(int cs, int val) {
   int mask = 0x80, i, ack;
   for (i=0; i<8; i++, mask>>=1) {
      const int bit = 
	 (val&mask) ? adcCSHigh(cs) : adcCSLow(cs);
      
      /* set data, raise clock and wait -- data are sample here...
       */
      PLD(SPI_CHIP_SELECT0) = bit;
      PLD(SPI_CTRL) = adcCLKHigh();
      waitus(1);

      /* drop the clock and wait...
       */
      PLD(SPI_CTRL) = adcCLKLow();
      waitus(1);
   }

   /* get ack, drop clock, wait, raise clock, wait, sample...
    */
   PLD(SPI_CHIP_SELECT0) = adcCSHigh(cs);
   PLD(SPI_CTRL) = adcCLKHigh();
   waitus(1);
   
   ack = PLD(SPI_CHIP_SELECT0) & (1<<(cs+5));

   /* printf("ack: %d\r\n", ack); */

   /* FIXME: check ack?  should be 0
    */

   PLD(SPI_CTRL) = adcCLKLow();
   waitus(1);
}

/* assume clock is low on entry
 *
 * we lv clock low and data high...
 */
static int readMax1139Byte(int cs, int ack) {
   int val = 0;
   int i;

   /* make sure we are high Z
    */
   PLD(SPI_CHIP_SELECT0) = adcCSHigh(cs);
   
   for (i=0; i<8; i++) {
      /* raise clock... */
      PLD(SPI_CTRL) = adcCLKHigh();
      waitus(1);
      
      /* read data... */
      val = (val<<1) | ( (PLD(SPI_CHIP_SELECT0) & (1<<(cs+5))) ? 1 : 0 );

      /* drop clock... */
      PLD(SPI_CTRL) = adcCLKLow();
      waitus(1);
   }
   
   /* send nack...
    */
   if (ack==0) PLD(SPI_CHIP_SELECT0) = adcCSHigh(cs);
   else PLD(SPI_CHIP_SELECT0) = adcCSLow(cs);

   PLD(SPI_CTRL) = adcCLKHigh();
   waitus(1);

   PLD(SPI_CTRL) = adcCLKLow();
   waitus(1);

   return val;
}

static int max1139Read(int cs, int ch) {
   int val;

   /* set channel to readout... */
   startMax1139(cs);
   writeMax1139Byte(cs, 0x6a); /* 0110 1010 */
   writeMax1139Byte(cs, 0xD8); /* 1101 1000 */
   writeMax1139Byte(cs, (ch<<1) | 0x61); /* 0110 0000 */
   stopMax1139(cs);

   /* start readout... */
   startMax1139(cs); /* start condition... */
   writeMax1139Byte(cs, 0x6b); /* 0110 1011 */
   val = readMax1139Byte(cs, 1)&3;
   val <<= 8;
   val += (readMax1139Byte(cs, 1)&0xff);
   stopMax1139(cs);

   return val;
}

int halIsInputData(void) {
   if (halIsConsolePresent()) {
      return (*(volatile int *)(REGISTERS + 0x280))&0x1f;
   }

   return hal_FPGA_TEST_msg_ready();
}

