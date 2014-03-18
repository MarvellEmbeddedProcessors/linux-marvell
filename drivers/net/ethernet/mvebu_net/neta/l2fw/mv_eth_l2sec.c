/*******************************************************************************
Copyright (C) Marvell International Ltd. and its affiliates

This software file (the "File") is owned and distributed by Marvell
International Ltd. and/or its affiliates ("Marvell") under the following
alternative licensing terms.  Once you have made an election to distribute the
File under one of the following license alternatives, please (i) delete this
introductory statement regarding license alternatives, (ii) delete the two
license alternatives that you have not elected to use and (iii) preserve the
Marvell copyright notice above.


********************************************************************************
Marvell GPL License Option

If you received this File from Marvell, you may opt to use, redistribute and/or
modify this File in accordance with the terms and conditions of the General
Public License Version 2, June 1991 (the "GPL License"), a copy of which is
available along with the File in the license.txt file or by writing to the Free
Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 or
on the worldwide web at http://www.gnu.org/licenses/gpl.txt.

THE FILE IS DISTRIBUTED AS-IS, WITHOUT WARRANTY OF ANY KIND, AND THE IMPLIED
WARRANTIES OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE ARE EXPRESSLY
DISCLAIMED.  The GPL License provides additional details about this warranty
disclaimer.
*******************************************************************************/

#include "mvOs.h"
#include  <linux/interrupt.h>
#include "ctrlEnv/mvCtrlEnvLib.h"
#include "mv_neta/net_dev/mv_netdev.h"
#include "mv_eth_l2sec.h"
#include "mv_eth_l2fw.h"
#include "mvDebug.h"
#include "gbe/mvNetaRegs.h"
#include <linux/spinlock.h>

static struct tasklet_struct l2sec_tasklet;
static MV_L2FW_SEC_CESA_PRIV *req_array[MV_L2FW_SEC_REQ_Q_SIZE];
unsigned int req_empty;
unsigned int req_ready;
atomic_t req_count;
spinlock_t cesa_lock[CESA_CHAN];

static int cesaChanPort[CONFIG_MV_ETH_PORTS_NUM];

static MV_BUF_INFO *pBufInfoArray[CESA_CHAN];

static int cesaPrivIndx[CESA_CHAN];
static int cesaCmdIndx[CESA_CHAN];
int cesaFullResBuf[CESA_CHAN];

MV_L2FW_SEC_CESA_PRIV *cesaPrivArray[CESA_CHAN];
MV_CESA_COMMAND *cesaCmdArray[CESA_CHAN];
static MV_CESA_MBUF *cesaMbufArray[CESA_CHAN];
void *cesaOSHandle;


static MV_L2FW_SEC_SA_ENTRY sa;


#define MALLOC_AND_CLEAR(_ptr_, _size_) {\
	(_ptr_) = mvOsMalloc(_size_);\
	if ((_ptr_) == NULL) {\
		mvOsPrintf("Can't allocate %d bytes of memory\n", (_size_));\
		return;\
	 } \
	memset((_ptr_), 0, (_size_));\
}


void mv_l2sec_stats()
{
	int chan;
	for (chan = 0; chan < CESA_CHAN; chan++)
		printk(KERN_INFO "number of l2sec channel %d full result buffer events = %d\n", chan, cesaFullResBuf[chan]);
}

void printEspHdr(MV_ESP_HEADER *pEspHdr)
{
	printk(KERN_INFO "pEspHdr->spi=%d in %s\n"  , pEspHdr->spi, __func__);
	printk(KERN_INFO "pEspHdr->seqNum=%d in %s\n", pEspHdr->seqNum, __func__);
}

void printIpHdr(MV_IP_HEADER *pIpHdr)
{
	MV_U8	  *srcIP, *dstIP;

	srcIP = (MV_U8 *)&(pIpHdr->srcIP);
	dstIP = (MV_U8 *)&(pIpHdr->dstIP);

	pr_info("%u.%u.%u.%u->%u.%u.%u.%u in %s\n", MV_IPQUAD(srcIP), MV_IPQUAD(dstIP), __func__);
	pr_info("MV_16BIT_BE(pIpHdr->totalLength)=%d  in %s\n", MV_16BIT_BE(pIpHdr->totalLength), __func__);
	pr_info("pIpHdr->protocol=%d\n", pIpHdr->protocol);
}

