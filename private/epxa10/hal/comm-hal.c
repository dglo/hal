/**
 * \file fpga-comm.c, the fpga dom hal for communications.
 */
#include <stddef.h>
#include <stdio.h>
#include <math.h>
#include <string.h>

#include "hal/DOM_MB_fpga.h"
#include "hal/DOM_MB_pld.h"
#include "DOM_FPGA_regs.h"

#include "dom-fpga/fpga-versions.h"

#include "booter/epxa.h"

/* debugging -> insert bit errors... */
#define INSERTBITERRORS
#undef INSERTBITERRORS

/* debugging -> serial */
#define DEBUGSERIAL
#undef  DEBUGSERIAL

#if defined(DEBUGSERIAL)
/* in loader/exceptions.c */
extern void writeSerialDebug(const char *);
static int debugIndent;

static void writeDebug(const char *ptr) {
   char msg[80];
   if (debugIndent>0 && debugIndent<sizeof(msg)-1) {
      memset(msg, 0, sizeof(msg));
      memset(msg, '\t', debugIndent);
      writeSerialDebug(msg);
   }
   writeSerialDebug(ptr);
   writeSerialDebug("\r\n");
}

static void writeDebugIndent(const char *ptr) {
   writeDebug(ptr);
   debugIndent++;
}

static void debugUnindent(void) {
   if (debugIndent>=1) {
      debugIndent--; 
   }
   else {
      writeDebug("unindent too far!!");
   }
}

#else

#define writeDebug(a)
#define writeDebugIndent(a)
#define debugUnindent()

#endif

/* should we do software error correction? */
#undef  SWERRORCORRECTION
#define SWERRORCORRECTION

#if defined(SWERRORCORRECTION)
/* should we do software packet assembly? */
# undef  SWPACKETASSEMBLY
# define SWPACKETASSEMBLY
#endif

/* tx queue size in bytes */
#define HWTXQUEUESIZE (500+4)
#define HWRXQUEUESIZE (500+4)

/* tx max hardware packet size */
#define HWTXMAXPACKETSIZE 500

/* rx max hardware packet size */
#define HWRXMAXPACKETSIZE 500

/* max packet size from sw */
#if defined(SWPACKETASSEMBLY)
# define MAXSWPKTSIZE (4096 - (8+4)) /* make sure packet -> page... */
#else
# if defined(SWERRORCORRECTION)
#  define MAXSWPKTSIZE (500 - (8+4))
# else
#  define MAXSWPKTSIZE 500
# endif
#endif

static int hasDualPortedComm() {
   const unsigned *rom = (const unsigned *) DOM_FPGA_VERSIONING;
   return rom[FPGA_VERSIONS_COM_DP]!=0;
}

static short hwQueueRemaining;

/* is there space for a hw tx packet? */
static int isHWPktSpace(int len) {
   if (RFPGABIT(TEST_COM_STATUS, TX_FIFO_ALMOST_FULL)==0) 
      hwQueueRemaining = HWTXQUEUESIZE;
   return len+4 <= hwQueueRemaining;
}

static int hal_FPGA_TEST_hwsend(int type, int len, const char *msg) {
   int i;

   if (len>4096) return 1;
   
   if (!hasDualPortedComm()) {
      /* wait for comm to become avail... */
      while (!RFPGABIT(TEST_COM_STATUS, AVAIL)) 
         writeDebug("hwsend: comm not avail!");

      /* wait for Tx fifo almost full to be low */
      while (!isHWPktSpace(len)) ;
      hwQueueRemaining -= len + 4;

      /* send data */
      FPGA(TEST_COM_TX_DATA) = type&0xff;
      FPGA(TEST_COM_TX_DATA) = (type>>8)&0xff;  
      FPGA(TEST_COM_TX_DATA) = len&0xff; 
      FPGA(TEST_COM_TX_DATA) = (len>>8)&0xff; 
      
      for (i=0; i<len; i++) {
#if defined(INSERTBITERRORS)
         char v = msg[i];
         static int flipit;
         flipit++;
         if (flipit==50000) {
            v ^= 1;
            flipit=0;
         }
         FPGA(TEST_COM_TX_DATA) = v;
#else
         FPGA(TEST_COM_TX_DATA) = msg[i];
#endif
      }
   }
   else {
      unsigned const volatile *tx_dpr_raddr = 
	 (unsigned const volatile *) 0x90081070;
      unsigned const volatile *comm_status = 
	 (unsigned const volatile *) 0x90081034;
      unsigned char *tx_buf = (unsigned char *) 0x80000000;
      unsigned short waddr = 
	 (unsigned short) (*(unsigned volatile *) 0x90081070);

      /* dual port ram interface... 
       */
      
      /* wait for comm avail... */
      while ( (*comm_status & 0x40) == 0 ) ;
      
      /* wait for the proper amount of space left... */
      while ( ((unsigned short)16*1024) - 
	      (waddr - (unsigned short)*tx_dpr_raddr) < 
	      (unsigned short) (len+4)) ;
      
      /* send the header... */
      tx_buf[4*(waddr%(16*1024))] = type&0xff; waddr++;
      tx_buf[4*(waddr%(16*1024))] = (type>>8)&0xff; waddr++;
      tx_buf[4*(waddr%(16*1024))] = len&0xff; waddr++;
      tx_buf[4*(waddr%(16*1024))] = (len>>8)&0xff; waddr++;
      
      /* send the data... */
      for (i=0; i<len; i++) {
	 tx_buf[4*(waddr%(16*1024))] = (unsigned char) msg[i]; 
	 waddr++;
      }
      
      /* write the pointer back (this will toggle pkt_ready)... */
      *(unsigned volatile *) 0x90081070 = (unsigned) waddr;
   }

   return 0;
}

