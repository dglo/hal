/**
 * \file hal.c, the cpld dom hal.
 */
#include <stddef.h>

#include "hal/DOM_MB_hal.h"
#include "hal/DOM_PLD_regs.h"

#include "booter/epxa.h"

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

BOOLEAN halIsSimulationPlatform() { return 0; }
USHORT halGetHalVersion() { return DOM_HAL_VERSION; }
BOOLEAN halIsConsolePresent() { 
#if defined(CPLD_ADDR)
   return RPLDBIT(UART_STATUS, SERIAL_POWER) != 0;
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

USHORT halReadADC(UBYTE channel) {
   int ret;
   const int mapping[] = { 0, 4, 1, 5, 2, 6, 3, 7 };
   
   const int cs = channel/8;
   const int dev = mapping[channel%8];
   
   /* FIXME: eventually we'll need a mutex here...
    */
   PLD(SPI_CHIP_SELECT0) = ~(1<<(5+cs));

   if (cs==0) {
      ret = max146Read(dev);
   }
   
   PLD(SPI_CHIP_SELECT0) = ~0;
   return ret;
}

static USHORT daclookup[DOM_HAL_NUM_DAC_CHANNELS];

void halWriteDAC(UBYTE channel, USHORT value) {
   const int cs = channel/4;
   const int dev = channel%4;

   if (channel>=DOM_HAL_NUM_DAC_CHANNELS) return;

   daclookup[channel] = value;

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


USHORT halReadTemp(void) { 
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

   /* wait for conversion done bit...
    */
   while ((readDS1631Config()&0x80) == 0) ;

   return readDS1631Temp();
}


static USHORT pmtlookup;

void halSetPMT_HV(USHORT value) { 
   pmtlookup = value; 
}

USHORT halReadPMT_HV() { 
   return pmtlookup;
}

static UBYTE muxlookup;

void halSelectAnalogMuxInput(UBYTE chan) {
   const int ch = chan%8;
   const int cs = (ch/4) ? 2 : 1;
   const int addr = ch%4;
   muxlookup = ch;
   PLD(MUX_CONTROL) = cs | (addr<<2);
}

void halSetSwapFlashChips() {
   PLD(BOOT_CONTROL) = 
      PLDBIT(BOOT_CONTROL, ALTERNATE_FLASH) |
      RPLDBIT(BOOT_STATUS, BOOT_FROM_FLASH);
}

void halClrSwapFlashChips() {
   PLD(BOOT_CONTROL) = RPLDBIT(BOOT_STATUS, BOOT_FROM_FLASH);
}

BOOLEAN halSwapFlashChipsState() {
   return RPLDBIT(BOOT_CONTROL, ALTERNATE_FLASH)!=0;
}

/* system control i/o
 */
void halEnableBarometer() {
   PLD(SYSTEM_CONTROL) = 
      PLD(SYSTEM_CONTROL) | PLDBIT(SYSTEM_CONTROL, BAROMETER_ENABLE);
}

void halDisableBarometer() {
   PLD(SYSTEM_CONTROL) =
      PLD(SYSTEM_CONTROL) & (~PLDBIT(SYSTEM_CONTROL, BAROMETER_ENABLE));
}

void halEnablePMT_HV() {
   /* ltc1257 requires cs0 to be low on power up...
    */
   PLD(SPI_CHIP_SELECT1) = 0; 
 
   PLD(SYSTEM_CONTROL) = 
      PLD(SYSTEM_CONTROL) | PLDBIT(SYSTEM_CONTROL, HV_PS_ENABLE);
   halUSleep(5000);

   /* chip selects are normally high...
    */
   PLD(SPI_CHIP_SELECT1) = PLDBIT2(SPI_CHIP_SELECT1, BASE_CS0, BASE_CS1); 
}

void halDisablePMT_HV() {
   PLD(SYSTEM_CONTROL) = 
      PLD(SYSTEM_CONTROL) & (~PLDBIT(SYSTEM_CONTROL, HV_PS_ENABLE));
}

void halEnableFlasher() {
   PLD(SYSTEM_CONTROL) = 
      PLD(SYSTEM_CONTROL) | PLDBIT(SYSTEM_CONTROL, FLASHER_ENABLE);
}

void halDisableFlasher() {
   PLD(SYSTEM_CONTROL) = 
      PLD(SYSTEM_CONTROL) & (~PLDBIT(SYSTEM_CONTROL, FLASHER_ENABLE));
}

BOOLEAN halFlasherState() {
   return RPLDBIT(SYSTEM_STATUS0, FLASHER_ENABLE)!=0; 
}

void halEnableLEDPS() {
}

void halDisableLEDPS() {
}

BOOLEAN halLEDPSState() { 
   return 0;
}

void halStepUpLED() {
}

void halStepDownLED() {
}

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
   if (halIsFPGALoaded()) {
      int i;

      /* FIXME: we need to check _which_ fpga is running, not
       * assume test...
       */
      hal_FPGA_TEST_request_reboot();
      
      for (i=0; i<1000 && !hal_FPGA_TEST_is_reboot_granted(); i++) 
	 halUSleep(10);
   }
   
   PLD(REBOOT_CONTROL) = PLDBIT(REBOOT_CONTROL, INITIATE_REBOOT);
}

const char *halGetBoardID(void) {
   static char id[256];
   static char isInit = 0;

   if (!isInit) {
      int i;
      volatile unsigned short *flash = 
	 (volatile unsigned short *) (0x41400000);

      /* read configuration register...
       */
      *flash = 0x90;

      /* get the protection register contents and cat them to id...
       */
      for (i=0; i<4; i++) {
	 const char *digit = "0123456789abcdef";
	 const unsigned short pr = *(flash + (0x81+i));
	 id[i*4] = digit[(pr>>12)&0xf];
	 id[i*4+1] = digit[(pr>>8)&0xf];
	 id[i*4+2] = digit[(pr>>4)&0xf];
	 id[i*4+3] = digit[(pr)&0xf];
      }

      /* back to read-array mode... */
      *flash = 0xff;

      /* we're initialized... */
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

/* read up to 32 bit value from the spi, assume chips can handle 1MHz
 *
 * data is read after falling edge...
 */
static int readSPI(int bits, int msk) {
   int mask;
   const int reg = PLDBIT(SPI_CTRL, DAC_CL);
   int ret = 0;

   PLD(SPI_CTRL) = reg;
   waitus(1);
   
   for (mask=(1<<(bits-1)); mask>0; mask>>=1) {
      PLD(SPI_CTRL) = reg | PLDBIT(SPI_CTRL, SERIAL_CLK);
      waitus(1);
      
      PLD(SPI_CTRL) = reg;
      waitus(1);

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
   waitus(1);

   /* cs low...
    */
   PLD(SPI_CHIP_SELECT1) = PLDBIT(SPI_CHIP_SELECT1, BASE_CS0);
   waitus(1);

   /* read out results...
    */
   ret = readSPI(14, PLDBIT(SPI_READ_DATA, MISO_DATA_BASE));

   /* chip select goes high again...
    */
   PLD(SPI_CHIP_SELECT1) = PLDBIT2(SPI_CHIP_SELECT1, BASE_CS0, BASE_CS1);

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

const char *halHVSerial(void) {
   int i;
   const char *hexdigit = "0123456789abcdef";
   const int sz = 64/4+1;
   char *t = (char *)malloc(sz);
   if (t==NULL) return NULL;
   memset(t, 0, sz);

   PLD(ONE_WIRE) = 0xf;
   waitBusy();

   for (i=0; i<8; i++) {
      PLD(ONE_WIRE) = ( (0x33>>i) & 1 ) ? 0x9 : 0xa;
      waitBusy();
   }

   for (i=0; i<64; i++) {
      int j;
      unsigned chk = 0;
      PLD(ONE_WIRE) = 0xb;
      waitBusy();
      t[i/4]<<=1;
      if (RPLDBIT(ONE_WIRE, DATA)) {
	 t[i/4] |= 1;
      }
   }

   for (i=0; i<64/4; i++) t[i] = hexdigit[t[i]];
   
   return t;
}

int halIsFPGALoaded(void) { return PLD(FPGA_PLD)==0x55; }