static inline MV_STATUS mv_eth_l2sec_tx(struct eth_pbuf *pkt, struct eth_port *pp)
{
	struct neta_tx_desc *tx_desc;
	struct tx_queue *txq_ctrl;
	int l3_status;

	/* assigning different txq for each rx port , to avoid waiting on the
	same txq lock when traffic on several rx ports are destined to the same
	outgoing interface */
	int txq = 0;
	txq_ctrl = &pp->txq_ctrl[pp->txp * CONFIG_MV_ETH_TXQ + txq];

	if (txq_ctrl->txq_count >= mv_ctrl_txdone)
		mv_eth_txq_done(pp, txq_ctrl);

	/* Get next descriptor for tx, single buffer, so FIRST & LAST */
	tx_desc = mv_eth_tx_desc_get(txq_ctrl, 1);

	if (tx_desc == NULL) {
		/* No resources: Drop */
		pp->dev->stats.tx_dropped++;
		return MV_DROPPED;
	}
	txq_ctrl->txq_count++;

#ifdef CONFIG_MV_ETH_BM_CPU
	tx_cmd |= NETA_TX_BM_ENABLE_MASK | NETA_TX_BM_POOL_ID_MASK(pkt->pool);
	txq_ctrl->shadow_txq[txq_ctrl->shadow_txq_put_i] = (u32) NULL;
#else
	txq_ctrl->shadow_txq[txq_ctrl->shadow_txq_put_i] = (u32) pkt;
#endif /* CONFIG_MV_ETH_BM_CPU */

	mv_eth_shadow_inc_put(txq_ctrl);
	l3_status = (0xE << NETA_TX_L3_OFFSET_OFFS) | NETA_TX_IP_CSUM_MASK | (0x5 << NETA_TX_IP_HLEN_OFFS);

	tx_desc->command = l3_status | NETA_TX_L4_CSUM_NOT | NETA_TX_FLZ_DESC_MASK | NETA_TX_F_DESC_MASK
				| NETA_TX_L_DESC_MASK | NETA_TX_PKT_OFFSET_MASK(pkt->offset + MV_ETH_MH_SIZE);

	tx_desc->dataSize    = pkt->bytes;
	tx_desc->bufPhysAddr = pkt->physAddr;
	mv_eth_tx_desc_flush(pp, tx_desc);
	mvNetaTxqPendDescAdd(pp->port, pp->txp, 0, 1);

	return MV_OK;
}

static inline void mv_l2sec_complete_out(unsigned long data)

{
	MV_L2FW_SEC_CESA_PRIV *sec_cesa_priv = (MV_L2FW_SEC_CESA_PRIV *)data;
	MV_U32            ifout;
	MV_BUF_INFO       *pBuf;
	struct eth_port   *pp;
	struct eth_pbuf   *pPkt;
	int oldOfsset;
	MV_STATUS status = MV_FAIL;
	static int counterOfFailed = 0;

	if (!sec_cesa_priv) {
		printk(KERN_INFO "sec_cesa_priv is NULL in %s\n", __func__);
		return;
	}
	ifout = sec_cesa_priv->ifout;

	pBuf = sec_cesa_priv->pBufInfo;
	if (!pBuf) {
		printk(KERN_INFO "pBuf is NULL in %s\n", __func__);
		return;
	}
	pPkt = sec_cesa_priv->pPkt;
	if (!pPkt) {
		printk(KERN_INFO "!pPkt) in %s\n", __func__);
		return;
	}
	pPkt->bytes    = pBuf->dataSize;
	pPkt->bytes   += MV_L2FW_SEC_ESP_OFFSET;
	oldOfsset      = pPkt->offset;
	pPkt->offset   = pPkt->offset - (sizeof(MV_ESP_HEADER) + sizeof(MV_IP_HEADER) + MV_CESA_AES_BLOCK_SIZE);

	pp     = mv_eth_ports[ifout];

	status = mv_eth_l2sec_tx(pPkt, pp);

	pPkt->offset = oldOfsset;

	if (status == MV_DROPPED) {
		struct bm_pool *pool = &mv_eth_pool[pPkt->pool];
		counterOfFailed++;
		mv_eth_pool_put(pool, pPkt);
	 }
}