int hal_FPGA_TEST_hwmsg_ready(void) {
   /* wait for comm avail... */
   while (!RFPGABIT(TEST_COM_STATUS, AVAIL)) 
      writeDebug("msg_ready: comm not avail");

   /* return message ready... */
   return RFPGABIT(TEST_COM_STATUS, RX_MSG_READY);
}

static int hal_FPGA_TEST_hwreceive(int *type, int *len, char *msg) {
   int i;
   unsigned bytes[2];

   if (!hasDualPortedComm()) {
      unsigned reg;
      
      /* wait for msg */
      while (!hal_FPGA_TEST_hwmsg_ready()) ;

      /* read it */
      bytes[0] = FPGA(TEST_COM_RX_DATA)&0xff;
      bytes[1] = FPGA(TEST_COM_RX_DATA)&0xff;
      *type = (bytes[1]<<8) | bytes[0];

      bytes[0] = FPGA(TEST_COM_RX_DATA)&0xff;
      bytes[1] = FPGA(TEST_COM_RX_DATA)&0xff;
      *len = (bytes[1]<<8) | bytes[0];
      
      for (i=0; i<*len; i++) msg[i] = FPGA(TEST_COM_RX_DATA)&0xff;

      reg = FPGA(TEST_COM_CTRL);
      FPGA(TEST_COM_CTRL) = reg | FPGABIT(TEST_COM_CTRL, RX_DONE);
      FPGA(TEST_COM_CTRL) = reg & (~FPGABIT(TEST_COM_CTRL, RX_DONE));
   }
   else {
      unsigned const volatile *comm_status = 
	 (unsigned const volatile *) 0x90081034;
      unsigned char *rx_buf = (unsigned char *) (0x80000000 + 1024*16*4);
      unsigned short raddr = 
	 (unsigned short) (*(unsigned volatile *) 0x90081078);

      /* wait for msg */
      while ( (*comm_status & 0x08) == 0 ) ;
      
      /* get header */
      bytes[0] = rx_buf[4*(raddr%(16*1024))]; raddr++;
      bytes[1] = rx_buf[4*(raddr%(16*1024))]; raddr++;
      *type = (bytes[1]<<8) | bytes[0];
      
      bytes[0] = rx_buf[4*(raddr%(16*1024))]; raddr++;
      bytes[1] = rx_buf[4*(raddr%(16*1024))]; raddr++;
      *len = (bytes[1]<<8) | bytes[0];
      
      /* get data */
      for (i=0; i<*len; i++) {
	 msg[i] = (char) (rx_buf[4*(raddr%(16*1024))]); 
	 raddr++;
      }
      
      /* write back pointer -- rx_done toggled inside fpga */
      *(unsigned volatile *) 0x90081078 = (unsigned) raddr;
   }

   return 0;
}

#if defined(SWERRORCORRECTION)

#define STATE_UNCONNECTED 0
#define STATE_CONNECTED 1

static int state = STATE_UNCONNECTED;

/* we use timer1 here to generate rtticks...
 */
static void initTicker(void) {
   static int isInit = 0;
   
   if (!isInit) {
      *((volatile unsigned *) 0x7fffc240) = 0;    /* free running, off */
      *((volatile unsigned *) 0x7fffc260) = 0xffffffff;  /* limit max... */
      /* set the period to the expected round trip time... */
      *((volatile unsigned *) 0x7fffc250) = 
         (unsigned) ( (AHB1/2) * 2e-3 );
      *((volatile unsigned *) 0x7fffc240) = 0x10; /* free running, go! */
      isInit = 1;
   }
}

static inline unsigned getTicks(void) { 
   initTicker();
   return *(volatile unsigned *) 0x7fffc270;
}

/* prepended to all packets... */
typedef struct PktHdrStruct {
   unsigned short flags; /* bit 0: acq, bit 1: syn_fin */
   unsigned short type;  /* kalle type field... */
   unsigned short len;   /* payload length in bytes */
   unsigned short seqn;  /* sequence number */
} PktHdr;

/* convert payload length (PktHdr->len) to hardware packet length... 
 *
 * sizeof(PktHdr) + (payload length padded to 32 bits) + (32 bit crc)
 */
static inline int hwPktLen(int swlen) { 
   return sizeof(PktHdr) + ((swlen+3)/4)*4 + 4;
}

/* read queue size 
 *
 * the read queue has to be able to hold at least an
 * entire software packet.  we give it space for two
 * just in case (too much?)...
 */
#define RDQUEUEDATASIZE (2*(MAXSWPKTSIZE + 4 + sizeof(PktHdr) + 4))
#define RDQUEUEDATALEN  (RDQUEUEDATASIZE/sizeof(unsigned))

/* tx data size.
 *
 * the tx data buffer must have enough room to hold data until
 * we expect an acq back...
 */
