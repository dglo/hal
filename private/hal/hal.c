/*
 * \file emu.c, emulate the dom hal.  Should be useful for
 * both the eval board and a standalone emulation dom mb 
 * emulation program
 */
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <sys/poll.h>

#include "hal/DOM_MB_hal_simul.h"

// define true and false
#define TRUE 1
#define FALSE 0

// storage for DACs and ADCs. used for simulation
static USHORT DAC[DOM_HAL_NUM_DAC_CHANNELS]={0,0,0,0,0,0,0,0};
static USHORT ADC[DOM_HAL_NUM_ADC_CHANNELS]; 
/* ={0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0}; */
static USHORT PMT_HV=0;
static BOOLEAN PMT_HV_ENABLE=FALSE;

BOOLEAN halIsSimulationPlatform() { return 1; }
USHORT halGetVersion() { return 1; }
USHORT halGetHWVersion() { return 1; }
USHORT halGetBuild() { return 1; }
USHORT halGetHWBuild() { return 1; }
BOOLEAN halIsConsolePresent() { return 0; }

static BOOLEAN flashboot = 0;

void halSetFlashBoot() { flashboot = 1; }
void halClrFlashBoot() { flashboot = 0; }

BOOLEAN halFlashBootState() { return flashboot; }

USHORT halReadADC(UBYTE channel) { 
    if(channel >= DOM_HAL_NUM_ADC_CHANNELS) return 0xffff;
    return ADC[channel]; 
}

void halWriteDAC(UBYTE channel, int value) {
   if (channel>=DOM_HAL_NUM_DAC_CHANNELS) return;
   DAC[channel] = value&0xfff;
}

USHORT halReadDAC(UBYTE channel) {
   if (channel>=DOM_HAL_NUM_DAC_CHANNELS) return 0;
   return DAC[channel];
}

void halEnableBarometer() {;}
void halDisableBarometer() {;}
void halStartReadTemp() { }
int halReadTempDone(void) { return 1; }

USHORT halFinishReadTemp() { return 100; }
USHORT halReadTemp() { return 100; }

void halPowerUpBase(void) {}
void halPowerDownBase(void) {}
void halEnableBaseHV(void) {}
void halDisableBaseHV(void) {}

void halEnablePMT_HV() { PMT_HV_ENABLE=TRUE; }
void halDisablePMT_HV() { PMT_HV_ENABLE=FALSE; }
BOOLEAN halPMT_HVisEnabled() {return PMT_HV_ENABLE; }

void halSetPMT_HV(USHORT value) { PMT_HV = value&0xfff; }
void halWriteActiveBaseDAC(USHORT value) { PMT_HV = value&0xfff; }
void halWritePassiveBaseDAC(USHORT value) { PMT_HV = value&0xfff; }
USHORT halReadPMT_HV() { return PMT_HV; }
USHORT halReadBaseADC(void) { return PMT_HV;}
USHORT halReadBaseDAC(void) { return PMT_HV;}
void halWriteBaseDAC(USHORT value) {PMT_HV = value;}

static UBYTE muxlookup;

void halSelectAnalogMuxInput(UBYTE channel) { muxlookup = channel; }

static BOOLEAN swapflash = 0;

void halSetSwapFlashChips() { swapflash = 1; }
void halClrSwapFlashChips() { swapflash = 0; }
BOOLEAN halSwapFlashChipsState() { return swapflash; }

static BOOLEAN flasher = 0;
void halEnableFlasher() { flasher = 1; }
void halDisableFlasher() { flasher = 0; }
BOOLEAN halFlasherState() { return flasher; }

static BOOLEAN ledps = 0;
void halEnableLEDPS() { ledps = 1; }
void halDisableLEDPS() { ledps = 0; }
BOOLEAN halLEDPSState() { return ledps; }

void halStepUpLED() {}
void halStepDownLED() {}

BOOLEAN serialdsr = 0;