int mv_l2sec_set_cesa_chan(int port, int cesaChan)
{
	if (cesaChan > (MV_CESA_CHANNELS - 1)) {
		pr_info("non permitted value for CESA channel\n");
		return -EINVAL;
	}

	pr_info("setting cesaChan to %d for port=%d\n", cesaChan, port);

	cesaChanPort[port] = cesaChan;

	return 0;
}

MV_STATUS my_mvSysCesaInit(int numOfSession, int queueDepth, void *osHandle)
{

	MV_CESA_HAL_DATA halData;
	MV_UNIT_WIN_INFO addrWinMap[MAX_TARGETS + 1];
	MV_STATUS status;
	MV_U8 chan;

	status = mvCtrlAddrWinMapBuild(addrWinMap, MAX_TARGETS + 1);

	if (status == MV_OK) {
		for (chan = 0; chan < MV_CESA_CHANNELS; chan++) {
			status = mvCesaTdmaWinInit(chan, addrWinMap);
			if (status != MV_OK) {
				mvOsPrintf("Error, unable to initialize CESA windows for channel(%d)\n", chan);
				break;
			}
			halData.sramPhysBase[chan] = (MV_ULONG)mv_crypto_virt_base_get(chan);
			halData.sramVirtBase[chan] = (MV_U8 *)mv_crypto_virt_base_get(chan);
			halData.sramOffset[chan] = 0;
		}

		if (status == MV_OK) {
		halData.ctrlModel = mvCtrlModelGet();
		halData.ctrlRev = mvCtrlRevGet();
			status = mvCesaHalInit(numOfSession, queueDepth,
					osHandle, &halData);
		}
	}

	return status;

}