#define TXPKTDATASIZE (8*(HWTXQUEUESIZE + HWRXQUEUESIZE))
#define TXPKTDATALEN  (TXPKTDATASIZE/sizeof(unsigned))

/* length of retransmit queue
 *
 * we want to make sure that we won't fail an allocation
 * unless there is no tx data too...
 */
#define WRPKTQUEUELEN (TXPKTDATASIZE/(4 + sizeof(PktHdr) + 4 + 4))

/* we keep one copy of this guy around
 * to pass back to the dor for compiling
 * statistics...
 */
static struct DomStatsPkt {
   PktHdr hdr;
   int controlType;
   int nBadFins;
   int minRxQueueSize; /* smallest Rx queue avail size in bytes */
   int maxRetxEntries; /* biggest number of used Retx buffer entries... */
   int minAckQueueEntries; /* smallest number of ack queue entries... */
   int nTxPkts;
   int nTxAckPkts;
   int nTxReackPkts;
   int nTxDataPkts;
   int nTxResentPkts;
   int nTxControlPkts;
   int nRxPkts;
   int nRxDataPkts;
   int nRxGoodDataPkts;
   int nRxAckPkts;
   int nRxDupDataPkts;
   int nRxDupAcks;
   int nRxGoodAcks;
   int nRxControlPkts;
   int nRxDroppedPkts;
   int nRxBadPkts;
   int nInvalidPostIC; /* number of invalid packets past ic... */
   unsigned crc;
} domStatsPkt;

/* the retx list is a linked list, with a pointer
 * to the head and tail so that we can take stuff
 * out and put stuff in quickly...
 */
typedef struct RetxEltStruct {
   struct RetxEltStruct *next;
   unsigned *pkt;  /* the packet to retx, set to NULL when done */
   unsigned ticks; /* the getTicks() at the time of last transmit */
} RetxElt;

static RetxElt retxQ[WRPKTQUEUELEN];
static RetxElt *retxHead, *retxTail, *retxFree;

static void retxInit(int force) {
   /* we need to initialize the free list -- first time only... */
   if (force || (retxHead==NULL && retxTail==NULL && retxFree==NULL)) {
      int i;
      const int n = sizeof(retxQ)/sizeof(retxQ[0]);
      /* clear it, this may not be the first time... */
      memset(retxQ, 0, sizeof(retxQ));
      retxHead = retxTail = NULL;
      retxFree = retxQ;
      for (i=0; i<n-1; i++) retxQ[i].next = retxQ + (i+1);
      retxQ[n-1].next = NULL;
   }
}

/* allocate from the free list... */
static RetxElt *retxAlloc(void) {
   retxInit(0);

   /* collect stats... */
   {  RetxElt *elt;
      int ne = 0;
      for (elt=retxTail; elt!=NULL; elt=elt->next) ne++;
      if (ne>domStatsPkt.maxRetxEntries) domStatsPkt.maxRetxEntries = ne;
   }
   
   if (retxFree==NULL) return NULL;

   /* unlink from the free list... */
   {  RetxElt *ret = retxFree;
      retxFree = retxFree->next;
      /* ret->next = NULL; */
      memset(ret, 0, sizeof(RetxElt));
      return ret;
   }
}

/* move allocated pointer back to free list... */
static void retxDispose(RetxElt *elt) {
   elt->next = retxFree;
   retxFree = elt;
   elt->pkt = NULL;
   elt->ticks = 0;
}

/* inserts are ordered by seqn, make sure to keep the order...
 * head points to the place to put it...
 */
static void retxInsert(RetxElt *elt) {
   if (retxHead!=NULL) retxHead->next = elt;
   else {
      /* retxHead==NULL -> retxTail==NULL */
      retxTail = elt;
   }

   retxHead = elt;
   elt->next = NULL;
}

/* search from tail for seqn...
 *
 * most likely the delete will come off the tail,
 * but it may not, so we have to relink it if it
 * came from the middle...
 *
 * return: 1 deleted, 0 not found
 */
static int retxDelete(unsigned short seqn) {
   /* delete is trickier... */
   RetxElt *prev = NULL, *t;
   for (t=retxTail; t!=NULL; prev=t, t=t->next) {
      PktHdr *hdr = (PktHdr *) t->pkt;
      if (hdr->seqn==seqn) {
         /* found it! */
         if (t==retxTail) {
            /* normal case... */
            if (retxTail==retxHead) retxTail = retxHead = NULL;
            else retxTail = retxTail->next;
         }
         else {
            /* must be more than one elt -- prev is set correctly... */
            prev->next = t->next;
            if (t==retxHead) retxHead = prev;
         }

         /* free the packet... */
         retxDispose(t);
         return 1;
      }
   }
   return 0;
}

/* the read queue, all read packets pass through here...
 *
 * the read queue data length _must_ be at least big enough
 * to hold a swpacket plus header...
 */
static unsigned rdQData[RDQUEUEDATALEN];
static unsigned rdQHead, rdQTail;

