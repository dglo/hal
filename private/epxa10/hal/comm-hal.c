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

#include "booter/epxa.h"

#define lengthof(a) (sizeof(a)/sizeof(a[0]))

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
static int hal_FPGA_hwsend(const unsigned *msg) {
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

int hal_FPGA_hwmsg_ready(void) {
   /* wait for comm avail... */
   while (!RFPGABIT(COMM_STATUS, AVAIL)) 
      writeDebug("msg_ready: comm not avail");

   /* return message ready... */
   return RFPGABIT(COMM_STATUS, RX_PKT_RCVD)!=0;
}

static int hal_FPGA_hwreceive(unsigned *msg) {
   unsigned *rx_buf = HWRXDPBUFFERPTR;
   unsigned short raddr = rx_dpr_radr();
   
   /* wait for msg */
   while (!hal_FPGA_hwmsg_ready()) ;
   
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
 * just in case (too much?).  it also has to be bigger
 * than the retransmit queue on the other side -- this
 * ensures that we don't start retxing packets on
 * "normal" throughput paths...
 */
#define RDQUEUEDATASIZE (2*(MAXSWPKTSIZE + sizeof(unsigned)))
#define RDQUEUEDATALEN  ((RDQUEUEDATASIZE+3)/sizeof(unsigned))

/* tx data size.
 *
 * the tx data buffer must have enough room to hold data until
 * we expect an acq back...
 *
 * these are misnamed, they should be retx datalen and retx datasize...
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

struct {
   struct {
      unsigned *pkt;
      unsigned ticks;
   } q[128];
   
   unsigned short h, t;
   unsigned short tt;
   unsigned short i;
} retx;

static inline int retxIndex(unsigned short i) {
   return i % lengthof(retx.q);
}

static int retxDelete(unsigned short seqn) {
   unsigned short i;
   int found = 0;

   /* search (from the back) for the seqn...
    */
   for (i=retx.t; i!=retx.tt; i++) {
      const int idx = retxIndex(i);
      unsigned *pkt = retx.q[idx].pkt;
      
      if (pkt!=NULL && pktSeqn(*pkt)==seqn) {
         /* found it!! */
         found = 1;
         retx.q[idx].pkt = NULL;
      }
   }

   /* clear out the retx tail of old packets... 
    */
   while (retx.t!=retx.tt) {
      const int idx = retxIndex(retx.t);
      if (retx.q[idx].pkt!=NULL) break;
      retx.t++;
   }

   return found;
}

static void retxClear(void) {
   retx.h = retx.t = retx.i = retx.tt = 0;
}

/* take the contents of the retx buffer and put
 * them on the hw...
 */
static void txFlush(void) {
   while (retx.i!=retx.h) {
      const int idx = retxIndex(retx.i);
      unsigned *pkt = retx.q[idx].pkt;
      if (pkt!=NULL) {
#if defined(DEBUGSERIAL) && 0
         {
            char msg[80];
            snprintf(msg, sizeof(msg),
                     "tx data: %hu [%u]", pktSeqn(pkt[0]), getTicks());
            writeDebug(msg);
         }
#endif
         hal_FPGA_hwsend(pkt);
         retx.q[idx].ticks = getTicks();
         domStatsPkt.nTxDataPkts++;
         domStatsPkt.nTxPkts++;
      }
      
      if (retx.i==retx.tt) retx.tt++;
      retx.i++;
   }
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
      unsigned short i;
      
      /* try a gc cycle... */
      for (i=retx.t; i!=retx.h; i++) {
         const int idx = retxIndex(i);
         unsigned *pkt = retx.q[idx].pkt;
         if (pkt!=NULL) {
            /* copy the packet... */
            unsigned hdr = *(unsigned *) pkt;
            const int nw = pktWords(hdr);
            memcpy(txPktData[to] + tidx, pkt, nw*sizeof(unsigned));
            retx.q[idx].pkt = txPktData[to] + tidx;
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
   hal_FPGA_hwreceive(pkt);
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
      hal_FPGA_hwsend(&hdr);
   }
}

static void sendIC(void) {
   unsigned hdr = pktMkHdr(0, 4, 0);
   if (isHWPktSpace(hdr)) {
      writeDebug("snd IC");
      hal_FPGA_hwsend(&hdr);
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

      hal_FPGA_hwsend(&pkt);
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
   retxClear();

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
      if (!hal_FPGA_hwmsg_ready()) continue;

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
            
            while (hal_FPGA_hwmsg_ready()) {
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
               if (!hal_FPGA_hwmsg_ready()) halUSleep(200);
            }
            
            sendCI();
            ticks = getTicks();
         }
         debugUnindent();
      }
   }
   debugUnindent();
}