void mv_l2sec_cesa_start(void)
{

	MV_CESA_MBUF *pMbufSrc[CESA_CHAN], *pMbufDst[CESA_CHAN];
	MV_BUF_INFO *pCesaBufs[CESA_CHAN], *pFragsSrc[CESA_CHAN], *pFragsDst[CESA_CHAN];
	MV_CESA_COMMAND *cesaCmdArrTmp;
	MV_BUF_INFO *pCesaBufsTmp;
	int chan;
	int i, j, idx;
	char *pBuf;

	for (chan = 0; chan < CESA_CHAN; chan++) {
		MALLOC_AND_CLEAR(cesaCmdArray[chan], sizeof(MV_CESA_COMMAND) * CESA_DEF_REQ_SIZE);
		MALLOC_AND_CLEAR(pMbufSrc[chan], sizeof(MV_CESA_MBUF) * CESA_DEF_REQ_SIZE);
		MALLOC_AND_CLEAR(pFragsSrc[chan], sizeof(MV_BUF_INFO) * L2SEC_CESA_BUF_NUM * CESA_DEF_REQ_SIZE);
		MALLOC_AND_CLEAR(pMbufDst[chan], sizeof(MV_CESA_MBUF) * CESA_DEF_REQ_SIZE);
		MALLOC_AND_CLEAR(pFragsDst[chan], sizeof(MV_BUF_INFO) * L2SEC_CESA_BUF_NUM * CESA_DEF_REQ_SIZE);
		MALLOC_AND_CLEAR(pCesaBufs[chan], sizeof(MV_BUF_INFO) * L2SEC_CESA_BUF_NUM * CESA_DEF_REQ_SIZE);

		idx = 0;
		pCesaBufsTmp = pCesaBufs[chan];
		cesaCmdArrTmp = cesaCmdArray[chan];

		for (i = 0; i < CESA_DEF_REQ_SIZE; i++) {
			pBuf = mvOsIoCachedMalloc(cesaOSHandle, L2SEC_CESA_BUF_SIZE * L2SEC_CESA_BUF_NUM * 2,
					  &pCesaBufsTmp[i].bufPhysAddr, &pCesaBufsTmp[i].memHandle);
			if (pBuf == NULL) {
				mvOsPrintf("testStart: Can't malloc %d bytes for pBuf\n", L2SEC_CESA_BUF_SIZE * L2SEC_CESA_BUF_NUM * 2);
				return;
			}

			memset(pBuf, 0, L2SEC_CESA_BUF_SIZE * L2SEC_CESA_BUF_NUM * 2);
			mvOsCacheFlush(cesaOSHandle, pBuf, L2SEC_CESA_BUF_SIZE * L2SEC_CESA_BUF_NUM * 2);
			if (pBuf == NULL) {
				mvOsPrintf("Can't allocate %d bytes for req_%d buffers\n",
						L2SEC_CESA_BUF_SIZE * L2SEC_CESA_BUF_NUM * 2, i);
				return;
			}

			pCesaBufsTmp[i].bufVirtPtr = (MV_U8 *) pBuf;
			pCesaBufsTmp[i].bufSize = L2SEC_CESA_BUF_SIZE * L2SEC_CESA_BUF_NUM * 2;

			cesaCmdArrTmp[i].pSrc = &pMbufSrc[chan][i];
			cesaCmdArrTmp[i].pSrc->pFrags = &pFragsSrc[chan][idx];
			cesaCmdArrTmp[i].pSrc->numFrags = L2SEC_CESA_BUF_NUM;
			cesaCmdArrTmp[i].pSrc->mbufSize = 0;

			cesaCmdArrTmp[i].pDst = &pMbufDst[chan][i];
			cesaCmdArrTmp[i].pDst->pFrags = &pFragsDst[chan][idx];
			cesaCmdArrTmp[i].pDst->numFrags = L2SEC_CESA_BUF_NUM;
			cesaCmdArrTmp[i].pDst->mbufSize = 0;

			for (j = 0; j < L2SEC_CESA_BUF_NUM; j++) {
				cesaCmdArrTmp[i].pSrc->pFrags[j].bufVirtPtr = (MV_U8 *) pBuf;
				cesaCmdArrTmp[i].pSrc->pFrags[j].bufSize = L2SEC_CESA_BUF_SIZE;
				pBuf += L2SEC_CESA_BUF_SIZE;
				cesaCmdArrTmp[i].pDst->pFrags[j].bufVirtPtr = (MV_U8 *) pBuf;

				cesaCmdArrTmp[i].pDst->pFrags[j].bufSize = L2SEC_CESA_BUF_SIZE;
				pBuf += L2SEC_CESA_BUF_SIZE;
			}
		idx += L2SEC_CESA_BUF_NUM;
		}

		MALLOC_AND_CLEAR(cesaMbufArray[chan], sizeof(MV_CESA_MBUF) * CESA_DEF_REQ_SIZE);
		MALLOC_AND_CLEAR(pBufInfoArray[chan], sizeof(MV_BUF_INFO) * MV_L2FW_SEC_REQ_Q_SIZE);
		MALLOC_AND_CLEAR(cesaPrivArray[chan],
				sizeof(MV_L2FW_SEC_CESA_PRIV) * (CESA_DEF_REQ_SIZE + MV_L2FW_SEC_REQ_Q_SIZE));

	} /*for chan*/

	printk(KERN_INFO "start finished in %s\n", __func__);
}

/*
 * nfp sec Interrupt handler routine.
 */



static irqreturn_t
mv_l2sec_interrupt_handler(int irq, void *arg)
{
	MV_CESA_RESULT  	result;
	MV_STATUS               status;
	int chan;

	chan = (irq == CESA_IRQ(0)) ? 0 : 1;

	/* clear interrupts */
	MV_REG_WRITE(MV_CESA_ISR_CAUSE_REG(chan), 0);
#ifndef CONFIG_MV_CESA_INT_PER_PACKET
	while (1) {
#endif
	/* Get Ready requests */
	status = mvCesaReadyGet(chan, &result);
	if (status != MV_OK) {
#ifdef CONFIG_MV_CESA_INT_PER_PACKET
		printk(KERN_ERR "ERROR: Ready get return %d\n", status);
		return IRQ_HANDLED;
#else
			break;
#endif
	}
	/* handle result */
	if (atomic_read(&req_count) > (MV_L2FW_SEC_REQ_Q_SIZE - 4)) {
		/*must take sure that no tx_done will happen on the same time.. */
		MV_L2FW_SEC_CESA_PRIV *req_priv = (MV_L2FW_SEC_CESA_PRIV *)result.pReqPrv;
		struct eth_pbuf *pPkt = req_priv->pPkt;
		struct bm_pool *pool = &mv_eth_pool[pPkt->pool];
		printk(KERN_ERR "Error: Q request is full - TBD test.\n");
		mv_eth_pool_put(pool, pPkt);
		cesaFullResBuf[chan]++;
		return IRQ_HANDLED;
	}

	req_array[req_empty] = (MV_L2FW_SEC_CESA_PRIV *)result.pReqPrv;
	req_empty = (req_empty + 1) % MV_L2FW_SEC_REQ_Q_SIZE;
	atomic_inc(&req_count);
#ifndef CONFIG_MV_CESA_INT_PER_PACKET
	}
#endif
	tasklet_hi_schedule(&l2sec_tasklet);

	return IRQ_HANDLED;
}