static inline unsigned rdQHeadIndex(void) { return rdQHead%RDQUEUEDATALEN; }
static inline unsigned rdQTailIndex(void) { return rdQTail%RDQUEUEDATALEN; }
static int rdQBytesFree(void) {
   int ret = (sizeof(unsigned)*(RDQUEUEDATALEN - (rdQHead - rdQTail))); 
   static int isInit;
   
   if (!isInit || ret<domStatsPkt.minRxQueueSize) {
      domStatsPkt.minRxQueueSize = ret;
      isInit = 1;
   }
   return ret;
}

static inline int rdQFull(void) { return rdQBytesFree() < HWRXMAXPACKETSIZE; }
static inline int rdQEmpty(void) { return rdQHead==rdQTail; }

/* get a packet from the read queue... */
static PktHdr *rdQGet(void) {
   const unsigned tailIdx = rdQTailIndex();
   unsigned *pkt = rdQData + tailIdx;
   PktHdr *hdr = (PktHdr *) pkt;
   PktHdr *ret = hdr;
   const unsigned pktIdxLen = hwPktLen(hdr->len)/sizeof(unsigned);
   const unsigned maxPktIdxLen = 
      (HWRXMAXPACKETSIZE+(sizeof(unsigned)-1))/sizeof(unsigned);

   rdQTail += pktIdxLen;
   if (rdQTailIndex() + maxPktIdxLen >= RDQUEUEDATALEN) {
      /* increment to boundry... */
      rdQTail += (RDQUEUEDATALEN - rdQTailIndex());
   }

   return ret;
}

/* put a packet into the read queue... */
static void rdQPut(unsigned *pkt) {
   const unsigned headIdx = rdQHeadIndex();
   unsigned *hpkt = rdQData + headIdx;
   PktHdr *hdr = (PktHdr *) pkt;
   const unsigned pktIdxLen = hwPktLen(hdr->len)/sizeof(unsigned);
   const unsigned maxPktIdxLen = 
      (HWRXMAXPACKETSIZE+(sizeof(unsigned)-1))/sizeof(unsigned);

   memcpy(hpkt, pkt, pktIdxLen*sizeof(unsigned));

   rdQHead += pktIdxLen;
   if (rdQHeadIndex() + maxPktIdxLen >= RDQUEUEDATALEN) {
      /* increment to boundry... */
      rdQHead += (RDQUEUEDATALEN - rdQHeadIndex());
   }
}

/* small copying collector for tx packets... */
static PktHdr *txAlloc(int payloadlen) {
   static unsigned txPktData[2][TXPKTDATALEN];
   static int from, idx;
   const int hwlen = hwPktLen(payloadlen)/sizeof(unsigned);

   if (idx+hwlen>TXPKTDATALEN) {
      int tidx = 0;
      int to = (from + 1)&1;
      RetxElt *elt = retxTail;
      
      /* try a gc cycle... */
      for (elt=retxTail; elt!=NULL; elt=elt->next) {
         if (elt->pkt!=NULL) {
            /* copy the packet... */
            PktHdr *hdr = (PktHdr *) elt->pkt;
            const int len = hwPktLen(hdr->len);
            memcpy(txPktData[to] + tidx, hdr, len);
            elt->pkt = txPktData[to] + tidx;
            tidx += len/sizeof(unsigned);
         }
      }
      from = to;
      idx = tidx;
   }

   if (idx+hwlen<=TXPKTDATALEN) {
      /* there's room... */
      PktHdr *ret = (PktHdr *) (txPktData[from] + idx);
      idx+=hwlen;
      return ret;
  }
   
   return NULL;
}


/* Table of CRCs of all 8-bit messages. */
static unsigned crc_table[256];

/* Flag: has the table been computed? Initially false. */
static int crc_table_computed = 0;

/* Make the table for a fast CRC. */
static void make_crc_table(void) {
   unsigned c;
   int n, k;
   for (n = 0; n < 256; n++) {
      c = (unsigned) n;
      for (k = 0; k < 8; k++) {
         if (c & 1) {
            c = 0xedb88320L ^ (c >> 1);
         }
         else {
            c = c >> 1;
         }
      }
      crc_table[n] = c;
   }
   crc_table_computed = 1;
}

/*
  Update a running crc with the bytes buf[0..len-1] and
  the updated crc. The crc should be initialized to zero. Pre-
  post-conditioning (one's complement) is performed within
  function so it shouldn't be done by the caller. Usage example
  
  unsigned crc = 0L
  
  while (read_buffer(buffer, length) != EOF) {
     crc = update_crc(crc, buffer, length);
  }
  if (crc != original_crc) error();
*/
static unsigned update_crc(unsigned crc,
                           unsigned char *buf, int len) {
   unsigned c = crc ^ 0xffffffffL;
   int n;
   
   if (!crc_table_computed) make_crc_table();
   
   for (n = 0; n < len; n++) {
      c = crc_table[(c ^ buf[n]) & 0xff] ^ (c >> 8);
   }

   return c ^ 0xffffffffL;
}
      
/* Return the CRC of the bytes buf[0..len-1]. */
static unsigned crc32(unsigned char *buf, int len) {
   return update_crc(0L, buf, len);
}

static int crcok(unsigned *pkt, int len) {
   const unsigned v = crc32((unsigned char *)pkt, len-4);
   return (v == pkt[(len-1)/4]);
}

