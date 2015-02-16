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
   Marvell Commercial License Option

   If you received this File from Marvell and you have entered into a commercial
   license agreement (a "Commercial License") with Marvell, the File is licensed
   to you under the terms of the applicable Commercial License.

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
********************************************************************************
   Marvell BSD License Option

   If you received this File from Marvell, you may opt to use, redistribute and/or
   modify this File under the following licensing terms.
   Redistribution and use in source and binary forms, with or without modification,
   are permitted provided that the following conditions are met:

*   Redistributions of source code must retain the above copyright notice,
    this list of conditions and the following disclaimer.

*   Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution.

*   Neither the name of Marvell nor the names of its contributors may be
    used to endorse or promote products derived from this software without
    specific prior written permission.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
   ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
   WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
   DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
   ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
   (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
   LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
   ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*******************************************************************************
* mv_prestera_smi.c
*
* DESCRIPTION:
*       functions in kernel mode special for prestera_smi.
*
* DEPENDENCIES:
*
*******************************************************************************/
#include "mvOs.h"
#include "mv_prestera.h"
#include "mv_prestera_smi_glob.h"
#include "mv_pss_api.h"

#undef MV_DEBUG

/* defines  */
#ifdef MV_DEBUG
#define dprintk(a...) printk(a)
#else
#define dprintk(a...)
#endif

/* local variables and variables */
static int presteraSmi_initialized = -1;

static int rx_DSR = -1;			/* rx DSR invocation counter */
static int tx_DSR = -1;			/* tx DSR invocation counter */

struct semaphore *netIfIntTaskSemPtr;   /*  netIfIntTask Signalling sema */
struct semaphore netIfIntTaskSem;


/******************************************************************************/
/*********************** ethernet port FIFO section ***************************/
/******************************************************************************/
static unsigned long	*fifoPtr;	/* the FIFO pointer */
static unsigned long	occupied;	/* occupied segments counter */
static unsigned long	fifoSize;	/* FIFO size */
static unsigned long	*frontPtr;	/* FIFO front pointer */
static unsigned long	*rearPtr;	/* FIFO rear pointer */
static unsigned long	*firstPtr;	/* first element in FIFO */
static unsigned long	*lastPtr;	/* last element in FIFO */

/*******************************************************************************
* ethPortFifoInit
*
* DESCRIPTION:  This routine allocates kernel memory for the FIFO. It is called
*               twice: once for the Rx and once for the Tx complete. The second
*               invocation allocates the memory and sets the FIFO pointers.
*
* INPUTS:
*       numOfElem - the number of elements (buffers and control data) needed
*                   for Rx/TxEnd
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       -ENOMEM - if there is no sufficiant memory
*       0 - on success.
*
*******************************************************************************/
long ethPortFifoInit(unsigned long numOfElem)
{
	if (numOfElem == 0) {
		printk(KERN_ERR "ethPortFifoInit:Err numOfElem is 0\n");
		return -EPERM;
	}

	if (fifoSize == 0)
		fifoSize = numOfElem;
	else {
		if (fifoPtr != NULL) {
			printk(KERN_ERR "ethPortFifoInit: FIFIO allready initialized\n");
			return -EPERM;
		}

		fifoSize += numOfElem;

		fifoPtr = kmalloc((1 + fifoSize) * sizeof(unsigned long), GFP_KERNEL);

		if (!fifoPtr) {
			printk(KERN_ERR "ethPortFifoInit: Failed allocating memory for FIFO\n");
			return -ENOMEM;
		}

		frontPtr = &fifoPtr[0];
		rearPtr  = &fifoPtr[0];
		firstPtr = &fifoPtr[0];
		lastPtr  = &fifoPtr[1 + fifoSize];
	}

	return 0;
}

/*******************************************************************************
* ethPortFifoEnQueue
*
* DESCRIPTION:  This routine adds the new data in the FIFO front.
*
* INPUTS:
*       elem - the element data to insert in the FIFO front
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       1 - FIFO is full, insertion failed!
*       0 - on success.
*
*******************************************************************************/
long ethPortFifoEnQueue(unsigned long elem)
{
	unsigned long    *frontTmpPtr;

	frontTmpPtr = frontPtr;

	frontTmpPtr++;

	/* need to wrap ? */
	if (frontTmpPtr >= lastPtr)
		frontTmpPtr = firstPtr;

	if (frontTmpPtr == rearPtr) {
		/* fifo was full, insertion failed */
		printk(KERN_ERR "ethPortFifoInsert: fifo full err\n");
		return 1;
	} else {
		/* success, put in the data and update fifo control front ptr */
		*frontPtr = elem;
		frontPtr = frontTmpPtr;
		occupied++;
	}
	return 0;
}

/*******************************************************************************
* ethPortFifoDeQueue
*
* DESCRIPTION:  This routine gets the first data element from the FIFO rear.
*
* INPUTS:
*       None.
*
* OUTPUTS:
*       elemPtr <- pointer to update with the element got from FIFO rear
*
* RETURNS:
*       1 - FIFO is empty.
*       0 - on success.
*
*******************************************************************************/
long ethPortFifoDeQueue(unsigned long *elemPtr)
{
	/* empty FIFO */
	if (rearPtr == frontPtr) {
		printk(KERN_ERR "ethPortFifoDeQueue: fifo is EMPTY\n");
		return 1;
	}

	/* get the data */
	*elemPtr = *rearPtr;

	rearPtr++;

	occupied--;

	/* need to wrap ? */
	if (rearPtr == lastPtr)
		rearPtr = firstPtr;

	return 0;
}

/*******************************************************************************
* ethPortFifoEnQueuePossible
*
* DESCRIPTION:  This routine returns a status if the number of elements can be
*               inserted to FIFO.
*
* INPUTS:
*       itemsCnt - the number of elements the user wants to insert
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       1 - there in`t enough space for the elements.
*       0 - there is enough space for the elements.
*
*******************************************************************************/
long ethPortFifoEnQueuePossible(unsigned long elemCnt)
{
	if (fifoSize - occupied > elemCnt)
		return 0;
	else
		return 1;
}

/******************************************************************************/
/*********************** ethernet port FIFO section end ***********************/
/******************************************************************************/
/*******************************************************************************
* dropThePacket
*
* DESCRIPTION:  This routine drops the packet.
*
*
* INPUTS:
*       ctrlSeg - packet segment control
*
* OUTPUTS:
*       None
*
* RETURNS:
*       <0 - the errno to pass to user app.
*
*******************************************************************************/
static int dropThePacket(unsigned long ctrlSeg)
{
	unsigned long	readCnt;
	unsigned long	sink;
	unsigned long	*segmentPtr;
	unsigned long	queueNum;
	unsigned long	segNumber;

	/* get the number of segments of the packet */
	segNumber = (ETH_PORT_FIFO_ELEM_CNT_MASK & ctrlSeg);

	/* allocate mem to copy the segments to */
	segmentPtr = kmalloc(segNumber * sizeof(unsigned long), GFP_KERNEL);

	if (!segmentPtr) {
		printk(KERN_ERR "dropThePacket: Failed allocating memory\n");
		return -ENOMEM;
	}
	readCnt = 0;

	/* copy the segments from FIFO to allocated memory */
	while (readCnt < segNumber) {
		/* read the segment pointer */
		if (ethPortFifoDeQueue(&segmentPtr[readCnt]) != 0) {
			panic("dropThePacket: expecting more segments in fifo");
			return -EIO;
		}
		readCnt++;

		/* read the segment length, not needed by free routine */
		if (ethPortFifoDeQueue(&sink) != 0) {
			panic("dropThePacket: expecting more segments in fifo");
			return -EIO;
		}
	}
	/* get the queue number from control segment */
	queueNum = (ctrlSeg & ETH_PORT_FIFO_QUE_NUM_MASK) >>
		   ETH_PORT_FIFO_QUE_NUM_OFFSET;

	bspEthRxPacketFree((unsigned char **)segmentPtr, readCnt, queueNum);
	kfree(segmentPtr);
	return -ENOBUFS;
}

/*******************************************************************************
* prestera_smi_read
*
* DESCRIPTION:  This routine reads a packet from the network interface FIFO. The
*               first segment in FIFO is the control, which includes some
*               additional information regarding the packet. The next segments
*               are the packet data and for Rx packets, the segment lengths.
*
* INPUTS:
*       filp    - device descriptor
*       count   - the number of segments in the buffer
*       f_pos   - position in the file (not used)
*
* OUTPUTS:
*       buf     <- the buffer to put the packet segments in
*
* RETURNS:
*       >0 - the number of segments in the read packet.
*       0 - there are no more packets to read.
*
*******************************************************************************/
ssize_t prestera_smi_read(struct file	*filp,
			 char	*buf,
			 size_t count,
			 loff_t	*f_pos)
{
	ssize_t readCnt;
	unsigned long segNumber;
	static unsigned long fifoData[MAX_SEG * 2 + 1];
	unsigned long wordNumber;

	readCnt = 0;

	/* read the first segment (control segment) from  FIFO */
	if (ethPortFifoDeQueue(&fifoData[readCnt++]) != 0)
		/* fifo is empty, no more data */
		return 0;

	/* extract from control segment needed data */
	segNumber = (ETH_PORT_FIFO_ELEM_CNT_MASK & fifoData[0]);
	dprintk("presteraSmi_read, segNumber=%ld\n", segNumber);

	/* extract from the fifo the data segments */
	if (ETH_PORT_FIFO_TYPE_RX_MASK & fifoData[0]) {
		wordNumber = (segNumber * 2) + 1;

		/* validate the number of words is not too big to copy to user */
		if ((wordNumber * sizeof(unsigned long)) > count) {
			/* The control segments indicates too many segments for the packet, */
			/* we can not pass it to user, it is dropped and freed!             */
			return dropThePacket(fifoData[0]);
		}
		/* RX packet segments */
		while (readCnt < wordNumber) {
			if (ethPortFifoDeQueue(&fifoData[readCnt]) != 0) {
				panic("presteraSmi_read: expecting more segments in fifo");
				return -EIO;
			}
			readCnt++;

			if (ethPortFifoDeQueue(&fifoData[readCnt]) != 0) {
				panic("presteraSmi_read: expecting more segments in fifo");
				return -EIO;
			}
			readCnt++;
		}
	} else { /* TX Complete packet segments */
		wordNumber = segNumber + 1;

		/* validate the number of words is not too big to copy to user */
		if ((wordNumber * sizeof(unsigned long)) > count) {
			/* return the first segment back to the fifo */
			if (ethPortFifoEnQueue(fifoData[0]) != 0) {
				panic("presteraSmi_read: expecting that fifo is not full");
				return -EIO;
			}
			/* put the rest of the segments back to the fifo*/
			while (readCnt < wordNumber) {
				if (ethPortFifoDeQueue(&fifoData[readCnt]) != 0) {
					panic("presteraSmi_read: expecting more segments in fifo");
					return -EIO;
				}
				if (ethPortFifoEnQueue(fifoData[readCnt]) != 0) {
					panic("presteraSmi_read: expecting that fifo is not full");
					return -EIO;
				}
				readCnt++;
			}
			return -ENOBUFS;
		}

		while (readCnt < wordNumber) {
			if (ethPortFifoDeQueue(&fifoData[readCnt]) != 0) {
				panic("presteraSmi_read: expecting more segments in fifo");
				return -EIO;
			}
			readCnt++;
		}
	}

	readCnt *= sizeof(unsigned long);

	if (copy_to_user((char *)buf, (unsigned long *)fifoData, readCnt)) {
		printk(KERN_ERR "presteraSmi_read: copy_to_user FAULT\n");
		return -EFAULT;
	}

	return readCnt;
}

/*******************************************************************************
* prestera_smi_write
*
* DESCRIPTION:  This routine sends the packet pointed by the segments in the buf
*               poiner over the network interface.
*
* INPUTS:
*       filp    - device descriptor
*       buf     - the buffer to be written
*       count   - the number of segments in the buffer
*       f_pos   - position in the file (not used)
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       -1 - FIFO is empty.
*       <0 - on success.
*
*******************************************************************************/
ssize_t prestera_smi_write(struct file	*filp,
			   const char	*buf,
			   size_t	count,
			   loff_t	*f_pos)
{
	unsigned char		*segmentListPtr[MAX_SEG];
	unsigned long		segmentLen[MAX_SEG];
	const unsigned long	*bufPtr;
	unsigned long		txQueue;

	bufPtr = (const unsigned long *)buf;

	if (count > MAX_SEG) {
		printk(KERN_ERR "%s: count too big\n", __func__);
		return -1;
	}

	/* the segment list is first in the bufPtr array */
	if (copy_from_user(segmentListPtr, &bufPtr[0], sizeof(unsigned long) * count)) {
		printk(KERN_ERR "%s: copy_from_user FAULT\n", __func__);
		return -EFAULT;
	}

	/* the segment length is second in the bufPtr array */
	if (copy_from_user(segmentLen, &bufPtr[count], sizeof(unsigned long) * count)) {
		printk(KERN_ERR "%s: copy_from_user FAULT\n", __func__);
		return -EFAULT;
	}

	/* The txQueue is in the 8 leftmost bits of segmentLen[0].
	   segmentLen[0] is very small and will never get to 2^24 size. gc
	   Read about it in
	   cpssEnabler/mainExtDrv/src/gtExtDrv/gtLinuxXcat/gtXcatEthPortControl.c
	 */

	txQueue = (segmentLen[0] & 0xff000000) >> 24;
	segmentLen[0] &= 0x00ffffff;

#ifdef MV_DEBUG
	{
		int i;
		printk(KERN_INFO "txQueue = %ld\n", txQueue);
		printk(KERN_INFO "in %s, Tx ", __func__);
		for (i = 0; i < count; i++)
			printk(KERN_INFO "seg[%d]=0x%X (%d) ", i,
			       (int)segmentListPtr[i], (int)segmentLen[i]);
		printk(KERN_INFO "\n");
	}
#endif

	if (bspEthPortTxQueue(segmentListPtr, segmentLen, count, txQueue)) {
		printk(KERN_ERR "%s: bspEthPortTxQueue err\n", __func__);
		return -1;
	}
	dprintk("%s: EXIT\n", __func__);
	return 1;
}

int prestera_smi_eth_port_rx_dsr_cnt(void)
{
	return (rx_DSR == -1) ? 0 : rx_DSR;
}


int prestera_smi_eth_port_tx_dsr_cnt(void)
{
	return (tx_DSR == -1) ? 0 : tx_DSR;
}


/*******************************************************************************
* prestera_smi_eth_port_rx_dsr
*
* DESCRIPTION:
*       This is the PresteraSMI ethernet port Rx Deferred-Service-Routine (DSR),
*       reponsible for inserting the packet segments and segment lengths to the
*       FIFO. The routine wakes the netIfintTask thread.
*
* INPUTS:
*       segmentList     - A list of pointers to the packets segments.
*       segmentLen      - A list of segement length.
*       numOfSegments   - The number of segment in segment list.
*       queueNum        - the received queue number
*
* OUTPUTS:
*       None.
*
* RETURNS:ppTq
*       0 on success, or
*       1 otherwise.
*
* COMMENTS:
*       None.
*
*******************************************************************************/
unsigned long prestera_smi_eth_port_rx_dsr(unsigned char	*segmentList[],
					  unsigned long		segmentLen[],
					  unsigned long		numOfSegments,
					  unsigned long		queueNum)
{
	unsigned long firstElem;
	int i;

	/* validate that there is ample space for the packet in FIFO */
	if (ethPortFifoEnQueuePossible(numOfSegments * 2 + 1) != 0)
		return 1;

	/* set the first element (packet control) and insert to FIFO */
	firstElem = (queueNum << ETH_PORT_FIFO_QUE_NUM_OFFSET) |
		    numOfSegments |
		    ETH_PORT_FIFO_TYPE_RX_MASK;

	if (ethPortFifoEnQueue(firstElem) != 0) {
		panic("presteraSmi_eth_port_rx_DSR: ethPortFifoEnQueue failed\n");
		return 1;
	}

	/* insert all packet segments and segment lengths to FIFO */
	for (i = 0; i < numOfSegments; i++) {
		if (ethPortFifoEnQueue((unsigned long)segmentList[i]) != 0) {
			panic("presteraSmi_eth_port_rx_DSR: ethPortFifoEnQueue failed\n");
			return 1;
		}

		if (ethPortFifoEnQueue((unsigned long)segmentLen[i]) != 0) {
			panic("presteraSmi_eth_port_rx_DSR: ethPortFifoEnQueue failed\n");
			return 1;
		}
	}
	rx_DSR++;

	/* awake reading process */
	up(netIfIntTaskSemPtr);

	return 0;
}

/*******************************************************************************
* prestera_smi_eth_port_tx_end_dsr
*
* DESCRIPTION:
*       This is the presteraSmi ethernet port Tx Complete Deferred-Service-Routine (DSR),
*       reponsible for inserting the packet segments to the FIFO. The routine
*       wakes the presteraSmi interrupt thread.
*
* INPUTS:
*       segmentList     - A list of pointers to the packets segments.
*       numOfSegments   - The number of segment in segment list.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       0 on success, or
*       1 otherwise.
*
* COMMENTS:
*       None.
*
*******************************************************************************/
unsigned long prestera_smi_eth_port_tx_end_dsr(unsigned char	*segmentList[],
					      unsigned long	numOfSegments)
{
	int i;

	/* validate that there is ample space for the packet in FIFO */
	if (ethPortFifoEnQueuePossible(numOfSegments + 1) != 0) {
		panic("presteraSmi_eth_port_tx_end_DSR: ethPortFifoEnQueue failed\n");
		return 1;
	}

	if (ethPortFifoEnQueue(numOfSegments) != 0) {
		panic("presteraSmi_eth_port_tx_end_DSR: ethPortFifoEnQueue failed\n");
		return 1;
	}

	/* insert all packet segments to FIFO */
	for (i = 0; i < numOfSegments; i++) {
		if (ethPortFifoEnQueue((unsigned long)segmentList[i]) != 0) {
			panic("presteraSmi_eth_port_tx_end_DSR: ethPortFifoEnQueue failed\n");
			return 1;
		}
	}

	tx_DSR++;

	/* awake reading process */
	up(netIfIntTaskSemPtr);

	return 0;
}

/************************************************************************
*
*                   presteraSmi_cleanup
*
************************************************************************/
void prestera_smi_cleanup(void)
{
	presteraSmi_initialized = -1;
}

#ifdef MV_DEBUG
static void ioctl_cmd_pr(unsigned int cmd)
{
	char *dir;

	static const char const *prestera_ioctls[] = {
		[_IOC_NR(PRESTERA_SMI_IOC_WRITEREG)]		= "WRITEREG",
		[_IOC_NR(PRESTERA_SMI_IOC_READREG)]		= "READREG",
		[_IOC_NR(PRESTERA_SMI_IOC_WRITEREGDIRECT)]	= "WRITEREGDIRECT",
		[_IOC_NR(PRESTERA_SMI_IOC_READREGDIRECT)]	= "READREGDIRECT",
		[_IOC_NR(PRESTERA_SMI_IOC_WRITEREGFIELD)]	= "WRITEREGFIELD",
		[_IOC_NR(PRESTERA_SMI_IOC_READREGRAM)]		= "READREGRAM",
		[_IOC_NR(PRESTERA_SMI_IOC_WRITEREGRAM)]		= "WRITEREGRAM",
		[_IOC_NR(PRESTERA_SMI_IOC_READREGVEC)]		= "READREGVEC",
		[_IOC_NR(PRESTERA_SMI_IOC_WRITEREGVEC)]		= "WRITEREGVEC",
		[_IOC_NR(PRESTERA_SMI_IOC_ETHPORTENABLE)]	= "ETHPORTENABLE",
		[_IOC_NR(PRESTERA_SMI_IOC_ETHPORTDISABLE)]	= "ETHPORTDISABLE",
		[_IOC_NR(PRESTERA_SMI_IOC_ETHPORTRXINIT)]	= "ETHPORTRXINIT",
		[_IOC_NR(PRESTERA_SMI_IOC_ETHPORTTXINIT)]	= "ETHPORTTXINIT",
		[_IOC_NR(PRESTERA_SMI_IOC_ETHPORTFREEBUF)]	= "ETHPORTFREEBUF",
		[_IOC_NR(PRESTERA_SMI_IOC_ETHPORTRXBIND)]	= "ETHPORTRXBIND",
		[_IOC_NR(PRESTERA_SMI_IOC_ETHPORTTXBIND)]	= "ETHPORTTXBIND",
		[_IOC_NR(PRESTERA_SMI_IOC_NETIF_WAIT)]		= "NETIF_WAIT",
		[_IOC_NR(PRESTERA_SMI_IOC_TXMODE_SET)]		= "TXMODE_SET",
		[_IOC_NR(PRESTERA_SMI_IOC_CPUCODE_TO_QUEUE)]	= "CPUCODE_TO_QUEUE",
		[_IOC_NR(PRESTERA_SMI_IOC_MUXSET)]		= "MUXSET",
		[_IOC_NR(PRESTERA_SMI_IOC_MUXGET)]		= "MUXGET",
};

	#define PRESTERA_IOCTLS ARRAY_SIZE(prestera_ioctls)

	switch (_IOC_DIR(cmd)) {
	case _IOC_NONE:
		dir = "--";
		break;

	case _IOC_READ:
		dir = "r-";
		break;

	case _IOC_WRITE:
		dir = "-w";
		break;

	case _IOC_READ | _IOC_WRITE:
		dir = "rw";
		break;

	default:
		dir = "*ERR*";
		break;
	}
	printk(KERN_INFO "got ioctl '%c', dir=%s, #%d (0x%08x) ",
		   _IOC_TYPE(cmd), dir, _IOC_NR(cmd), cmd);

	if (_IOC_NR(cmd) < PRESTERA_IOCTLS)
		printk("%s\n", prestera_ioctls[_IOC_NR(cmd)]);
}
#endif /* MV_DEBUG */


/************************************************************************
*
*           presteraSmi_ioctl: ioctl() implementation
*
************************************************************************/
int prestera_smi_ioctl(unsigned int cmd, unsigned long arg)
{
	struct SMI_REG		smiReg;
	struct SMI_REG_RAM_STC	smiRegRam;
	unsigned long		i;
	unsigned long		smiRegVal;
	unsigned int		numOfTxBufs;
	unsigned int		txMode;
	int			retStatus;
	struct MUX_PARAM	muxParam;
	struct RX_INIT_PARAM	rxParam;
	struct RX_FREE_BUF_PARAM	bufFreeParam;
	struct CPU_CODE_TO_QUEUE_PARAM	cpuCodeToQueueParam;

	if (presteraSmi_initialized == -1)
		return -ENODEV;

#ifdef MV_DEBUG
	ioctl_cmd_pr(cmd);
#endif

	/* GETTING DATA */
	switch (cmd) {
	case PRESTERA_SMI_IOC_ETHPORTRXBIND:
	case PRESTERA_SMI_IOC_ETHPORTTXBIND:
	case PRESTERA_SMI_IOC_ETHPORTENABLE:
	case PRESTERA_SMI_IOC_ETHPORTDISABLE:
		break;

	case PRESTERA_SMI_IOC_READREG:
	case PRESTERA_SMI_IOC_WRITEREG:
		/* read and parse user data structurr */
		if (copy_from_user(&smiReg, (struct SMI_REG *)arg, sizeof(struct SMI_REG)))
			goto ioctlFault;
		break;
	case PRESTERA_SMI_IOC_MUXSET:
		/* read and parse user data structurr */
		if (copy_from_user(&muxParam, (struct MUX_PARAM *)arg, sizeof(struct MUX_PARAM)))
			goto ioctlFault;
		break;

	case PRESTERA_SMI_IOC_READREGRAM:
	case PRESTERA_SMI_IOC_WRITEREGRAM:
		if (copy_from_user(&smiRegRam, (struct SMI_REG_RAM_STC *)arg,
				   sizeof(struct SMI_REG_RAM_STC)))
			goto ioctlFault;

	case PRESTERA_SMI_IOC_TXMODE_SET:
		if (copy_from_user(&txMode, (unsigned int *)arg, sizeof(unsigned int)))
			goto ioctlFault;
		break;

	case PRESTERA_SMI_IOC_CPUCODE_TO_QUEUE:
		/* read and parse user data structure */
		if (copy_from_user(&cpuCodeToQueueParam,
				   (struct CPU_CODE_TO_QUEUE_PARAM *)arg,
				   sizeof(struct CPU_CODE_TO_QUEUE_PARAM)))
			goto ioctlFault;
		break;

	case PRESTERA_SMI_IOC_ETHPORTTXINIT:
		/* read and parse user data structure */
		if (copy_from_user(&numOfTxBufs, (unsigned int *)arg, sizeof(unsigned int)))
			goto ioctlFault;
		break;
	case PRESTERA_SMI_IOC_ETHPORTRXINIT:
		/* read and parse user data structure */
		if (copy_from_user(&rxParam, (struct RX_INIT_PARAM *)arg, sizeof(struct RX_INIT_PARAM)))
			goto ioctlFault;
		break;

	case PRESTERA_SMI_IOC_ETHPORTFREEBUF:
		/* read and parse user data structure */
		if (copy_from_user(&bufFreeParam, (struct RX_FREE_BUF_PARAM *)arg, 2 * sizeof(long)))
			goto ioctlFault;
		if (copy_from_user(&bufFreeParam.segmentList,
				   (struct RX_FREE_BUF_PARAM *)(arg + (2 * sizeof(long))),
				   bufFreeParam.numOfSegments * sizeof(char *)))
			goto ioctlFault;
		break;

	case PRESTERA_SMI_IOC_NETIF_WAIT:
		break;

	default:
		printk(KERN_WARNING "Unknown ioctl (%x).\n", cmd);
		break;
	}
	/* DOING SOMETHING */
	switch (cmd) {
	/* Note. Both bspSmiReadReg and bspSmiWriteReg perform indirect operation */

	case PRESTERA_SMI_IOC_READREG:
		/* Read the user params */
		bspSmiReadReg(smiReg.slvId, 0, smiReg.regAddr, &smiReg.value);
		break;

	case PRESTERA_SMI_IOC_WRITEREG:
		/* Write the user params */
		bspSmiWriteReg(smiReg.slvId, 0, smiReg.regAddr, smiReg.value);
		break;
	case PRESTERA_SMI_IOC_MUXSET:
		break;

	case PRESTERA_SMI_IOC_MUXGET:
		break;

	case PRESTERA_SMI_IOC_READREGRAM:
		for (i = 0; i < smiRegRam.arrLen; i++, smiRegRam.addr += 4) {
			bspSmiReadReg(smiRegRam.devSlvId, 0, smiRegRam.addr, &smiRegVal);
			if (copy_to_user((unsigned long *)(smiRegRam.dataArr + i),
					 &smiRegVal,
					 sizeof(smiRegVal)))
				goto ioctlFault;
		}
		break;

	case PRESTERA_SMI_IOC_WRITEREGRAM:
		for (i = 0; i < smiRegRam.arrLen; i++, smiRegRam.addr += 4) {
			if (copy_from_user(&smiRegVal,
					   (unsigned long *)(smiRegRam.dataArr + i),
					   sizeof(smiRegVal)))
				goto ioctlFault;

			bspSmiWriteReg(smiRegRam.devSlvId, 0, smiRegRam.addr, smiRegVal);
		}
		break;

	case PRESTERA_SMI_IOC_ETHPORTENABLE:
		bspEthPortEnable();
		break;

	case PRESTERA_SMI_IOC_ETHPORTDISABLE:
		bspEthPortDisable();
		break;

	case PRESTERA_SMI_IOC_TXMODE_SET:
		bspEthPortTxModeSet((void *)txMode);
		break;

	case PRESTERA_SMI_IOC_CPUCODE_TO_QUEUE:
		bspEthCpuCodeToQueue(cpuCodeToQueueParam.cpuCode,
				      cpuCodeToQueueParam.queue);
		break;

	case PRESTERA_SMI_IOC_ETHPORTTXINIT:
		bspEthInit(1); /*?*/
		dprintk("tx bspEthInit - done\n");
		bspEthPortTxInit(numOfTxBufs);
		ethPortFifoInit(numOfTxBufs * 2);
		break;

	case PRESTERA_SMI_IOC_ETHPORTRXINIT:
		bspEthInit(1); /*?*/
		dprintk("rx: bspEthInit - done\n");

		dprintk("IOCTL_ETHPORTRXINIT:rxParam.rxBufPoolPtr 0x%x\n",
			(unsigned int)rxParam.rxBufPoolPtr);
		retStatus = bspEthPortRxInit(rxParam.rxBufPoolSize,
						rxParam.rxBufPoolPtr,
						rxParam.rxBufSize,
						rxParam.numOfRxBufsPtr,
						rxParam.headerOffset,
						rxParam.rxQNum,
						rxParam.rxQbufPercentage);

		if (retStatus != MV_OK) {
			printk(KERN_ERR "PRESTERA_SMI_IOC_ETHPORTRXINIT,retStatus = %d\n", retStatus);
			goto ioctlFault;
		}
		ethPortFifoInit((*(rxParam.numOfRxBufsPtr)) * 3);

		break;

	case PRESTERA_SMI_IOC_ETHPORTFREEBUF:
		bspEthRxPacketFree(bufFreeParam.segmentList,
					bufFreeParam.numOfSegments,
					bufFreeParam.queueNum);

		break;

	case PRESTERA_SMI_IOC_ETHPORTRXBIND:
		bspEthInputHookAdd((BSP_RX_CALLBACK_FUNCPTR)prestera_smi_eth_port_rx_dsr);

		break;

	case PRESTERA_SMI_IOC_ETHPORTTXBIND:
		bspEthTxCompleteHookAdd((BSP_TX_COMPLETE_CALLBACK_FUNCPTR)prestera_smi_eth_port_tx_end_dsr);
		break;

	case PRESTERA_SMI_IOC_NETIF_WAIT:
		dprintk("netIfIntTaskSemPtr:%p\n", netIfIntTaskSemPtr);

		if (down_interruptible(netIfIntTaskSemPtr))
			return -ERESTARTSYS;
		break;

	default:
		printk(KERN_WARNING "Unknown ioctl (%x).\n", cmd);
		break;
	}

	/* Write back to user */
	switch (cmd) {
	case PRESTERA_SMI_IOC_READREG:
	case PRESTERA_SMI_IOC_READREGDIRECT:
		if (copy_to_user((struct SMI_REG *)arg, &smiReg, sizeof(struct SMI_REG)))
			goto ioctlFault;
		break;

	case PRESTERA_SMI_IOC_MUXGET:
		if (copy_to_user((struct MUX_PARAM *)arg, &muxParam, sizeof(struct MUX_PARAM)))
			goto ioctlFault;
		break;

	case PRESTERA_SMI_IOC_NETIF_WAIT:
	case PRESTERA_SMI_IOC_WRITEREG:
	case PRESTERA_SMI_IOC_MUXSET:
	case PRESTERA_SMI_IOC_READREGRAM:
	case PRESTERA_SMI_IOC_WRITEREGRAM:
	case PRESTERA_SMI_IOC_WRITEREGDIRECT:
	case PRESTERA_SMI_IOC_WRITEREGFIELD:
	case PRESTERA_SMI_IOC_ETHPORTENABLE:
	case PRESTERA_SMI_IOC_ETHPORTDISABLE:
	case PRESTERA_SMI_IOC_TXMODE_SET:
	case PRESTERA_SMI_IOC_CPUCODE_TO_QUEUE:
	case PRESTERA_SMI_IOC_ETHPORTTXINIT:
	case PRESTERA_SMI_IOC_ETHPORTFREEBUF:
	case PRESTERA_SMI_IOC_ETHPORTRXBIND:
	case PRESTERA_SMI_IOC_ETHPORTTXBIND:
	case PRESTERA_SMI_IOC_ETHPORTRXINIT:
		break;

	default:
		printk(KERN_WARNING "Unknown ioctl (%x).\n", cmd);
		break;
	}
	return 0;

ioctlFault:
	printk(KERN_ERR "IOCTL: FAULT\n");
	return -EFAULT;
}

/************************************************************************
*
*                   presteraSmi_init
*
************************************************************************/
int prestera_smi_init(void)
{
	netIfIntTaskSemPtr = &netIfIntTaskSem;

	/* The netIf user process will wait on it */
	sema_init(netIfIntTaskSemPtr, 1);

	presteraSmi_initialized = 1;

	rx_DSR = tx_DSR = 0;

	fifoPtr = NULL;
	occupied = 0;
	fifoSize = 0;
	frontPtr = NULL;
	rearPtr = NULL;
	firstPtr = NULL;
	lastPtr = NULL;

	dprintk("%s done\n", __func__);

	return 0;
}
