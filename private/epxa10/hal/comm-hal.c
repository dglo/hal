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

/* prepended to all packets... */
static inline int pktLen(unsigned h)     { return h&0xfff; }
static inline int pktType(unsigned h)    { return (h&0x7000)>>12; }
static inline int pktDomType(unsigned h) { return (h&0x8000)>>15; }
static inline int pktSeqn(unsigned h)    { return h>>16; }
static inline int pktWords(unsigned h)   { return 1 + (pktLen(h)+3)/4; }

static inline unsigned pktMkHdr(unsigned len, unsigned type, unsigned seqn) {
   return (seqn<<16)|(type<<12)|len;
}

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

/* rx/tx max hardware packet size 
 */
#define HWMAXPACKETLEN    150
#define HWMAXPACKETSIZE   (HWMAXPACKETLEN*sizeof(unsigned))
#define HWMAXPAYLOADLEN   (HWMAXPACKETLEN-1)
#define HWMAXPAYLOADSIZE  (HWMAXPAYLOADLEN*sizeof(unsigned))

/* max packet size from sw */
#if defined(SWPACKETASSEMBLY)
 /* make sure packet + hdr fits in a page... */
# define MAXSWPKTSIZE (4096 - sizeof(unsigned))
#else
# define MAXSWPKTSIZE (HWMAXPACKETSIZE - sizeof(unsigned))
#endif

/* size of rx/tx dual ported memory buffer in 32 bit words */
#define HWRXDPBUFFERLEN (8*1024)
#define HWTXDPBUFFERLEN (8*1024)
#define HWTXDPBUFFERPTR ( (unsigned *) 0x80000000 )
#define HWRXDPBUFFERPTR ( (unsigned *) 0x80000000 + 8*1024)

static inline unsigned short tx_dpr_radr(void) {
   return (unsigned short) (*(unsigned volatile *) DOM_FPGA_COMM_TX_DPR_RADR);
}

static inline unsigned short tx_dpr_wadr(void) {
   return (unsigned short) (*(unsigned volatile *) DOM_FPGA_COMM_TX_DPR_WADR);
}

static inline void set_tx_dpr_wadr(unsigned short waddr) {
   *(unsigned volatile *) DOM_FPGA_COMM_TX_DPR_WADR = waddr;
}

static inline unsigned short rx_dpr_radr(void) {
   return (unsigned short) (*(unsigned volatile *) DOM_FPGA_COMM_RX_DPR_RADR);
}

static inline void set_rx_dpr_radr(unsigned short raddr) {
   *(unsigned volatile *) DOM_FPGA_COMM_RX_DPR_RADR = raddr;
}

/* how much space are we using in the tx buf?
 *
 * the value returned is in 32 bit word units...
 */
static inline unsigned short txSpaceUsed(void) {
   return tx_dpr_wadr() - tx_dpr_radr();
}

/* how much space is left in tx? 
 */
static inline unsigned short txSpaceRemaining(void) {
   const unsigned short size = HWTXDPBUFFERLEN;
   return size - txSpaceUsed();
}

/* is there space for a hw tx packet? */
static inline int isHWPktSpace(unsigned hdr) {
   return txSpaceRemaining() >= pktWords(hdr);
}

/* very low level -- put a hw packet on the wire,
 * block until space is avail...
 */
static int hal_FPGA_TEST_hwsend(const unsigned *msg) {
   int i;
   const int nw = pktWords(*msg);
   const int hwlen = nw*4;
   
   if (hwlen>HWMAXPACKETSIZE) return 1;
            
   {  unsigned *tx_buf = HWTXDPBUFFERPTR;
      unsigned short waddr = tx_dpr_wadr();

      /* wait for comm avail... */
      while ( RFPGABIT(COMM_STATUS, AVAIL) == 0 ) ; 

      /* wait for the proper amount of space left... */
      while (!isHWPktSpace(*msg)) ;

#if defined(DEBUGSERIAL)
      {
         char pmsg[80];
         snprintf(pmsg, sizeof(pmsg), "send: 0x%08x", *msg);
         writeDebug(pmsg);
      }
#endif
      
      /* send the data... */
      for (i=0; i<nw; i++) {
	 tx_buf[(waddr%((32/4)*1024))] = msg[i];
	 waddr++;
      }
      
      /* write the pointer back (this will toggle pkt_ready)... */
      set_tx_dpr_wadr(waddr);
   }

   return 0;
}