BOOLEAN halIsSerialDSR() { return 0; }
BOOLEAN halIsSerialReceiveData() { return 0; }
BOOLEAN halIsSerialTransmitData() { return 1; }
void halEnableSerialDSR() { serialdsr = 1; }
void halDisableSerialDSR() { serialdsr = 0; }
BOOLEAN halSerialDSRState() { return serialdsr; }

void halBoardReboot() {
   exit(0);
}

UBYTE boardID[6]={0,0,0,0,0,0};
const char *halGetBoardID(void) {
    // make string static to keep it around after returning
    static char ID[13];
    int i;
    UBYTE b;
    for (i=0;i<6;i++) {
	b = ((boardID[i]>>4) & 0xf) + 0x30;
	if (b > 0x39) {
	    ID[i*2] = b + 0x7;
	}
	else {
	    ID[i*2]=b;
	}
	b = (boardID[i] & 0xf) + 0x30;
	if (b > 0x39) {
	    ID[i*2+1] = b + 0x7;
	} else {
	    ID[i*2+1]=b;
	}
    }
    // make sure we null terminate the string;
    ID[12]=0;

    return ID;
}
void halSetBoardID(char *ID) { 
    int i;
    UBYTE b;
    char temp[32];

    if(strlen(ID) < 12) {
	// ID is too short, make something up
	strcpy(temp, "123456123456");
    } else if(strlen(ID) > 12) {
	// ID is too long, make something up
	strcpy(temp, "123456123456");
    } else {
	strcpy(temp, ID);
    }


    for (i=0;i<6;i++) {
	b=(UBYTE)temp[i*2];

	// make sure its upper case.
	if(b > 0x60) {
	    b-=0x20;
	}
	// deal with hex digits
   	if(b > 0x40) {
	    b-=0x37;
	} else {
	    b-=0x30;
	}
	boardID[i]=b<<4;
    	
	b=(UBYTE)temp[i*2+1];
	if(b >0x60) {
	    b-=0x20;
	}
	if(b > 0x40) {
	    b-=0x37;
	} else {
	    b-=0x30;
	}
	boardID[i]+=b;
    }
}

char boardName[128]="none";
char *halGetBoardName(void) {return boardName; }
void halSetBoardName(char *name) { strcpy(boardName,name);}

void halUSleep(int us) { 
   struct timespec ts, rm;

   ts.tv_sec = us/1000000;
   ts.tv_nsec = (us%1000000)*1000;

   while (1) {
      if (nanosleep(&ts, &rm)<0 && errno==EINTR) {
	 ts = rm;
      }
      else return;
   }
   
   /* usleep(us); */
}

UBYTE *bufferBaseAddr;
int bufferMask;
pthread_mutex_t *FPGAsimulMutex;
int readoutState;
ULONG readoutIndex;
BOOLEAN readoutEnabled;
int readoutMode;

void
halInitReadoutIface(UBYTE *base, int mask) {
    // get the simulation lock
    pthread_mutex_lock(FPGAsimulMutex);

    bufferBaseAddr = base;
    bufferMask = mask;
    readoutState = READOUT_STATE_STOPPED;
    readoutIndex = 0;
    readoutEnabled = FALSE;

    // release it
    pthread_mutex_unlock(FPGAsimulMutex);
}

void
halSimulInitReadoutIface(pthread_mutex_t *mutex) {
    FPGAsimulMutex=mutex;
}

void
halResetReadoutIface(void) {
    // get the simulation lock
    pthread_mutex_lock(FPGAsimulMutex);

    readoutIndex = 0;
    //printf("halResetReadoutIface: \nbase: %x\nlen: %d\nnumBuf: %d\n",
	//bufferBaseAddr, bufferByteLen, numReadoutBuffers);
    //printf("state: %d, \nindex: %d, \nenabled: %d\n",
	//readoutState, readoutIndex, readoutEnabled);

    // release it
    pthread_mutex_unlock(FPGAsimulMutex);
}

int
halGetBufferElementLen(void) {
    return BUFFER_ELEMENT_LEN;
}

//int
//halGetBufferMask(void) {
//    return bufferMask;
//}