void mv_l2sec_req_handler(unsigned long dummy)
{
	int req_count_init = atomic_read(&req_count);
	int counter = req_count_init;

	while (counter != 0) {
		mv_l2sec_complete_out((unsigned long)req_array[req_ready]);
		req_ready = (req_ready + 1) % MV_L2FW_SEC_REQ_Q_SIZE;
		counter--;
	}
	atomic_sub(req_count_init, &req_count);

}


void mv_l2sec_open_cesa_session(void)
{
	unsigned char sha1Key[]  = {0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc, 0xde, 0xf0,
								0x24, 0x68, 0xac, 0xe0, 0x24, 0x68, 0xac, 0xe0,
								0x13, 0x57, 0x9b, 0xdf};
	/* sizeof(cryptoKey) should be 128 for AES-128 */
	unsigned char cryptoKey[] = {0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef,
									0x02, 0x46, 0x8a, 0xce, 0x13, 0x57, 0x9b, 0xdf};

	int i;
	MV_L2FW_SEC_SA_ENTRY sa;
	MV_CESA_OPEN_SESSION os;
	unsigned short digest_size = 0;
	memset(&sa, 0, sizeof(MV_L2FW_SEC_SA_ENTRY));
	memset(&os, 0, sizeof(MV_CESA_OPEN_SESSION));

	os.operation       = MV_CESA_MAC_THEN_CRYPTO;
	os.cryptoAlgorithm = MV_CESA_CRYPTO_AES;
	os.macMode         = MV_CESA_MAC_HMAC_SHA1;
	digest_size        = MV_CESA_SHA1_DIGEST_SIZE;
	os.cryptoMode      = MV_CESA_CRYPTO_CBC;

	for (i = 0; i < sizeof(cryptoKey); i++)
		os.cryptoKey[i] = cryptoKey[i];

	os.cryptoKeyLength = sizeof(cryptoKey);

	for (i = 0; i < sizeof(sha1Key); i++)
		os.macKey[i] = sha1Key[i];
	os.macKeyLength = sizeof(sha1Key);
	os.digestSize = digest_size;

	if (mvCesaSessionOpen(&os, (short *)&(sa.sid)))
		printk(KERN_INFO "mvCesaSessionOpen failed in %s\n", __func__);
}

static void mv_l2sec_casa_param_init(void)
{
	const u8 da_addr[] = {0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff};
	const u8 sa_addr[] = {0xab, 0xac, 0xad, 0xae, 0xaf, 0xaa};

	memset(&sa, 0, sizeof(MV_L2FW_SEC_SA_ENTRY));
	sa.digestSize = MV_CESA_SHA1_DIGEST_SIZE;
	sa.ivSize = MV_CESA_AES_BLOCK_SIZE;
	sa.spi = 3;

	sa.tunProt = MV_L2FW_SEC_TUNNEL;
	sa.encap   = MV_L2FW_SEC_ESP;
	sa.seqNum  = 4;
	sa.tunnelHdr.sIp = 0x6400A8C0;
	sa.tunnelHdr.dIp = 0x6401A8C0;
	sa.tunnelHdr.outIfIndex = 0;
	sa.lifeTime = 0;

	sa.secOp = MV_L2FW_SEC_ENCRYPT;
	memcpy(sa.tunnelHdr.dstMac, da_addr, 6);
	memcpy(sa.tunnelHdr.srcMac, sa_addr, 6);

}