/* create an acq packet... */
static unsigned  *mkAcqPkt(unsigned short seqn) {
   /* hdr + crc32 */
   static unsigned ar[sizeof(PktHdr)/4 + 1];
   unsigned *sdata = ar + sizeof(PktHdr)/4;
   PktHdr *hdr = (PktHdr *) ar;
      
   hdr->flags = 1;
   hdr->len = 0;
   hdr->type = 0;
   hdr->seqn = seqn;
   sdata[0] = crc32((unsigned char *) ar, sizeof(PktHdr));
   return ar;
}

/* wait for a packet from hardware...
 */
static unsigned *rcvHWPkt(int *len, int *type) {
   static unsigned pkt[HWRXMAXPACKETSIZE/sizeof(unsigned)];
   memset(pkt, 0, sizeof(pkt));
   hal_FPGA_TEST_hwreceive(type, len, (unsigned char *)pkt);
   domStatsPkt.nRxPkts++;
   return pkt;
}

#define NACKQBITS    8
#define NACKQENTRIES (1<<NACKQBITS)
#define ACKQMASK    (NACKQENTRIES-1)
static unsigned short aq[NACKQENTRIES];
static unsigned aqHead, aqTail;
static inline int getAckQHead(void) { return aqHead&ACKQMASK; }
static inline int getAckQTail(void) { return aqTail&ACKQMASK; }
static inline void putAckQ(unsigned short v) { aq[getAckQHead()]=v; aqHead++; }
static inline unsigned short getAckQ(void) {
   const unsigned short ret = aq[getAckQTail()];
   aqTail++;
   return ret;
}
static inline int nAckQEntries(void) { return aqHead - aqTail; }
static inline int ackQisFull(void) { return NACKQENTRIES == nAckQEntries(); }
static inline int ackQisEmpty(void) { return aqHead == aqTail; }

static void sendCI(void) {
   static struct CIStruct {
      PktHdr hdr;
      unsigned crc;
   } pkt;
   
   if (pkt.hdr.flags!=0x10) {
      /* initialize... */
      pkt.hdr.flags = 0x10;
      pkt.crc = crc32((unsigned char *)&pkt, sizeof(pkt)-4);
   }

   if (isHWPktSpace(hwPktLen(0))) {
      writeDebug("snd CI");
      hal_FPGA_TEST_hwsend(0, hwPktLen(0), (char *)&pkt);
   }
}

static void sendIC(void) {
   static struct ICStruct {
      PktHdr hdr;
      unsigned crc;
   } pkt;

   if (pkt.hdr.flags!=0x08) {
      /* initialize... */
      pkt.hdr.flags = 0x08;
      pkt.crc = crc32((unsigned char *)&pkt, sizeof(pkt)-4);
   }
   
   if (isHWPktSpace(hwPktLen(0))) {
      writeDebug("snd IC");
      hal_FPGA_TEST_hwsend(0, hwPktLen(0), (char *)&pkt);
   }
}

static void flushAckQueue(void) {
   const int len = hwPktLen(0);
   
   while (!ackQisEmpty() && isHWPktSpace(len)) {
      /* ack one packet -- since there's space... */
      unsigned *pkt = mkAcqPkt(getAckQ());
      PktHdr *hdr = (PktHdr *) pkt;

      {  char msg[80];
         snprintf(msg, sizeof(msg), "snd ack: %hu [%u]", 
                  hdr->seqn, getTicks());
         writeDebug(msg);
      }
      
      hal_FPGA_TEST_hwsend(0, len, (char *) pkt);
      domStatsPkt.nTxPkts++;
      domStatsPkt.nTxAckPkts++;
   }
}

static unsigned short rxSeqn; /* expected Rx sequence number */
static unsigned short txSeqn; /* next Tx sequence number to use */

/* initialization when we've become connected...
 */
static void connectInit(void) {
   /* reset sequence numbers... */
   rxSeqn = 0;
   txSeqn = 0;

   /* reset read queue: FIXME: do we really want this!?!? */
   rdQHead = rdQTail = 0;

   /* reset retx buffer */
   retxInit(1);

   /* signal that we're connected... */
   state=STATE_CONNECTED;
}

/* move from unconnected to connected, or die trying...
 */
static void connectUp(void) {
   unsigned ticks = getTicks();

   /* hmmm, this shouldn't be possible... */
   if (state==STATE_CONNECTED) return;

   writeDebugIndent("connect");

   sendIC(); /* signal i'm unconnected */

   while (state!=STATE_CONNECTED) {
      unsigned age = getTicks() - ticks;
      if (age>100) {
         /* every 200ms retry... */
         writeDebugIndent("retry");
         sendIC();
         debugUnindent();
         ticks = getTicks();
      }

      /* we need a response to go on... */
      if (!hal_FPGA_TEST_hwmsg_ready()) continue;

      {  int hwlen, hwtype;
         unsigned *pkt = (unsigned *) rcvHWPkt(&hwlen, &hwtype);
         PktHdr *hdr = (PktHdr *) pkt;
      
         writeDebugIndent("unconnected");
         if (hdr->flags&0x10) {
            writeDebug("got CI");
            connectInit();
            sendCI();
            ticks = getTicks();
         }
         else if (hdr->flags&0x08) {
            writeDebug("got IC");
            
            while (hal_FPGA_TEST_hwmsg_ready()) {
               /* better be an ic! */
               unsigned *pkt = (unsigned *) rcvHWPkt(&hwlen, &hwtype);
               PktHdr *hdr = (PktHdr *) pkt;
               if (hdr->flags&0x10) {
                  /* ci? */
                  writeDebug("loop: got CI");
                  connectInit();
                  break;
               }
               else if ((hdr->flags&0x08)==0) {
                  /* invalid packet... */
                  writeDebug("loop: invalid postIC");
                  
                  domStatsPkt.nInvalidPostIC++;
                  break;
               }
               else {
                  writeDebug("loop: dup IC");
               }
               
               /* give enough time for another one to come
                * across the wire (16 * 10 * 1us)...
                */
               if (!hal_FPGA_TEST_hwmsg_ready()) halUSleep(200);
            }
            
            sendCI();
            ticks = getTicks();
         }
         debugUnindent();
      }
   }
   debugUnindent();
}