/* signal when we've reconnected so that we can
 * throw away stale data...
 */
static int connectFlag;

/* update read queue, all data from card funnels through this routine
 * it fills the rdQ with hw packets and clears the wq of acked tx packets...
 *
 * also we manage connection state in here...
 */
static int scanPkts(int aggressive) {
   int npackets = 0;

   if (state!=STATE_CONNECTED) connectUp();
   
   while (hal_FPGA_hwmsg_ready() &&
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
         if (retxDelete(pktSeqn(hdr))) {
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
               hal_FPGA_hwsend((unsigned *) &domStatsPkt);
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
         retxClear();
         connectUp();
         connectFlag=1;
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
 *
 * if we _ever_ start to retx, we must stop sending
 * packets until the _entire_ retx queue is drained...
 *
 * we just reset the tx pointer now and call txFlush...
 *
 * how do we arrive at 800ms?
 *
 *   outstanding data on dor side:  16Kb
 *   outstanding data on our side:   8kb
 *   outstanding data on other dom:  8kb
 *   -----------------------------------
 *   total:                         32kb
 *        we do 45kb/sec => 32/45=711ms
 */
static int timeoutRetransmit(unsigned ticks) {
   /* retransmit all expired packets... */
   const unsigned tooOld = 400; /* 800ms */

   if (retx.tt != retx.t &&   /* anything in retx queue? */
       ticks - retx.q[retxIndex(retx.t)].ticks > tooOld) { /* old packet? */
      /* resend... */
#if defined(DEBUGSERIAL) && 0
      {  char msg[80];
         snprintf(msg, sizeof(msg), "retx");
         writeDebug(msg);
      }
#endif
      retx.i = retx.t;
      txFlush();
      scanPkts(1);  /* drop rx pkts on the floor to get to the acks... */
      return 1;
   }

   return 0;
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
   if (rdQFull() && hal_FPGA_hwmsg_ready()) {
      /* check for stale retx data */
      unsigned short i;
      int found = 0;

      for (i=retx.t; !found && i!=retx.tt; i++) {
         const int idx = retxIndex(i);
         if (retx.q[idx].pkt!=NULL) {
            const unsigned short age = ticks - retx.q[idx].ticks;
            const unsigned short tooOld = 40; /* 80ms */
            found = age>tooOld;
         }
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
   static unsigned lastTicks;
   const unsigned ticks = getTicks();
   if (ticks!=lastTicks) {
      timeoutRetransmit(ticks);
      unstickRx(ticks);
      lastTicks = ticks;
   }
}

/* try to fill packet from read queue...
 *
 * FIXME: should be:
 *   static int fillPkt(unsigned char *data, int *idx, int *type)
 *
 * where:
 *   data is a pointer to the data to fill
 *   idx is a pointer to the index of the next byte in data to fill
 *   type is a pointer to the type of packet to fill
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
int hal_FPGA_receive(int *type, int *len, char *msg) {
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
         while (!hal_FPGA_hwmsg_ready() && rdQEmpty()) {
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

/* send a cooked packet... 
 *
 * there are two barriers to cross to get a packet on the wire.
 *
 * 1) get it in the retx buffer
 * 2) get it on the hw queue
 */
int hal_FPGA_send(int type, int len, const char *msg) {
   const unsigned char *data = (const unsigned char *) msg;
   int idx = 0;
   
   writeDebugIndent("snd");

   waitConnected();

   /* we need to know if we ever get disconnected
    * so that we can drop this packet...
    */
   connectFlag=0;

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

      /* make sure there is space to send packet... */
      while (1) {
         /* speculatively clear out retx packets... */
         scanPkts(0);
         if (connectFlag) {
            /* uh-oh, we had a open/close, drop this
             * packet -- it is stale...
             */
            return 0;
         }

         /* maybe packets need to be retx'd in order for them
          * to be acked (and hence removed from the list)
          */
         if (timeoutRetransmit(getTicks())) {
            /* may need to be more aggressive... */
            runPeriodic();
            continue;
         }

         /* try retx allocation... */
         if (retx.h - retx.t == (unsigned short) lengthof(retx.q)) {
            writeDebug("failed retxAlloc!");
         }
         else {  
            /* try tx allocation... */
            const unsigned short i = retxIndex(retx.h);
            unsigned *pkt;
            
            if ((pkt=retx.q[i].pkt=txAlloc(pktlen))==NULL) {
               writeDebug("failed txAlloc!");

               /* no space for tx/retx, process packets until we're free... */
               {  int ret = scanPkts(0);
                  if (connectFlag) return 0;
                  if (!ret) {
                     flushAckQueue();
                     runPeriodic();
                  }
               }
            }
            else {
               /* wrap up packet */
               pkt[0] = pktMkHdr(pktlen, (pktlen==nleft) ? 2 : 0, txSeqn);
               txSeqn++;
               retx.h++;
               memcpy(pkt + 1, data + idx, pktlen);
               idx += pktlen;

               break;
            }
         }
      }
   }
   
   /* send packets off... */
   txFlush();

   debugUnindent();

   return 0;
}

int hal_FPGA_msg_ready(void) {
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

int hal_FPGA_msg_ready(void) { return hal_FPGA_hwmsg_ready(); }

int hal_FPGA_send(int type, int len, const char *msg) {
   /* package up the packet... */
   unsigned pkt[HWMAXPACKETLEN];
   pkt[0] = pktMkHdr(len, 0, 0);
   memcpy(pkt+1, msg, len);
   return hal_FPGA_hwsend(pkt);
}

int hal_FPGA_receive(int *type, int *len, char *msg) {
   /* unpackage the packet... */
   unsigned pkt[HWMAXPACKETLEN];
   if (hal_FPGA_hwreceive(pkt)) return 1;
   *type = pktType(*pkt);
   *len = pktLen(*pkt);
   memcpy(msg, pkt+1, *len);
   return 0;
}

#endif

void hal_FPGA_request_reboot(void) { 
   unsigned reg = FPGA(COMM_CTRL);

   /* wait for Tx to drain forever... */
   while (txSpaceUsed()!=0) ;
   
   FPGA(COMM_CTRL) = reg | FPGABIT(COMM_CTRL, REBOOT_REQUEST); 
}

int  hal_FPGA_is_reboot_granted(void) {
   return RFPGABIT(COMM_STATUS, REBOOT_GRANTED)!=0;
}

int hal_FPGA_is_comm_avail(void) {
   return RFPGABIT(COMM_STATUS, AVAIL)!=0;
}

void hal_FPGA_set_comm_params(int thresh, int dacmax,
                              int rdelay, int sdelay,
                              int minclev, int maxclev) {
   FPGA(COMM_CLEV) = (minclev & 0x3ff)| ( (maxclev & 0x3ff) << 16 );
   FPGA(COMM_THR_DEL) = 
      (thresh & 0xff) |
      ((dacmax & 0x3)<<8) |
      ((rdelay & 0xff)<<16) |
      ((sdelay & 0xff)<<24);
}