int mv_l2sec_cesa_init(void)
{
	int chan, mask;
	printk(KERN_INFO "%s: start.\n", __func__);
	if (mvCtrlPwrClckGet(CESA_UNIT_ID, 0) == MV_FALSE)
		return 0;

	if (MV_OK != my_mvSysCesaInit(1, 256, NULL)) {
		pr_err("%s: cesa init failed.\n", __func__);
		return EINVAL;
	}

#ifdef CONFIG_MV_CESA_INT_COALESCING_SUPPORT
	mask = MV_CESA_CAUSE_EOP_COAL_MASK;
#else
	mask = MV_CESA_CAUSE_ACC_DMA_MASK;
#endif

	for (chan = 0 ; chan < CESA_CHAN; chan++) {
		/* clear and unmask channel 0 interrupt */
		MV_REG_WRITE(MV_CESA_ISR_CAUSE_REG(chan), 0);
		MV_REG_WRITE(MV_CESA_ISR_MASK_REG(chan), mask);

		/* register channel 0 interrupt */
		if (request_irq(CESA_IRQ(chan), mv_l2sec_interrupt_handler, (IRQF_DISABLED), "cesa", NULL)) {
			printk(KERN_INFO "%s: cannot assign irq %x\n", __func__, CESA_IRQ(chan));
			return -EINVAL;
		}

		cesaChanPort[chan] = 0;
		cesaPrivIndx[chan] = 0;
		cesaCmdIndx[chan] = 0;
		cesaFullResBuf[chan] = 0;
		spin_lock_init(&cesa_lock[chan]);
	}

	tasklet_init(&l2sec_tasklet, mv_l2sec_req_handler, (unsigned long) 0);
	atomic_set(&req_count, 0);

	mv_l2sec_casa_param_init();
	mv_l2sec_open_cesa_session();
	mv_l2sec_cesa_start();
	printk(KERN_INFO "%s: done.\n", __func__);

	return 0;
}


void mv_l2sec_build_tunnel(MV_BUF_INFO *pBuf, MV_L2FW_SEC_SA_ENTRY *pSAEntry)
{
	MV_IP_HEADER *pIpHdr, *pIntIpHdr;
	MV_U16 newIpTotalLength;

	newIpTotalLength = pBuf->dataSize - sizeof(MV_802_3_HEADER);
	pIpHdr = (MV_IP_HEADER *) (pBuf->bufVirtPtr + sizeof(MV_802_3_HEADER));

	pIntIpHdr = (MV_IP_HEADER *) ((MV_U8 *) (pIpHdr) + sizeof(MV_IP_HEADER) + sizeof(MV_ESP_HEADER) +
				      pSAEntry->ivSize);

	/* TBD - review below settings in RFC */
	pIpHdr->version = 0x45;
	pIpHdr->tos = 0;
	pIpHdr->checksum = 0;
	pIpHdr->totalLength = MV_16BIT_BE(newIpTotalLength);
	pIpHdr->identifier = 0;
	pIpHdr->fragmentCtrl = 0;
	pIpHdr->ttl = pIntIpHdr->ttl - 1;
	pIpHdr->protocol = MV_IP_PROTO_ESP;
	pIpHdr->srcIP = pSAEntry->tunnelHdr.sIp;
	pIpHdr->dstIP = pSAEntry->tunnelHdr.dIp;

	return;
}


/* Append sequence number and spi, save some space for IV */
void mv_l2sec_build_esp_hdr(MV_BUF_INFO *pBuf, MV_L2FW_SEC_SA_ENTRY *pSAEntry)
{
	MV_ESP_HEADER *pEspHdr;

	pEspHdr = (MV_ESP_HEADER *) (pBuf->bufVirtPtr + sizeof(MV_802_3_HEADER) + sizeof(MV_IP_HEADER));
	pEspHdr->spi = pSAEntry->spi;
	pSAEntry->seqNum++;
	pEspHdr->seqNum = MV_32BIT_BE(pSAEntry->seqNum);
}