int hal_FPGA_TEST_hwmsg_ready(void) {
   /* wait for comm avail... */
   while (!RFPGABIT(COMM_STATUS, AVAIL)) 
      writeDebug("msg_ready: comm not avail");

   /* return message ready... */
   return RFPGABIT(COMM_STATUS, RX_PKT_RCVD)!=0;
}

static int hal_FPGA_TEST_hwreceive(unsigned *msg) {
   unsigned *rx_buf = HWRXDPBUFFERPTR;
   unsigned short raddr = rx_dpr_radr();
   
   /* wait for msg */
   while (!hal_FPGA_TEST_hwmsg_ready()) ;
   
   /* get header */
   msg[0] = rx_buf[raddr%(1024*32/4)]; raddr++;
   
   /* get data */
   {  const int nw = pktWords(msg[0]);
      int i;

      for (i=1; i<nw; i++) {
         msg[i] = rx_buf[raddr%(1024*32/4)];
         raddr++;
      }
   }

   /* write back pointer -- rx_done toggled inside fpga */
   set_rx_dpr_radr(raddr);
   
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


/* convert payload length (unsigned->len) to hardware packet length... 
 *
 * sizeof(unsigned) + (payload length padded to 32 bits) + (32 bit crc)
 */
static inline int hwPktLen(int swlen) { 
   return sizeof(unsigned) + ((swlen+3)/4)*4 + 4;
}

/* read queue size 
 *
 * the read queue has to be able to hold at least an
 * entire software packet.  we give it space for two
 * just in case (too much?)...
 */
#define RDQUEUEDATASIZE (2*(MAXSWPKTSIZE + sizeof(unsigned)))
#define RDQUEUEDATALEN  ((RDQUEUEDATASIZE+3)/sizeof(unsigned))

/* tx data size.
 *
 * the tx data buffer must have enough room to hold data until
 * we expect an acq back...
 */
#define TXPKTDATALEN  (1024*2)
#define TXPKTDATASIZE (TXPKTDATALEN*sizeof(unsigned))

/* length of retransmit queue
 *
 * we want to make sure that we won't fail an allocation
 * unless there is no tx data too...
 */
#define WRPKTQUEUELEN (TXPKTDATASIZE/(sizeof(unsigned)))

/* we keep one copy of this guy around
 * to pass back to the dor for compiling
 * statistics...
 */
static struct DomStatsPkt {
   unsigned hdr;
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
      unsigned hdr = t->pkt[0];
      if (pktSeqn(hdr)==seqn) {
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

#if defined(DEBUGSERIAL) && 0
         {
            char msg[80];
            snprintf(msg, sizeof(msg), "del seq: %hu", pktSeqn(hdr));
            writeDebug(msg);
         }
#endif

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

static inline int rdQFull(void) { return rdQBytesFree() < HWMAXPACKETSIZE; }
static inline int rdQEmpty(void) { return rdQHead==rdQTail; }

/* get a packet from the read queue... */
static unsigned *rdQGet(void) {
   const unsigned tailIdx = rdQTailIndex();
   unsigned *pkt = rdQData + tailIdx;
   unsigned hdr = *(unsigned *) pkt;
   unsigned *ret = pkt;
   const unsigned pktIdxLen = pktWords(hdr);
   const unsigned maxPktIdxLen = HWMAXPACKETLEN;

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
   unsigned hdr = *(unsigned *) pkt;
   const unsigned pktIdxLen = pktWords(hdr);
   const unsigned maxPktIdxLen = HWMAXPACKETLEN;

   memcpy(hpkt, pkt, pktIdxLen*sizeof(unsigned));

   rdQHead += pktIdxLen;
   if (rdQHeadIndex() + maxPktIdxLen >= RDQUEUEDATALEN) {
      /* increment to boundry... */
      rdQHead += (RDQUEUEDATALEN - rdQHeadIndex());
   }
}

/* small copying collector for tx packets... */
static unsigned *txAlloc(int payloadlen) {
   static unsigned txPktData[2][TXPKTDATALEN];
   static int from, idx;
   const int hwlen = pktWords(payloadlen);

   if (idx+hwlen>TXPKTDATALEN) {
      int tidx = 0;
      int to = (from + 1)&1;
      RetxElt *elt = retxTail;
      
      /* try a gc cycle... */
      for (elt=retxTail; elt!=NULL; elt=elt->next) {
         if (elt->pkt!=NULL) {
            /* copy the packet... */
            unsigned *pkt = (unsigned *) elt->pkt;
            unsigned hdr = *(unsigned *) pkt;
            const int nw = pktWords(hdr);
            memcpy(txPktData[to] + tidx, pkt, nw*sizeof(unsigned));
            elt->pkt = txPktData[to] + tidx;
            tidx += nw;
         }
      }
      from = to;
      idx = tidx;
   }

   if (idx+hwlen<=TXPKTDATALEN) {
      /* there's room... */
      unsigned *ret = txPktData[from] + idx;
      idx+=hwlen;
      return ret;
   }
   
   return NULL;
}

/* create an acq packet... */
static unsigned inline mkAcqPkt(unsigned short seqn) {
   return pktMkHdr(0, 1, seqn);
}

/* wait for a packet from hardware...
 */
static unsigned *rcvHWPkt(void) {
   static unsigned pkt[HWMAXPACKETLEN];
   memset(pkt, 0, sizeof(pkt)); /* !!!! FIXME: help!!! */
   hal_FPGA_TEST_hwreceive(pkt);
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
   unsigned hdr = pktMkHdr(0, 5, 0);
   if (isHWPktSpace(hdr)) {
      writeDebug("snd CI");
      hal_FPGA_TEST_hwsend(&hdr);
   }
}

static void sendIC(void) {
   unsigned hdr = pktMkHdr(0, 4, 0);
   if (isHWPktSpace(hdr)) {
      writeDebug("snd IC");
      hal_FPGA_TEST_hwsend(&hdr);
   }
}

static void flushAckQueue(void) {
   while (!ackQisEmpty() && isHWPktSpace(0)) {
      /* ack one packet -- since there's space... */
      unsigned pkt = mkAcqPkt(getAckQ());

#if defined(DEBUGSERIAL) && 0
      {  char msg[80];
         snprintf(msg, sizeof(msg), "snd ack: %hu [%u]", 
                  pktSeqn(pkt), getTicks());
         writeDebug(msg);
      }
#endif

      hal_FPGA_TEST_hwsend(&pkt);
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

      {  unsigned *pkt = rcvHWPkt();
         const unsigned hdr = *pkt;
      
         writeDebugIndent("unconnected");
         if (pktType(hdr)==5) {
            writeDebug("got CI");
            connectInit();
            sendCI();
            ticks = getTicks();
         }
         else if (pktType(hdr)==4) {
            writeDebug("got IC");
            
            while (hal_FPGA_TEST_hwmsg_ready()) {
               /* better be an ic! */
               unsigned *pkt = rcvHWPkt();
               const unsigned hdr = * (unsigned *) pkt;
               if (pktType(hdr)==5) {
                  /* ci? */
                  writeDebug("loop: got CI");
                  connectInit();
                  break;
               }
               else if (pktType(hdr)!=4) {
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
      /* get the packet... */
      unsigned *pkt = rcvHWPkt();
      const unsigned hdr = *pkt;
      const int hwtype = pktType(hdr);
      const int hwlen = pktLen(hdr);

      /* update statistics... */
      domStatsPkt.nRxPkts++;

      /* first, check for acq packet... */
      if (hwtype==1 && hwlen==0) {
         /* update statistics... */
         domStatsPkt.nRxAckPkts++;
         
         /* ack packet... */
         const int idx = retxDelete(pktSeqn(hdr));
         if (idx==0) {
            /* whoops -- already acked! */
            domStatsPkt.nRxDupAcks++;
            
#if defined(DEBUGSERIAL) && 0
            {  char msg[80];
               snprintf(msg, sizeof(msg), 
                        "rcv dup ack: seqn=%u", pktSeqn(hdr));
               writeDebug(msg);
            }
#endif
         }
         else {
            /* transmitted packet is ok -- 
             * drop it from retransmit queue... 
             */
#if defined(DEBUGSERIAL) && 0
            {  char msg[80];
               snprintf(msg, sizeof(msg), 
                        "rcv good ack: %u [%u]", 
                        pktSeqn(hdr), getTicks());
               writeDebug(msg);
            }
#endif
            domStatsPkt.nRxGoodAcks++;
         }
      }
      else if (hwtype==3 && hwlen<=HWMAXPAYLOADSIZE) {
         domStatsPkt.nRxControlPkts++;
            
#if defined(DEBUGSERIAL) && 0
         {
            char msg[80];
            snprintf(msg, sizeof(msg),
                     "control packet: len=%d, data=%02x", pktLen(hdr),
                     *(unsigned char *) (pkt+1));
            writeDebug(msg);
         }
#endif
      
         /* control packet... */
         if (* (unsigned char *) (pkt + 1) == 0 ) {
            /* request for domstats... */
            domStatsPkt.hdr = 
               pktMkHdr(sizeof(domStatsPkt) - sizeof(unsigned), 3, 0);

            domStatsPkt.controlType = 0;

            if (isHWPktSpace(domStatsPkt.hdr)) {
               hal_FPGA_TEST_hwsend((unsigned *) &domStatsPkt);
               domStatsPkt.nTxControlPkts++;
               domStatsPkt.nTxPkts++;
            }
         }
      }
      else if (hwtype==4 && hwlen==0) {
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
      else if (hwtype==5 && hwlen==0) {
         /* ignore CI when connected... */
      }
      else if ( (hwtype==0 || hwtype==2) && 
                hwlen>0 && hwlen<=HWMAXPAYLOADSIZE) {
         /* delSeqn is the difference between the packet seqn
          * and our next expected sequence number...
          *
          * careful with the types here (so the wraparound comes
          * out right)...
          */
         const signed short delSeqn = pktSeqn(hdr) - rxSeqn;
         
         /* update statistics... */
         domStatsPkt.nRxDataPkts++;

#if defined(DEBUGSERIAL)
         {  char msg[80];
            snprintf(msg, sizeof(msg),
                     "rcv: 0x%08x", hdr);
            writeDebugIndent(msg);
         }
#endif

         /* data packet... */
         if (delSeqn<0 && !ackQisFull()) {
            /* we have to re-ack packets for which the ack didn't make
             * it down...
             */
            domStatsPkt.nRxDupDataPkts++;
            putAckQ(pktSeqn(hdr));
            writeDebug("re-ack");
         }
         else if (pktLen(hdr)>0 && pktSeqn(hdr)==rxSeqn && 
                  !rdQFull() && !ackQisFull()) {
            /* valid data -- correct seqn and we have space for it... */
            domStatsPkt.nRxGoodDataPkts++;
            
            /* add to ack packet queue... */
            putAckQ(pktSeqn(hdr));
            
            /* we have a new expected rx seqn */
            rxSeqn = pktSeqn(hdr)+1;
            
            /* put it in the queue */
            rdQPut(pkt);
            
            writeDebug("ok");
         }
         else { 
            /* invalid seqn or read queue full, drop it on the floor */ 
            domStatsPkt.nRxDroppedPkts++;
            
#if defined(DEBUGSERIAL)
            {
               char msg[80];
               snprintf(msg, sizeof(msg),
                        "dropped: delSeqn=%hd, rdQFull()=%d, "
                        "ackQisFull()=%d",
                        delSeqn, rdQFull(), ackQisFull());
               writeDebug(msg);
            }
#endif
         }
         debugUnindent();
      }
      else { 
         domStatsPkt.nRxBadPkts++; 

#if defined(DEBUGSERIAL)
         {
            char msg[80];
            snprintf(msg, sizeof(msg),
                     "!!!BAD PACKET: hdr=0x%08x", hdr);
            writeDebug(msg);
         }
#endif
} 

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
   const int tooOld = 100; /* 200ms */
   int found = 0;
   
   for (elt=retxTail; elt!=NULL; elt=elt->next) {
      unsigned *pkt = elt->pkt;
      const int dt = (int) ticks - (int) elt->ticks;

#if defined(DEBUGSERIAL) && 0
         {  char msg[80];
            snprintf(msg, sizeof(msg),
                     "retx: seqn=%hu dt=%d", pktSeqn(*pkt), dt);
            writeDebug(msg);
         }
#endif
      
      if (pkt!=NULL && dt > tooOld) {
         const unsigned h = *pkt;
   
         found = 1;

#if defined(DEBUGSERIAL)
         {  char msg[80];
            snprintf(msg, sizeof(msg),
                     "retx: seqn: %hu", pktSeqn(h));
            writeDebug(msg);
         }
#endif
         /* don't go on if there is no space in tx... */
         if (!isHWPktSpace(h)) break;

         /* mark the time again and resend... */
         elt->ticks = getTicks();
         hal_FPGA_TEST_hwsend(pkt);
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
      unsigned *pkt = rdQGet();
      const unsigned hdr = *pkt;

#if defined(DEBUGSERIAL) && 0
      {  char msg[80];
         snprintf(msg, sizeof(msg),
                  "fill: len=%hu, seqn=%hu", pktLen(hdr), pktSeqn(hdr));
         writeDebug(msg);
      }
#endif

#if defined(SWPACKETASSEMBLY)
      /* make sure the data fits... */
      if (pktLen(hdr) + idx > MAXSWPKTSIZE) {
         /* clear the read idx */
         domStatsPkt.nBadFins++;
         idx = 0;
         
#if defined(DEBUGSERIAL) && 0
         {
            char msg[80];
            snprintf(msg, sizeof(msg), "packet too big: %d", pktLen(hdr));
            writeDebug(msg);
         }
#endif

         return NULL;
      }
#endif

      /* copy the data... */
      memcpy(data + idx, pkt + 1, pktLen(hdr));
      idx += pktLen(hdr);
      
#if defined(SWPACKETASSEMBLY)
      /* no syn_fin, keep working on it... */
      if (pktType(hdr) == 0) {
         writeDebug("no syn fin");
         continue;
      }
#endif
      
      /* packet is complete... */
      *len = idx;
      *type = pktType(hdr);
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
         /* FIXME: not another copy!!! */
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
      const int wleft = pktWords(nleft);

#if defined(SWPACKETASSEMBLY)
      const int pktlen = 
         (wleft > HWMAXPACKETLEN) ? 4 * (HWMAXPACKETLEN - 1) : nleft;
#else
      const int pktlen = nleft;
#endif
      RetxElt *elt = NULL;

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
            if ((elt->pkt=txAlloc(pktlen))==NULL) {
               writeDebug("failed txAlloc!");
               retxDispose(elt);
               elt = NULL;
            }
         }

         /* is there space for the packet?
          */
         if (elt!=NULL && isHWPktSpace(pktlen)) break;

         writeDebug("no pkt space");

         /* no space for tx/retx, process packets until we're free... */
         if (!scanPkts(0)) {
            flushAckQueue();
            runPeriodic();
         }
      }

      /* wrap up packet */
      elt->pkt[0] = pktMkHdr(pktlen, (pktlen==nleft) ? 2 : 0, txSeqn);
      txSeqn++;
      memcpy(elt->pkt + 1, data + idx, pktlen);
      idx += pktlen;
      
      /* send it off on it's merry way... */
#if defined(DEBUGSERIAL) && 0
      {
         char msg[80];
         snprintf(msg, sizeof(msg),
                  "tx data: %hu [%u]", pktSeqn(elt->pkt[0]), getTicks());
         writeDebug(msg);
      }
#endif

      /* mark the time it was sent... */
      elt->ticks = getTicks();
      
      /* add it to the retx active list... */
      retxInsert(elt);

      /* it's been added to retransmit queue, we can go ahead and send it,
       * the mem will be freed when (if) it is acked...
       */
      hal_FPGA_TEST_hwsend(elt->pkt);
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
   /* package up the packet... */
   unsigned pkt[HWMAXPACKETLEN];
   pkt[0] = pktMkHdr(len, 0, 0);
   memcpy(pkt+1, msg, len);
   return hal_FPGA_TEST_hwsend(pkt);
}

int hal_FPGA_TEST_receive(int *type, int *len, char *msg) {
   /* unpackage the packet... */
   unsigned pkt[HWMAXPACKETLEN];
   if (hal_FPGA_TEST_hwreceive(pkt)) return 1;
   *type = pktType(*pkt);
   *len = pktLen(*pkt);
   memcpy(msg, pkt+1, *len);
   return 0;
}

#endif

void hal_FPGA_TEST_request_reboot(void) { 
   unsigned reg = FPGA(COMM_CTRL);

   /* wait for Tx to drain forever... */
   while (txSpaceUsed()!=0) ;
   
   FPGA(COMM_CTRL) = reg | FPGABIT(COMM_CTRL, REBOOT_REQUEST); 
}

int  hal_FPGA_TEST_is_reboot_granted(void) {
   return RFPGABIT(COMM_STATUS, REBOOT_GRANTED)!=0;
}

int hal_FPGA_TEST_is_comm_avail(void) {
   return RFPGABIT(COMM_STATUS, AVAIL)!=0;
}