/* update read queue, all data from card funnels through this routine
 * it fills the rdQ with hw packets and clears the wq of acked tx packets...
 *
 * also we manage connection state in here...
 */
static int scanPkts(int aggressive) {
   int npackets = 0;

   if (state!=STATE_CONNECTED) connectUp();
   
   while (hal_FPGA_TEST_hwmsg_ready() &&
          (aggressive || (!rdQFull() && !ackQisFull())) ) {
      int hwlen, hwtype; 
      /* get the packet... */
      unsigned *pkt = (unsigned *) rcvHWPkt(&hwlen, &hwtype);

      /* update statistics... */
      domStatsPkt.nRxPkts++;

      /* valid packet? */
      if ((hwlen&0x3)==0 && hwlen>=4+sizeof(PktHdr) && 
          hwlen<=HWRXMAXPACKETSIZE && crcok((unsigned *) pkt, hwlen)) {
         PktHdr *hdr = (PktHdr *) pkt;
         
         /* first, check for acq packet... */
         if (hdr->flags&1) {
            /* update statistics... */
            domStatsPkt.nRxAckPkts++;
            
            /* ack packet... */
            const int idx = retxDelete(hdr->seqn);
            if (idx==0) {
               /* whoops -- already acked! */
               domStatsPkt.nRxDupAcks++;
               {  char msg[80];
                  snprintf(msg, sizeof(msg), 
                           "rcv dup ack: seqn=%u", hdr->seqn);
                  writeDebug(msg);
               }
            }
            else {
               /* transmitted packet is ok -- 
                * drop it from retransmit queue... 
                */
               {  char msg[80];
                  snprintf(msg, sizeof(msg), 
                           "rcv good ack: %u [%u]", 
                           hdr->seqn, getTicks());
                  writeDebug(msg);
               }
               domStatsPkt.nRxGoodAcks++;
            }
         }
         else if (hdr->flags&4) {
            domStatsPkt.nRxControlPkts++;
            
            {
               char msg[80];
               snprintf(msg, sizeof(msg),
                        "control packet: len=%d, data=%02x", hdr->len,
                        *(unsigned char *) (pkt+1));
               writeDebug(msg);
            }
            
            /* control packet... */
            if (hdr->len==1 && *(unsigned char *) (hdr + 1) == 0 ) {
               /* request for domstats... */
               domStatsPkt.hdr.flags = 4;
               domStatsPkt.hdr.seqn = 0;
               domStatsPkt.hdr.type = 0;
               domStatsPkt.hdr.len = 
                  sizeof(domStatsPkt) - sizeof(unsigned) - sizeof(PktHdr);
               domStatsPkt.controlType = 0;
               domStatsPkt.crc = crc32((unsigned char *)&domStatsPkt, 
                                       sizeof(domStatsPkt)-4);

               if (isHWPktSpace(sizeof(domStatsPkt))) {
                  hal_FPGA_TEST_hwsend(0, sizeof(domStatsPkt), 
                                       (char *) &domStatsPkt);
                  domStatsPkt.nTxControlPkts++;
                  domStatsPkt.nTxPkts++;
               }
            }
         }
         else if (hdr->flags&8) {
            /* IC */
            writeDebug("connected: got IC");
            state = STATE_UNCONNECTED;
            rxSeqn = 0;
            txSeqn = 0;
            /* FIXME: do i really want to kill these?!?!? */
            rdQHead = rdQTail = 0;
            retxInit(1);
            connectUp();
         }
         else if (hdr->flags&0x10) {
            /* ignore CI when connected... */
         }
         else {
            /* delSeqn is the difference between the packet seqn
             * and our next expected sequence number...
             *
             * careful with the types here (so the wraparound comes
             * out right)...
             */
            const signed short delSeqn = hdr->seqn - rxSeqn;

            /* update statistics... */
            domStatsPkt.nRxDataPkts++;

            {  char msg[80];
               snprintf(msg, sizeof(msg),
                        "rcv data: %hu [%u]", hdr->seqn, getTicks());
               writeDebugIndent(msg);
            }
            
            /* data packet... */
            if (delSeqn<0 && !ackQisFull()) {
               /* we have to re-ack packets for which the ack didn't make
                * it down...
                */
               domStatsPkt.nRxDupDataPkts++;
               putAckQ(hdr->seqn);
               writeDebug("re-ack");
            }
            else if (hdr->len>0 && hdr->seqn==rxSeqn && 
                     !rdQFull() && !ackQisFull()) {
               /* valid data -- correct seqn and we have space for it... */
               domStatsPkt.nRxGoodDataPkts++;

               /* add to ack packet queue... */
               putAckQ(hdr->seqn);
               
               /* we have a new expected rx seqn */
               rxSeqn = hdr->seqn+1;

               /* put it in the queue */
               rdQPut(pkt);

               writeDebug("ok");
            }
            else { 
               /* invalid seqn or read queue full, drop it on the floor */ 
               domStatsPkt.nRxDroppedPkts++;
               
               {
                  char msg[80];
                  snprintf(msg, sizeof(msg),
                           "dropped: delSeqn=%hd, rdQFull()=%d, "
                           "ackQisFull()=%d",
                           delSeqn, rdQFull(), ackQisFull());
                  writeDebug(msg);
               }
            }
            debugUnindent();
         }
      }
      else { domStatsPkt.nRxBadPkts++; writeDebug("bad packet"); }

      /* flush pending acks, if we can... */
      flushAckQueue();

      /* signal that we did some work... */
      npackets++;
   }

   /* flush pending acks, if we can... */
   flushAckQueue();

#if 0
   writeDebug("done");
   debugUnindent();
#endif
   return npackets;
}