void mv_l2sec_build_mac(MV_BUF_INFO *pBuf, MV_L2FW_SEC_SA_ENTRY *pSAEntry)
{
	MV_802_3_HEADER *pMacHdr;
	pMacHdr = (MV_802_3_HEADER *) ((MV_U8 *) (pBuf->bufVirtPtr));

	memcpy(pMacHdr, &pSAEntry->tunnelHdr.dstMac, 12);
	pMacHdr->typeOrLen = 0x08;/* stands for IP protocol code 16bit swapped */
	return;
}


MV_STATUS mv_l2sec_esp_process(struct eth_pbuf *pPkt, MV_BUF_INFO *pBuf, MV_L2FW_SEC_SA_ENTRY *pSAEntry,
				struct eth_port *newpp, int channel, int inPort)
{
	MV_CESA_COMMAND	*pCesaCmd;
	MV_CESA_MBUF *pCesaMbuf;
	MV_L2FW_SEC_CESA_PRIV *pCesaPriv;
	MV_STATUS status;
	MV_IP_HEADER *pIpHdr;
	int cmdIndx = cesaCmdIndx[channel];
	int privIndx = cesaPrivIndx[channel];

	pCesaCmd  = &cesaCmdArray[channel][cmdIndx];
	pCesaMbuf = &cesaMbufArray[channel][cmdIndx];

	cmdIndx = (cmdIndx + 1) % CESA_DEF_REQ_SIZE;
	cesaCmdIndx[channel] = cmdIndx;

	pCesaPriv = &cesaPrivArray[channel][privIndx];

	privIndx = (privIndx + 1) % (CESA_DEF_REQ_SIZE + MV_L2FW_SEC_REQ_Q_SIZE);
	cesaPrivIndx[channel] = privIndx;

	pCesaPriv->pBufInfo = pBuf;
	pCesaPriv->pSaEntry = pSAEntry;
	pCesaPriv->pCesaCmd = pCesaCmd;

	pCesaPriv->pPkt   = pPkt;
	pCesaPriv->ifout  = newpp->port;
	pCesaPriv->inPort = inPort;
	/*
	 *  Fix, encrypt/decrypt the IP payload only, --BK 20091027
	 */
	pIpHdr = (MV_IP_HEADER *)(pBuf->bufVirtPtr + sizeof(MV_802_3_HEADER));
	pBuf->dataSize = MV_16BIT_BE(pIpHdr->totalLength) + sizeof(MV_802_3_HEADER);

	/* after next command, pBuf->bufVirtPtr will point to ESP */
	pBuf->bufVirtPtr += MV_L2FW_SEC_ESP_OFFSET;
	pBuf->bufPhysAddr += MV_L2FW_SEC_ESP_OFFSET;
	pBuf->dataSize -= MV_L2FW_SEC_ESP_OFFSET;

	pBuf->bufAddrShift -= MV_L2FW_SEC_ESP_OFFSET;
	pCesaMbuf->pFrags = pBuf;
	pCesaMbuf->numFrags = 1;
	pCesaMbuf->mbufSize = pBuf->dataSize;

	pCesaMbuf->pFrags->bufSize = pBuf->dataSize;

	pCesaCmd->pReqPrv = (void *)pCesaPriv;
	pCesaCmd->sessionId = pSAEntry->sid;
	pCesaCmd->pSrc = pCesaMbuf;
	pCesaCmd->pDst = pCesaMbuf;
	pCesaCmd->skipFlush = MV_TRUE;

	/* Assume ESP */
	pCesaCmd->cryptoOffset = sizeof(MV_ESP_HEADER) + pSAEntry->ivSize;
	pCesaCmd->cryptoLength =  pBuf->dataSize - (sizeof(MV_ESP_HEADER)
				  + pSAEntry->ivSize + pSAEntry->digestSize);
	pCesaCmd->ivFromUser = 0; /* relevant for encode only */
	pCesaCmd->ivOffset = sizeof(MV_ESP_HEADER);
	pCesaCmd->macOffset = 0;
	pCesaCmd->macLength = pBuf->dataSize - pSAEntry->digestSize;


	if ((pCesaCmd->digestOffset != 0) && ((pCesaCmd->digestOffset%4)))  {
		printk(KERN_INFO "pBuf->dataSize=%d pSAEntry->digestSize=%d in %s\n",
			pBuf->dataSize, pSAEntry->digestSize, __func__);
		printk(KERN_INFO "pCesaCmd->digestOffset=%d in %s\n",
			pCesaCmd->digestOffset, __func__);
	}

