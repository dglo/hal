/*******************************************************/
/* This header file contains defines and prototypes    */
/* used by the XSVF player code.                       */
/*******************************************************/

#ifndef FB_HAL_INCLUDE
#define FB_HAL_INCLUDE

/* TCK cycle time, in us (minimum 2, should be even) */
#define TCK_CYCLE_US   2

/*******************************************************/
/* From ports.h                                        */
/*******************************************************/

/* these constants are used to send the appropriate ports to setPort */
#define TCK (short) 0
#define TMS (short) 1
#define TDI (short) 2

/* set the port "p" (TCK, TMS, or TDI) to val (0 or 1) */
void setPort(short p, short val);

/* read the TDO bit and store it in val */
unsigned char readTDOBit();

/* Initialite the xsvf data reading */
void xsvfReadInit(int *p, int nbytes);

/* read the next byte of data from the xsvf file */
void readByte(unsigned char *data);

void waitTime(long microsec);

/*******************************************************/
/* From lenval.h                                       */
/*******************************************************/

/* the lenVal structure is a byte oriented type used to store an */
/* arbitrary length binary value. As an example, the hex value   */
/* 0x0e3d is represented as a lenVal with len=2 (since 2 bytes   */
/* and val[0]=0e and val[1]=3d.  val[2-MAX_LEN] are undefined    */

/* maximum length (in bytes) of value to read in        */
/* this needs to be at least 4, and longer than the     */
/* length of the longest SDR instruction.               */
#define MAX_LEN 7000

typedef struct var_len_byte
{
    short len;   /* number of chars in this value */
    unsigned char val[MAX_LEN+1];  /* bytes of data */
} lenVal;

/* return the long representation of a lenVal */
long value(lenVal *x);

/* set lenVal equal to value */
void initLenVal(lenVal *x, long value);

/* check if expected equals actual (taking the mask into account) */
short EqualLenVal(lenVal *expected, lenVal *actual, lenVal *mask);

/* add val1+val2 and put the result in resVal */
void addVal(lenVal *resVal, lenVal *val1, lenVal *val2);

/* return the (byte, bit) of lv (reading from left to right) */
short RetBit(lenVal *lv, int byte, int bit);

/* set the (byte, bit) of lv equal to val (e.g. SetBit("00000000",byte, 1)
   equals "01000000" */
void SetBit(lenVal *lv, int byte, int bit, short val);

/* read from XSVF numBytes bytes of data into x */
void  readVal(lenVal *x, short numBytes);

/*****************************************************************************
* From micro.h
*****************************************************************************/

/* Legacy error codes for xsvfExecute from original XSVF player v2.0 */
#define XSVF_LEGACY_SUCCESS 1
#define XSVF_LEGACY_ERROR   0

/* 4.04 [NEW] Error codes for xsvfExecute. */
/* Must #define XSVF_SUPPORT_ERRORCODES in micro.c to get these codes */
#define XSVF_ERROR_NONE         0
#define XSVF_ERROR_UNKNOWN      1
#define XSVF_ERROR_TDOMISMATCH  2
#define XSVF_ERROR_MAXRETRIES   3   /* TDO mismatch after max retries */
#define XSVF_ERROR_ILLEGALCMD   4
#define XSVF_ERROR_ILLEGALSTATE 5
#define XSVF_ERROR_DATAOVERFLOW 6   /* Data > lenVal MAX_LEN buffer size*/
/* Insert new errors here */
#define XSVF_ERROR_LAST         7

/*============================================================================
* XSVF #define
============================================================================*/

#define XSVF_VERSION    "5.01"

/*****************************************************************************
* Define:       XSVF_SUPPORT_ERRORCODES
* Description:  Define this to support the new XSVF error codes.
*               (The original XSVF player just returned 1 for success and
*               0 for an unspecified failure.)
*****************************************************************************/
#ifndef XSVF_SUPPORT_ERRORCODES
    #define XSVF_SUPPORT_ERRORCODES     1
#endif

#ifdef  XSVF_SUPPORT_ERRORCODES
    #define XSVF_ERRORCODE(errorCode)   errorCode
#else   /* Use legacy error code */
    #define XSVF_ERRORCODE(errorCode)   ((errorCode==XSVF_ERROR_NONE)?1:0)
#endif  /* XSVF_SUPPORT_ERRORCODES */