ULONG
halGetReadoutIndex(void) {
    ULONG temp;

    // get the simulation lock
    pthread_mutex_lock(FPGAsimulMutex);

    temp = readoutIndex & bufferMask;

    // release it
    pthread_mutex_unlock(FPGAsimulMutex);

    return temp;
}

ULONG
halGetReadoutEventIndex(void) {
    ULONG temp;

    // get the simulation lock
    pthread_mutex_lock(FPGAsimulMutex);

    temp = readoutIndex;

    // release it
    pthread_mutex_unlock(FPGAsimulMutex);

    return temp;
}

void
halDisableReadoutIface(void) {
    // get the simulation lock
    pthread_mutex_lock(FPGAsimulMutex);

    readoutEnabled = FALSE;

    // release it
    pthread_mutex_unlock(FPGAsimulMutex);
}

void
halEnableReadoutIface(void) {
    // get the simulation lock
    pthread_mutex_lock(FPGAsimulMutex);

    readoutEnabled = TRUE;
    readoutState = READOUT_STATE_RUNNING;

    // release it
    pthread_mutex_unlock(FPGAsimulMutex);
}

BOOLEAN
halIsReadoutIfaceEnabled(void) {
    return readoutEnabled;
}

void
halSetReadoutIfaceFillOnce(void) {
    // get the simulation lock
    pthread_mutex_lock(FPGAsimulMutex);

    readoutMode = READOUT_FILL_ONCE;

    // release it
    pthread_mutex_unlock(FPGAsimulMutex);
}

void
halSetReadoutIfaceWrap(void) {
    // get the simulation lock
    pthread_mutex_lock(FPGAsimulMutex);

    readoutMode = READOUT_WRAP;

    // release it
    pthread_mutex_unlock(FPGAsimulMutex);
}

int
halGetReadoutIfaceState(void) {
    return readoutState;
}

void
halResumeReadoutIface(void) {
    // get the simulation lock
    pthread_mutex_lock(FPGAsimulMutex);

    if(readoutState == READOUT_STATE_PAUSED) {
	readoutState = READOUT_STATE_RUNNING;
    }

    // release it
    pthread_mutex_unlock(FPGAsimulMutex);
}


UBYTE *
halSimulGetCurReadoutIfaceAddr(void) {
    UBYTE *temp;

    // get the simulation lock
    pthread_mutex_lock(FPGAsimulMutex);

    temp = bufferBaseAddr + (BUFFER_ELEMENT_LEN * 
	(readoutIndex & bufferMask));

    // release it
    pthread_mutex_unlock(FPGAsimulMutex);

    return temp;
}

UBYTE *
halGetReadoutIfaceAddr(ULONG index) {
    return bufferBaseAddr + (BUFFER_ELEMENT_LEN *
	(index & bufferMask));
}

void
halSimulIncReadoutIndex(void) {
    // get the simulation lock
    pthread_mutex_lock(FPGAsimulMutex);

    // only act if we are enabled
    if(readoutEnabled) {
	// only act if we are running
	if(readoutState == READOUT_STATE_RUNNING) {
	    readoutIndex++;
	    // check for wrap around
	    if((readoutIndex & bufferMask) == 0) {
		readoutIndex &= ~bufferMask;
		// pause if in fill once mode
		if(readoutMode == READOUT_FILL_ONCE) {
		    readoutState = READOUT_STATE_PAUSED;
		}
	    }
	}
	if(readoutMode == READOUT_FILL_ONCE) {
	    readoutState = READOUT_STATE_PAUSED;
	}
    }

    // release it
    pthread_mutex_unlock(FPGAsimulMutex);
}

int
hal_FPGA_TEST_send(int type, int len, const char *msg) {
    return 1;
}

int
hal_FPGA_TEST_receive(int *type, int *len, char *msg) {
    return 1;
}

long long hal_FPGA_getClock() {
    return (long long)time(NULL);
}

BOOLEAN ATWD0_done = FALSE;
BOOLEAN ATWD1_done = FALSE;

BOOLEAN
hal_FPGA_TEST_readout_done(int mask) {
    return TRUE;
}