/* look through the retx list for packets that need to
 * be retransmitted...
 */
static int timeoutRetransmit(unsigned ticks) {
   /* retransmit all expired packets... */
   RetxElt *elt;
   const unsigned tooOld = 50; /* 100ms */
   int found = 0;
   
   for (elt=retxTail; elt!=NULL; elt=elt->next) {
      if (ticks - elt->ticks > tooOld) {
         PktHdr *h = (PktHdr *) elt->pkt;
         const int len = hwPktLen(h->len);
   
         found = 1;
         {  char msg[80];
            snprintf(msg, sizeof(msg),
                     "retx: seqn: %hu", h->seqn);
            writeDebug(msg);
         }
         
         /* don't go on if there is no space in tx... */
         if (!isHWPktSpace(len)) break;

         /* mark the time again and resend... */
         elt->ticks = getTicks();
         hal_FPGA_TEST_hwsend(h->type, len, (const char *) h);
         domStatsPkt.nTxPkts++;
         domStatsPkt.nTxResentPkts++;
      }
   }

   return found;
}

/* if the Rx fifo has data and the Rx queue is full
 * and there is stale data in the retx buffer,
 * we need to risk dropping packets 
 * to get at acks that we are expecting...
 */
static void unstickRx(unsigned ticks) {
   /* look for data in rx that has not been read because the
    * read queue is full.  after too old we need to aggressively
    * scan the rx fifo, even if that means dropping packets...
    */
   if (rdQFull() && hal_FPGA_TEST_hwmsg_ready()) {
      /* check for stale retx data */
      RetxElt *elt;
      int found = 0;

      for (elt=retxTail; !found && elt!=NULL; elt=elt->next) {
         const unsigned short age = ticks - elt->ticks;
         const unsigned short tooOld = 40; /* 80ms */
         found = age>tooOld;
      }
      
      if (found) {
         /* ok, clear them out... */
         scanPkts(1);
         flushAckQueue();
         return;
      }
   }
}

/* run this routine approx every tick (where a tick is approx
 * equal to the expected worst case packet round trip time)...
 *
 * this is where all the yucky stuff happens, dropped and
 * retransmitted packets, etc...
 */
static void runPeriodic(void) {
   {  static unsigned lastTicks;
      const unsigned ticks = getTicks();
      if (ticks!=lastTicks) {
         timeoutRetransmit(ticks);
         unstickRx(ticks);
         lastTicks = ticks;
      }
   }
}

/* try to fill packet from read queue...
 *
 * *idx = new index into data
 *
 * returns: 0 packet not filled
 *          1 packet filled
 */
static unsigned char *fillPkt(int *len, int *type) {
   /* as we read packet fragments, we assemble them into this buffer */
   static unsigned char data[MAXSWPKTSIZE];
   static unsigned short idx; /* index into the rdPkt buffer */
   
   while (!rdQEmpty()) {
      PktHdr *hdr = rdQGet();

      {  char msg[80];
         snprintf(msg, sizeof(msg),
                  "fill: len=%hu, seqn=%hu", hdr->len, hdr->seqn);
         writeDebug(msg);
      }

#if defined(SWPACKETASSEMBLY)
      /* make sure the data fits... */
      if (hdr->len + idx > MAXSWPKTSIZE) {
         /* clear the read idx */
         domStatsPkt.nBadFins++;
         idx = 0;
         
         {
            char msg[80];
            snprintf(msg, sizeof(msg), "packet too big: %d", hdr->len);
            writeDebug(msg);
         }
         return NULL;
      }
#endif

      /* copy the data... */
      memcpy(data + idx, hdr + 1, hdr->len);
      idx += hdr->len;
      
#if defined(SWPACKETASSEMBLY)
      /* no syn_fin, keep working on it... */
      if ( (hdr->flags & 2) == 0) {
         writeDebug("no syn fin");
         continue;
      }
#endif
      
      /* packet is complete... */
      *len = idx;
      *type = hdr->type;
      idx = 0;

      return data;
   }
         
   return NULL;
}