/*============================================================================
* DEBUG_MODE #define
============================================================================*/

#ifdef  DEBUG_MODE
    #define XSVFDBG_PRINTF(iDebugLevel,pzFormat) \
                { if ( xsvf_iDebugLevel >= iDebugLevel ) \
                    printf( pzFormat ); }
    #define XSVFDBG_PRINTF1(iDebugLevel,pzFormat,arg1) \
                { if ( xsvf_iDebugLevel >= iDebugLevel ) \
                    printf( pzFormat, arg1 ); }
    #define XSVFDBG_PRINTF2(iDebugLevel,pzFormat,arg1,arg2) \
                { if ( xsvf_iDebugLevel >= iDebugLevel ) \
                    printf( pzFormat, arg1, arg2 ); }
    #define XSVFDBG_PRINTF3(iDebugLevel,pzFormat,arg1,arg2,arg3) \
                { if ( xsvf_iDebugLevel >= iDebugLevel ) \
                    printf( pzFormat, arg1, arg2, arg3 ); }
    #define XSVFDBG_PRINTLENVAL(iDebugLevel,plenVal) \
                { if ( xsvf_iDebugLevel >= iDebugLevel ) \
                    xsvfPrintLenVal(plenVal); }
#else   /* !DEBUG_MODE */
    #define XSVFDBG_PRINTF(iDebugLevel,pzFormat)
    #define XSVFDBG_PRINTF1(iDebugLevel,pzFormat,arg1)
    #define XSVFDBG_PRINTF2(iDebugLevel,pzFormat,arg1,arg2)
    #define XSVFDBG_PRINTF3(iDebugLevel,pzFormat,arg1,arg2,arg3)
    #define XSVFDBG_PRINTLENVAL(iDebugLevel,plenVal)
#endif  /* DEBUG_MODE */

/*============================================================================
* XSVF Type Declarations
============================================================================*/

/*****************************************************************************
* Struct:       SXsvfInfo
* Description:  This structure contains all of the data used during the
*               execution of the XSVF.  Some data is persistent, predefined
*               information (e.g. lRunTestTime).  The bulk of this struct's
*               size is due to the lenVal structs (defined in lenval.h)
*               which contain buffers for the active shift data.  The MAX_LEN
*               #define in lenval.h defines the size of these buffers.
*               These buffers must be large enough to store the longest
*               shift data in your XSVF file.  For example:
*                   MAX_LEN >= ( longest_shift_data_in_bits / 8 )
*               Because the lenVal struct dominates the space usage of this
*               struct, the rough size of this struct is:
*                   sizeof( SXsvfInfo ) ~= MAX_LEN * 7 (number of lenVals)
*               xsvfInitialize() contains initialization code for the data
*               in this struct.
*               xsvfCleanup() contains cleanup code for the data in this
*               struct.
*****************************************************************************/
typedef struct tagSXsvfInfo
{
    /* XSVF status information */
    unsigned char   ucComplete;         /* 0 = running; 1 = complete */
    unsigned char   ucCommand;          /* Current XSVF command byte */
    long            lCommandCount;      /* Number of commands processed */
    int             iErrorCode;         /* An error code. 0 = no error. */

    /* TAP state/sequencing information */
    unsigned char   ucTapState;         /* Current TAP state */
    unsigned char   ucEndIR;            /* ENDIR TAP state (See SVF) */
    unsigned char   ucEndDR;            /* ENDDR TAP state (See SVF) */

    /* RUNTEST information */
    unsigned char   ucMaxRepeat;        /* Max repeat loops (for xc9500/xl) */
    long            lRunTestTime;       /* Pre-specified RUNTEST time (usec) */

    /* Shift Data Info and Buffers */
    long            lShiftLengthBits;   /* Len. current shift data in bits */
    short           sShiftLengthBytes;  /* Len. current shift data in bytes */

    lenVal          lvTdi;              /* Current TDI shift data */
    lenVal          lvTdoExpected;      /* Expected TDO shift data */
    lenVal          lvTdoCaptured;      /* Captured TDO shift data */
    lenVal          lvTdoMask;          /* TDO mask: 0=dontcare; 1=compare */

} SXsvfInfo;

/* Declare pointer to functions that perform XSVF commands */
typedef int (*TXsvfDoCmdFuncPtr)( SXsvfInfo* );