int
hal_FPGA_TEST_readout(short *ch0_0, short *ch1_0, short *ch2_0, short *ch3_0,
	short *ch0_1, short *ch1_1, short *ch1_2, short *ch3_1, int atwd_max,
	short *fadc, int fadc_max,
	int mask) {
   return 0;
}

void 
hal_FPGA_TEST_trigger_forced(int chip) {
    if(chip == 0) ATWD0_done = TRUE;
    else ATWD1_done = TRUE;
}

void
hal_FPGA_TEST_trigger_disc(int chip) {
    if(chip == 0) ATWD0_done = TRUE;
    else ATWD1_done = TRUE;

}


const char *halHVSerial(void) { return "flash-serial"; }

void
hal_FPGA_TEST_set_pulser_rate(DOM_HAL_FPGA_PULSER_RATES rate){}

void
hal_FPGA_TEST_enable_pulser(void){}
 
void
hal_FPGA_TEST_disable_pulser(void){}

int
hal_FPGA_TEST_get_spe_rate(void) {
    return 1234;
}

int
hal_FPGA_TEST_get_mpe_rate(void) {
    return 2345;
}

unsigned long long 
hal_FPGA_TEST_get_local_clock(void) { return 123456;}

int halIsFPGALoaded(void) { return 0; }

void hal_FPGA_TEST_request_reboot(void){}
int hal_FPGA_TEST_is_reboot_granted(void) { return 1; }

void hal_FPGA_TEST_enable_ping_pong(void) {}
unsigned long long hal_FPGA_TEST_get_atwd0_clock(void) {return 0ULL; }
unsigned long long hal_FPGA_TEST_get_atwd1_clock(void) { return 0ULL; }
void hal_FPGA_TEST_readout_ping_pong(short *ch0, short *ch1, 
                                     short *ch2, short *ch3,
                                     int max, short ch_mask) {
}
unsigned long long hal_FPGA_TEST_get_ping_pong_clock(void) {
   return 0ULL;
}

void hal_FPGA_TEST_readout_ping_pong_done(void) {}
void hal_FPGA_TEST_disable_ping_pong(void) {
}
int
hal_FPGA_query_versions(DOM_HAL_FPGA_TYPES type, unsigned comps) { 
   return -1;
}

void hal_FPGA_TEST_clear_trigger(void) {
}

void hal_FPGA_TEST_trigger_LED(int trigger_mask){}
void hal_FPGA_TEST_enable_LED(void){}
void hal_FPGA_TEST_disable_LED(void){}
void hal_FPGA_TEST_set_atwd_LED_delay(int delay){}

int halIsInputData(void) {
   struct pollfd fds[1];
   fds[0].fd = 0;
   fds[0].events = POLLIN;
   return poll(fds, 1, 0)==1;
}

unsigned long long halGetBoardIDRaw(void) { return 0ULL; }
unsigned long long halHVSerialRaw(void) { return 0ULL; }
void hal_FPGA_TEST_set_scalar_period(DOM_HAL_FPGA_SCALAR_PERIODS ms) {}
void hal_FPGA_TEST_init_state(void){}
void hal_FPGA_TEST_set_deadtime(int ns){}
DOM_HAL_FPGA_TYPES hal_FPGA_query_type(void){ 
   return DOM_HAL_FPGA_TYPE_INVALID; 
}
void hal_FPGA_TEST_start_FB_flashing(void) {}
void hal_FPGA_TEST_stop_FB_flashing(void) {}

void hal_FB_enable(void) {}
void hal_FB_disable(void) {}
const char * hal_FB_get_serial(void) {return "deadbeefdeadbeef";}
USHORT hal_FB_get_fw_version(void) {return 0;}
USHORT hal_FB_get_hw_version(void) {return 0;}
void hal_FB_set_pulse_width(UBYTE value) {}
void hal_FB_set_brightness(UBYTE value) {}
void hal_FB_enable_LEDs(USHORT enables) {}
void hal_FB_select_mux_input(UBYTE value) {}
int hal_FB_xsvfExecute(int *p, int nbytes) {return 0;}