/* wait here until we're connected... */
static void waitConnected(void) {
   while ((volatile int) state != STATE_CONNECTED) {
      /* aggressively scan packets... */
      if (!scanPkts(1)) runPeriodic();
      /* wait */
   }
}

/* receive a cooked packet... */
int hal_FPGA_TEST_receive(int *type, int *len, char *msg) {
   writeDebugIndent("rcv");

   /* don't do anything until we're connected... */
   waitConnected();
   
   while (1) {
      unsigned char *pkt;
      
      /* do we have a filled packet in the rdQ? */
      if ((pkt=fillPkt(len, type))!=NULL) {
         /* fill the packet... */
         if (*len>0) memcpy(msg, pkt, *len);
         debugUnindent();
         return 0;
      }

      /* scan for packets... */
      if (!scanPkts(0)) {
         /* no work done last scan packet, wait for something to come in...
          */
         writeDebugIndent("wait hwmsg ready");
         while (!hal_FPGA_TEST_hwmsg_ready() && rdQEmpty()) {
            scanPkts(0);
            runPeriodic();
            flushAckQueue();
         }
         writeDebug("done");
         debugUnindent();
      }
   }

   debugUnindent();
   return 1;
}

/* send a cooked packet... */
int hal_FPGA_TEST_send(int type, int len, const char *msg) {
   const unsigned char *data = (const unsigned char *) msg;
   int idx = 0;

   writeDebugIndent("snd");

   waitConnected();

#if !defined(SWPACKETASSEMBLY)
   if (len > MAXSWPKTSIZE) {
      badPacketSize++;
      debugUnindent();
      return 1;
   }
#endif

   while (idx<len) {
      const int nleft = len-idx;
#if defined(SWPACKETASSEMBLY)
      const int nw =
         (nleft + sizeof(PktHdr) + 4 > HWTXMAXPACKETSIZE) ? 
         HWTXMAXPACKETSIZE - (sizeof(PktHdr) + 4) : nleft;
#else
      const int nw = nleft;
#endif
      RetxElt *elt = NULL;
      PktHdr *hdr = NULL;
      const int sz = hwPktLen(nw);

      /* make sure there is space to send packet... */
      while (1) {
         /* speculatively clear out retx packets... */
         scanPkts(0);

         /* resend any old retx packets first... */
         if (timeoutRetransmit(getTicks())) {
            /* may need to be more aggressive... */
            runPeriodic();
            continue;
         }

         /* try retx allocation... */
         elt=retxAlloc();
         if (elt==NULL) writeDebug("failed retxAlloc!");
         else {
            /* try tx allocation... */
            hdr = txAlloc(nw);
            elt->pkt = (unsigned *) hdr;
            if (hdr==NULL) {
               writeDebug("failed txAlloc!");
               retxDispose(elt);
               elt = NULL;
            }
         }

         /* is there space for the packet? */
         if (hdr!=NULL && elt!=NULL && isHWPktSpace(sz)) break;

         writeDebug("no pkt space");

         /* we need to reallocate our elt... */
         retxDispose(elt);
         elt = NULL;

         /* no space for tx/retx, process packets until we're free... */
         if (!scanPkts(0)) {
            flushAckQueue();
            runPeriodic();
         }
      }

      /* wrap up packet */
      memset(hdr, 0, sz);
      hdr->len = nw;
      hdr->type = type;
      hdr->flags = (nleft == nw) ? 2 : 0; /* syn_fin? */
      hdr->seqn = txSeqn;
      txSeqn++;
      
      memcpy(elt->pkt + sizeof(PktHdr)/4, data + idx, nw);
      idx += nw;
      elt->pkt[sz/4 - 1] = crc32((unsigned char *) elt->pkt, sz - 4);
      
      /* send it off on it's merry way... */
      {
         char msg[80];
         snprintf(msg, sizeof(msg),
                  "tx data: %hu [%u]", hdr->seqn, getTicks());
         writeDebug(msg);
      }

      /* mark the time it was sent... */
      elt->ticks = getTicks();
      
      /* add it to the retx active list... */
      retxInsert(elt);

      /* it's been added to retransmit queue, we can go ahead and send it,
       * the mem will be freed when (if) it is acked...
       */
      hal_FPGA_TEST_hwsend(type, sz, (char *)elt->pkt);
      domStatsPkt.nTxDataPkts++;
      domStatsPkt.nTxPkts++;
   }

   debugUnindent();
   return 0;
}

int hal_FPGA_TEST_msg_ready(void) {
   writeDebugIndent("rdy");
   
   /* wait until we have a connection... */
   waitConnected();
   
   /* first, have we timed out data on the retransmit queue? */
   runPeriodic();

   /* update then check the read queue... */
   scanPkts(0);

   /* flush ack queue... */
   flushAckQueue();

   debugUnindent();
   
   return !rdQEmpty();
}

#else

int hal_FPGA_TEST_msg_ready(void) { return hal_FPGA_TEST_hwmsg_ready(); }


int hal_FPGA_TEST_send(int type, int len, const char *msg) {
   return hal_FPGA_TEST_hwsend(type, len, msg);
}

int hal_FPGA_TEST_receive(int *type, int *len, char *msg) {
   return hal_FPGA_TEST_hwreceive(type, len, msg);
}

#endif