	pCesaCmd->digestOffset = pBuf->dataSize - pSAEntry->digestSize ;

	disable_irq(CESA_IRQ(channel));
	status = mvCesaAction(channel, pCesaCmd);
	enable_irq(CESA_IRQ(channel));


	if (status != MV_OK) {
		pSAEntry->stats.rejected++;
		mvOsPrintf("%s: mvCesaAction failed %d\n", __func__, status);
	}
	return status;
}

MV_STATUS mv_l2sec_out_going(struct eth_pbuf *pkt, MV_BUF_INFO *pBuf, MV_L2FW_SEC_SA_ENTRY *pSAEntry,
			struct eth_port *new_pp, int inPort, int chan)
{
	MV_U8 *pTmp;
	MV_U32 cryptoSize, encBlockMod, dSize;
	/* CESA Q is full drop. */
	if (cesaReqResources[chan] <= 1)
		return MV_DROPPED;

	cryptoSize = pBuf->dataSize - sizeof(MV_802_3_HEADER);

	/* Align buffer address to beginning of new packet - TBD handle VLAN tag, LLC */
	dSize = pSAEntry->ivSize + sizeof(MV_ESP_HEADER) + sizeof(MV_IP_HEADER);
	pBuf->bufVirtPtr -= dSize;
	pBuf->bufPhysAddr -= dSize;
	pBuf->dataSize += dSize;
	pBuf->bufAddrShift += dSize;

	encBlockMod = (cryptoSize % MV_L2FW_SEC_ENC_BLOCK_SIZE);
	/* leave space for padLen + Protocol */
	if (encBlockMod > 14) {
		encBlockMod =  MV_L2FW_SEC_ENC_BLOCK_SIZE - encBlockMod;
		encBlockMod += MV_L2FW_SEC_ENC_BLOCK_SIZE;
	} else
		encBlockMod =  MV_L2FW_SEC_ENC_BLOCK_SIZE - encBlockMod;

	pBuf->dataSize += encBlockMod;

	pTmp = pBuf->bufVirtPtr + pBuf->dataSize;
	memset(pTmp - encBlockMod, 0, encBlockMod - 2);
	*((MV_U8 *)(pTmp-2)) = (MV_U8)(encBlockMod-2);
	*((MV_U8 *)(pTmp-1)) = (MV_U8)4;

	pBuf->dataSize += pSAEntry->digestSize;

	mv_l2sec_build_esp_hdr(pBuf, pSAEntry);
	mv_l2sec_build_tunnel(pBuf, pSAEntry);
	mv_l2sec_build_mac(pBuf, pSAEntry);

	return mv_l2sec_esp_process(pkt, pBuf, pSAEntry, new_pp, chan, inPort);
}

MV_STATUS mv_l2sec_handle_esp(struct eth_pbuf *pkt, struct neta_rx_desc *rx_desc, struct eth_port  *new_pp, int inPort)
{
	MV_STATUS res;
	int chan = cesaChanPort[inPort];
	MV_BUF_INFO *pBufInfoArr = pBufInfoArray[chan];
	int cmdIndx = cesaCmdIndx[chan];
	spin_lock(&cesa_lock[chan]);

	pBufInfoArr[cmdIndx].bufAddrShift = 0;
	pBufInfoArr[cmdIndx].dataSize    = pkt->bytes;

	pBufInfoArr[cmdIndx].bufSize     = pkt->bytes;
	pBufInfoArr[cmdIndx].bufVirtPtr  = pkt->pBuf + pkt->offset + MV_ETH_MH_SIZE;

	pBufInfoArr[cmdIndx].bufPhysAddr = mvOsIoVirtToPhy(NULL, pBufInfoArr[cmdIndx].bufVirtPtr);
	pBufInfoArr[cmdIndx].memHandle   = 0;

	res = mv_l2sec_out_going(pkt, &pBufInfoArr[cmdIndx], &sa, new_pp, inPort, chan);

	spin_unlock(&cesa_lock[chan]);
	return res;
}

