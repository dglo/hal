/*
 * \file emu.c, emulate the dom hal.  Should be useful for
 * both the eval board and a standalone emulation dom mb 
 * emulation program
 */
#include <unistd.h>
#include <time.h>
#include <errno.h>

#include "hal/DOM_MB_hal_simul.h"

// define true and false
#define TRUE 1
#define FALSE 0

// storage for DACs and ADCs. used for simulation
static USHORT DAC[DOM_HAL_NUM_DAC_CHANNELS]={0,0,0,0,0,0,0,0};
static USHORT ADC[DOM_HAL_NUM_ADC_CHANNELS]={0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0};
static USHORT PMT_HV=0;
static BOOLEAN PMT_HV_ENABLE=FALSE;

BOOLEAN halIsSimulationPlatform() { return 1; }
USHORT halGetHalVersion() { return 1; }
BOOLEAN halIsConsolePresent() { return 0; }

static BOOLEAN flashboot = 0;

void halSetFlashBoot() { flashboot = 1; }
void halClrFlashBoot() { flashboot = 0; }

BOOLEAN halFlashBootState() { return flashboot; }

USHORT halReadADC(UBYTE channel) { 
    if(channel >= DOM_HAL_NUM_ADC_CHANNELS) return 0xffff;
    return ADC[channel]; 
}

void halWriteDAC(UBYTE channel, USHORT value) {
   if (channel>=DOM_HAL_NUM_DAC_CHANNELS) return;
   DAC[channel] = value&0xfff;
}

USHORT halReadDAC(UBYTE channel) {
   if (channel>=DOM_HAL_NUM_DAC_CHANNELS) return 0;
   return DAC[channel];
}

void halEnableBarometer() {;}
void halDisableBarometer() {;}
USHORT halReadTemp() { return 100; }

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
char *halGetBoardID(void) {
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
hal_FPGA_TEST_atwd_readout_done(int chip) {
    if(chip == 0) return ATWD0_done;
    else return ATWD1_done;

}

int
hal_FPGA_TEST_atwd_readout(short *ch0, short *ch1, short *ch2, short *ch3,
	int max, int chip) {
    int i;

    if(max > 128) max = 128;
    for(i = 0; i < max; i++) {
	ch0[i]=i%4;
	ch1[i]=i%8;
	ch2[i]=i%12;
	ch3[i]=i%16;
    }
    if(chip == 0) ATWD0_done = FALSE;
    else ATWD1_done = FALSE;
    return max;
}

int
hal_FPGA_TEST_fadc_readout(short *fadc, int max) {
    int i;
 
    if(max > 256) max = 256;
    for(i = 0; i < max; i++) {
        fadc[i]=i;
    }
    return max;
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