/*============================================================================
* XSVF Command Bytes
============================================================================*/

/* encodings of xsvf instructions */
#define XCOMPLETE        0
#define XTDOMASK         1
#define XSIR             2
#define XSDR             3
#define XRUNTEST         4
/* Reserved              5 */
/* Reserved              6 */
#define XREPEAT          7
#define XSDRSIZE         8
#define XSDRTDO          9
#define XSETSDRMASKS     10
#define XSDRINC          11
#define XSDRB            12
#define XSDRC            13
#define XSDRE            14
#define XSDRTDOB         15
#define XSDRTDOC         16
#define XSDRTDOE         17
#define XSTATE           18         /* 4.00 */
#define XENDIR           19         /* 4.04 */
#define XENDDR           20         /* 4.04 */
#define XSIR2            21         /* 4.10 */
#define XCOMMENT         22         /* 4.14 */
#define XWAIT            23         /* 5.00 */
/* Insert new commands here */
/* and add corresponding xsvfDoCmd function to xsvf_pfDoCmd below. */
#define XLASTCMD         24         /* Last command marker */


/*============================================================================
* XSVF Command Parameter Values
============================================================================*/

#define XSTATE_RESET     0          /* 4.00 parameter for XSTATE */
#define XSTATE_RUNTEST   1          /* 4.00 parameter for XSTATE */

#define XENDXR_RUNTEST   0          /* 4.04 parameter for XENDIR/DR */
#define XENDXR_PAUSE     1          /* 4.04 parameter for XENDIR/DR */

/* TAP states */
#define XTAPSTATE_RESET     0x00
#define XTAPSTATE_RUNTEST   0x01    /* a.k.a. IDLE */
#define XTAPSTATE_SELECTDR  0x02
#define XTAPSTATE_CAPTUREDR 0x03
#define XTAPSTATE_SHIFTDR   0x04
#define XTAPSTATE_EXIT1DR   0x05
#define XTAPSTATE_PAUSEDR   0x06
#define XTAPSTATE_EXIT2DR   0x07
#define XTAPSTATE_UPDATEDR  0x08
#define XTAPSTATE_IRSTATES  0x09    /* All IR states begin here */
#define XTAPSTATE_SELECTIR  0x09
#define XTAPSTATE_CAPTUREIR 0x0A
#define XTAPSTATE_SHIFTIR   0x0B
#define XTAPSTATE_EXIT1IR   0x0C
#define XTAPSTATE_PAUSEIR   0x0D
#define XTAPSTATE_EXIT2IR   0x0E
#define XTAPSTATE_UPDATEIR  0x0F

/*============================================================================
* XSVF Function Prototypes
============================================================================*/

int xsvfDoIllegalCmd( SXsvfInfo* pXsvfInfo );   /* Illegal command function */
int xsvfDoXCOMPLETE( SXsvfInfo* pXsvfInfo );
int xsvfDoXTDOMASK( SXsvfInfo* pXsvfInfo );
int xsvfDoXSIR( SXsvfInfo* pXsvfInfo );
int xsvfDoXSIR2( SXsvfInfo* pXsvfInfo );
int xsvfDoXSDR( SXsvfInfo* pXsvfInfo );
int xsvfDoXRUNTEST( SXsvfInfo* pXsvfInfo );
int xsvfDoXREPEAT( SXsvfInfo* pXsvfInfo );
int xsvfDoXSDRSIZE( SXsvfInfo* pXsvfInfo );
int xsvfDoXSDRTDO( SXsvfInfo* pXsvfInfo );
int xsvfDoXSETSDRMASKS( SXsvfInfo* pXsvfInfo );
int xsvfDoXSDRINC( SXsvfInfo* pXsvfInfo );
int xsvfDoXSDRBCE( SXsvfInfo* pXsvfInfo );
int xsvfDoXSDRTDOBCE( SXsvfInfo* pXsvfInfo );
int xsvfDoXSTATE( SXsvfInfo* pXsvfInfo );
int xsvfDoXENDXR( SXsvfInfo* pXsvfInfo );
int xsvfDoXCOMMENT( SXsvfInfo* pXsvfInfo );
int xsvfDoXWAIT( SXsvfInfo* pXsvfInfo );
/* Insert new command functions here */

#endif

