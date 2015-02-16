/*******************************************************************************
   Copyright (C) Marvell International Ltd. and its affiliates
*******************************************************************************
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
* pssBspApis.h - bsp APIs
*
* DESCRIPTION:
*       Enable managment of cache memory
*
* DEPENDENCIES:
*       None.
*
*******************************************************************************/
#ifndef __MV_PSS_API
#define __MV_PSS_API

#include "mvTypes.h"

/* Defines */
#define ETH_PHY_TIMEOUT		10000

/* registers offsetes defines */

#define ETH_PHY_SMI_DATA_OFFS		0 /* Data */
#define ETH_PHY_SMI_DATA_MASK		(0xffff << ETH_PHY_SMI_DATA_OFFS)

/* SMI register fields (ETH_PHY_SMI_REG) */
#define ETH_PHY_SMI_DEV_ADDR_OFFS		16 /* PHY device address */
#define ETH_PHY_SMI_DEV_ADDR_MASK		(0x1f << ETH_PHY_SMI_DEV_ADDR_OFFS)

#define ETH_PHY_SMI_REG_ADDR_OFFS		21 /* PHY device register address */
#define ETH_PHY_SMI_REG_ADDR_MASK		(0x1f << ETH_PHY_SMI_REG_ADDR_OFFS)

#define ETH_PHY_SMI_OPCODE_OFFS		26 /* Write/Read opcode */
#define ETH_PHY_SMI_OPCODE_MASK		(3 << ETH_PHY_SMI_OPCODE_OFFS)
#define ETH_PHY_SMI_OPCODE_WRITE	(0 << ETH_PHY_SMI_OPCODE_OFFS)
#define ETH_PHY_SMI_OPCODE_READ		(1 << ETH_PHY_SMI_OPCODE_OFFS)

#define ETH_PHY_SMI_READ_VALID_BIT	27	/* Read Valid  */
#define ETH_PHY_SMI_READ_VALID_MASK	(1 << ETH_PHY_SMI_READ_VALID_BIT)

#define ETH_PHY_SMI_BUSY_BIT		28  /* Busy */
#define ETH_PHY_SMI_BUSY_MASK		(1 << ETH_PHY_SMI_BUSY_BIT)

#define PCIR_BARS	0x10
#define PCIR_BAR(x)	(PCIR_BARS + (x) * 4)

/*
 * dev_id[15:10] bits of DeviceID register of Prestera (0x4C)
 * determine the chip type (xCat or xCat2).
 * dev_id[15:10] == 0x37 stands for xCat
 * dev_id[15:10] == 0x39 stands for xCat2
 */
#define MV_PP_CHIP_TYPE_MASK		0x000FC00
#define MV_PP_CHIP_TYPE_OFFSET		10

/*
 * Typedef: enum bspCacheType
 *
 * Description:
 *	This type defines used cache types
 *
 * Fields:
 *	bspCacheType_InstructionCache_E - cache of commands
 *	bspCacheType_DataCache_E- cache of data
 *
 * Note:
 *	The enum has to be compatible with MV_MGMT_CACHE_TYPE.
 **/
enum bspCacheType {
	bspCacheType_InstructionCache_E,
	bspCacheType_DataCache_E
};

/*
 * Description: Enumeration For PCI interrupt lines.
 *
 * Enumerations:
 *	bspPciInt_PCI_INT_A_E - PCI INT# A
 *	bspPciInt_PCI_INT_B_ - PCI INT# B
 *	bspPciInt_PCI_INT_C - PCI INT# C
 *	bspPciInt_PCI_INT_D - PCI INT# D
 *
 * Assumption:
 *	This enum should be identical to bspPciInt_PCI_INT.
 */
enum bspPciInt_PCI_INT {
	bspPciInt_PCI_INT_A = 1,
	bspPciInt_PCI_INT_B,
	bspPciInt_PCI_INT_C,
	bspPciInt_PCI_INT_D
};

/*
 * enum bspSmiAccessMode
 *
 * Description:
 *	PP SMI access mode.
 *
 * Fields:
 *	bspSmiAccessMode_Direct_E   - direct access mode (single/parallel)
 *	bspSmiAccessMode_inDirect_E - indirect access mode
 *
 * Note:
 *	The enum has to be compatible with MV_MGMT_CACHE_TYPE.
 */
enum bspSmiAccessMode {
	bspSmiAccessMode_Direct_E,
	bspSmiAccessMode_inDirect_E
};

/*  fix erroneous devSlvId from cpss */
#define MV_FIX_DEV_SLAVE_ID_4_LION(devSlvId) {\
	if (devSlvId > 0x13)\
		devSlvId = 0x14;\
	if (devSlvId >= 0x10)\
		devSlvId -= 0x10;\
}

/*******************************************************************************
* BSP_RX_CALLBACK_FUNCPTR
*
* DESCRIPTION:
*       The prototype of the routine to be called after a packet was received
*
* INPUTS:
*	segmentList     - A list of pointers to the packets segments.
*	segmentLen      - A list of segment length.
*	numOfSegments   - The number of segment in segment list.
*	queueNum        - the received queue number
*
* OUTPUTS:
*       None.
*
* RETURNS:
*	MV_TRUE if it has handled the input packet and no further action should
*	be taken with it, or
*	MV_FALSE if it has not handled the input packet and normal processing.
*
* COMMENTS:
*	None.
*
*******************************************************************************/
typedef int (*BSP_RX_CALLBACK_FUNCPTR)(unsigned char *segmentList[],
					unsigned long segmentLen[],
					unsigned long numOfSegments,
					unsigned long queueNum);

/*******************************************************************************
* BSP_TX_COMPLETE_CALLBACK_FUNCPTR
*
* DESCRIPTION:
*       The prototype of the routine to be called after a packet was received
*
* INPUTS:
*       segmentList     - A list of pointers to the packets segments.
*       numOfSegments   - The number of segment in segment list.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       MV_TRUE if it has handled the input packet and no further action should
*               be taken with it, or
*       MV_FALSE if it has not handled the input packet and normal processing.
*
* COMMENTS:
*       None.
*
*******************************************************************************/
typedef int (*BSP_TX_COMPLETE_CALLBACK_FUNCPTR)(unsigned char *segmentList[],
						unsigned long numOfSegments);

/*** reset ***/
/*******************************************************************************
* bspResetInit
*
* DESCRIPTION:
*       This routine calls in init to do system init config for reset.
*
* INPUTS:
*       none.
*
* OUTPUTS:
*       none.
*
* RETURNS:
*       MV_OK      - on success.
*       MV_FAIL    - otherwise.
*
* COMMENTS:
*       None.
*
*******************************************************************************/
int bspResetInit(void);

/*******************************************************************************
* bspReset
*
* DESCRIPTION:
*	This routine calls to reset of CPU.
*
* INPUTS:
*	none.
*
* OUTPUTS:
*	none.
*
* RETURNS:
*	MV_OK      - on success.
*	MV_FAIL    - otherwise.
*
* COMMENTS:
*	None.
*
*******************************************************************************/
int bspReset(void);

/*** PCI ***/
/*******************************************************************************
* bspPciConfigWriteReg
*
* DESCRIPTION:
*       This routine write register to the PCI configuration space.
*
* INPUTS:
*	busNo    - PCI bus number.
*	devSel   - the device devSel.
*	funcNo   - function number.
*	regAddr  - Register offset in the configuration space.
*	data     - data to write.
*
* OUTPUTS:
*	None.
*
* RETURNS:
*	MV_OK   - on success,
*	MV_FAIL - othersise.
*
* COMMENTS:
*
*******************************************************************************/
int bspPciConfigWriteReg(unsigned long busNo,
			 unsigned long devSel,
			 unsigned long funcNo,
			 unsigned long regAddr,
			 unsigned long data);


/*******************************************************************************
* bspPciConfigReadReg
*
* DESCRIPTION:
*       This routine read register from the PCI configuration space.
*
* INPUTS:
*       busNo    - PCI bus number.
*       devSel   - the device devSel.
*       funcNo   - function number.
*       regAddr  - Register offset in the configuration space.
*
* OUTPUTS:
*       data     - the read data.
*
* RETURNS:
*       MV_OK   - on success,
*       MV_FAIL - othersise.
*
* COMMENTS:
*
*******************************************************************************/
int bspPciConfigReadReg(unsigned long	busNo,
			unsigned long	devSel,
			unsigned long	funcNo,
			unsigned long	regAddr,
			unsigned long	*data);

/*******************************************************************************
* bspPciGetResourceStart
*
* DESCRIPTION:
*       This routine performs pci_resource_start.
*       In MIPS64 this function must be used instead of reading the bar
*       directly.
*
* INPUTS:
*       busNo    - PCI bus number.
*       devSel   - the device devSel.
*       funcNo   - function number.
*       barNo    - Bar Number.
*
* OUTPUTS:
*       ResourceStart - the address of the resource.
*
* RETURNS:
*       MV_OK   - on success,
*       MV_FAIL - othersise.
*
* COMMENTS:
*
*******************************************************************************/
int bspPciGetResourceStart(unsigned long	busNo,
				unsigned long	devSel,
				unsigned long	funcNo,
				unsigned long	barNo,
				unsigned long long	*resourceStart);

/*******************************************************************************
* bspPciGetResourceLen
*
* DESCRIPTION:
*       This routine performs pci_resource_len.
*       In MIPS64 this function must be used instead of reading the bar
*       directly.
*
* INPUTS:
*       busNo    - PCI bus number.
*       devSel   - the device devSel.
*       funcNo   - function number.
*       barNo    - Bar Number.
*
* OUTPUTS:
*       ResourceLen - the address of the resource.
*
* RETURNS:
*       MV_OK   - on success,
*       MV_FAIL - othersise.
*
* COMMENTS:
*
*******************************************************************************/
int bspPciGetResourceLen(unsigned long		busNo,
				unsigned long		devSel,
				unsigned long		funcNo,
				unsigned long		barNo,
				unsigned long long	*resourceLen);

/*******************************************************************************
* bspPciFindDev
*
* DESCRIPTION:
*       This routine returns the next instance of the given device (defined by
*       vendorId & devId).
*
* INPUTS:
*       vendorId - The device vendor Id.
*       devId    - The device Id.
*       instance - The requested device instance.
*
* OUTPUTS:
*       busNo    - PCI bus number.
*       devSel   - the device devSel.
*       funcNo   - function number.
*
* RETURNS:
*       MV_OK   - on success,
*       MV_FAIL - othersise.
*
* COMMENTS:
*
*******************************************************************************/
int bspPciFindDev(unsigned short	vendorId,
			unsigned short	devId,
			unsigned long		instance,
			unsigned long		*busNo,
			unsigned long		*devSel,
			unsigned long		*funcNo);

/*******************************************************************************
* bspPciGetIntVec
*
* DESCRIPTION:
*       This routine return the PCI interrupt vector.
*
* INPUTS:
*       pciInt - PCI interrupt number.
*
* OUTPUTS:
*       intVec - PCI interrupt vector.
*
* RETURNS:
*       MV_OK      - on success.
*       MV_FAIL    - otherwise.
*
* COMMENTS:
*       None.
*
*******************************************************************************/
int bspPciGetIntVec(enum bspPciInt_PCI_INT	pciInt,
			void			**intVec);

/*******************************************************************************
* bspPciGetIntMask
*
* DESCRIPTION:
*       This routine return the PCI interrupt vector.
*
* INPUTS:
*       pciInt - PCI interrupt number.
*
* OUTPUTS:
*       intMask - PCI interrupt mask.
*
* RETURNS:
*       MV_OK      - on success.
*       MV_FAIL    - otherwise.
*
* COMMENTS:
*       PCI interrupt mask should be used for interrupt disable/enable.
*
*******************************************************************************/
int bspPciGetIntMask(enum bspPciInt_PCI_INT	pciInt,
			unsigned long		*intMask);

/*******************************************************************************
* bspPciEnableCombinedAccess
*
* DESCRIPTION:
*       This function enables / disables the Pci writes / reads combining
*       feature.
*       Some system controllers support combining memory writes / reads. When a
*       long burst write / read is required and combining is enabled, the master
*       combines consecutive write / read transactions, if possible, and
*       performs one burst on the Pci instead of two. (see comments)
*
* INPUTS:
*       enWrCombine - MV_TRUE enables write requests combining.
*       enRdCombine - MV_TRUE enables read requests combining.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       MV_OK               - on sucess,
*       MV_NOT_SUPPORTED    - if the controller does not support this feature,
*       MV_FAIL             - otherwise.
*
* COMMENTS:
*       1.  Example for combined write scenario:
*           The controller is required to write a 32-bit data to address 0x8000,
*           while this transaction is still in progress, a request for a write
*           operation to address 0x8004 arrives, in this case the two writes are
*           combined into a single burst of 8-bytes.
*
*******************************************************************************/
int bspPciEnableCombinedAccess(int enWrCombine,
					int enRdCombine);

/*** cache ***/
/*******************************************************************************
* bspCacheFlush
*
* DESCRIPTION:
*       Flush to RAM content of cache
*
* INPUTS:
*       type        - type of cache memory data/intraction
*       address_PTR - starting address of memory block to flush
*       size        - size of memory block
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       MV_OK   - on success,
*       MV_FAIL - othersise.
*
* COMMENTS:
*
*******************************************************************************/
int bspCacheFlush(enum bspCacheType	cacheType,
			void			*address_PTR,
			size_t		size);

/*******************************************************************************
* bspCacheInvalidate
*
* DESCRIPTION:
*       Invalidate current content of cache
*
* INPUTS:
*       type        - type of cache memory data/intraction
*       address_PTR - starting address of memory block to flush
*       size        - size of memory block
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       MV_OK   - on success,
*       MV_FAIL - othersise.
*
* COMMENTS:
*
*******************************************************************************/
int bspCacheInvalidate(enum bspCacheType	cacheType,
				void			*address_PTR,
				size_t			size);

/*** DMA ***/
/*******************************************************************************
* bspDmaWrite
*
* DESCRIPTION:
*       Write a given buffer to the given address using the Dma.
*
* INPUTS:
*       address     - The destination address to write to.
*       buffer      - The buffer to be written.
*       length      - Length of buffer in words.
*       burstLimit  - Number of words to be written on each burst.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       MV_OK   - on success,
*       MV_FAIL - othersise.
*
* COMMENTS:
*       1.  The given buffer is allways 4 bytes aligned, any further allignment
*           requirements should be handled internally by this function.
*       2.  The given buffer may be allocated from an uncached memory space, and
*           it's to the function to handle the cache flushing.
*       3.  The Prestera Driver assumes that the implementation of the DMA is
*           blocking, otherwise the Driver functionality might be damaged.
*
*******************************************************************************/
int bspDmaWrite(unsigned long address,
		unsigned long  *buffer,
		unsigned long length,
		unsigned long burstLimit);

/*******************************************************************************
* bspDmaRead
*
* DESCRIPTION:
*       Read a memory block from a given address.
*
* INPUTS:
*       address     - The address to read from.
*       length      - Length of the memory block to read (in words).
*       burstLimit  - Number of words to be read on each burst.
*
* OUTPUTS:
*       buffer  - The read data.
*
* RETURNS:
*       MV_OK   - on success,
*       MV_FAIL - othersise.
*
* COMMENTS:
*       1.  The given buffer is allways 4 bytes aligned, any further allignment
*           requirements should be handled internally by this function.
*       2.  The given buffer may be allocated from an uncached memory space, and
*           it's to the function to handle the cache flushing.
*       3.  The Prestera Driver assumes that the implementation of the DMA is
*           blocking, otherwise the Driver functionality might be damaged.
*
*******************************************************************************/
int bspDmaRead(unsigned long	address,
		unsigned long	length,
		unsigned long	burstLimit,
		unsigned long	*buffer);

/*******************************************************************************
* bspCacheDmaMalloc
*
* DESCRIPTION:
*       Allocate a cache free area for DMA devices.
*
* INPUTS:
*       size_t bytes - number of bytes to allocate
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       pointer to allocated data per success
*       NULL - per failure to allocate space
*
* COMMENTS:
*       None
*
*******************************************************************************/
void *bspCacheDmaMalloc(size_t bytes);

 /*** SMI ***/
/*******************************************************************************
* bspSmiInitDriver
*
* DESCRIPTION:
*       Init the TWSI interface
*
* INPUTS:
*       None.
*
* OUTPUTS:
*       smiAccessMode - direct/indirect mode
*
* RETURNS:
*       MV_OK               - on success
*       MV_ERROR   - on hardware error
*
* COMMENTS:
*
*******************************************************************************/
int bspSmiInitDriver(enum bspSmiAccessMode  *smiAccessMode);

/*******************************************************************************
* bspSmiReadReg
*
* DESCRIPTION:
*       Reads a register from SMI slave.
*
* INPUTS:
*       devSlvId - Slave Device ID
*		actSmiAddr - actual smi addr to use (relevant for SX PPs)
*       regAddr - Register address to read from.
*
* OUTPUTS:
*       valuePtr     - Data read from register.
*
* RETURNS:
*       MV_OK               - on success
*       MV_ERROR   - on hardware error
*
* COMMENTS:
*
*******************************************************************************/
int bspSmiReadReg(unsigned long devSlvId,
			unsigned long actSmiAddr,
			unsigned long regAddr,
			unsigned long *valuePtr);


/*******************************************************************************
* bspSmiReadRegLionSpecificSet
*
* DESCRIPTION:
*       Set lios specific register mode in bspSmiReadReg
*
* INPUTS:
*       None
*
* OUTPUTS:
*       None
*
* RETURNS:
*       None
*
* COMMENTS:
*
*******************************************************************************/
void bspSmiReadRegLionSpecificSet(void);

/*******************************************************************************
* bspSmiWriteReg
*
* DESCRIPTION:
*       Writes a register to an SMI slave.
*
* INPUTS:
*       devSlvId - Slave Device ID
*		actSmiAddr - actual smi addr to use (relevant for SX PPs)
*       regAddr - Register address to read from.
*       value   - data to be written.
*
* OUTPUTS:
*        None,
*
* RETURNS:
*       MV_OK               - on success
*       MV_ERROR   - on hardware error
*
* COMMENTS:
*
*******************************************************************************/
int bspSmiWriteReg(unsigned long devSlvId,
			unsigned long actSmiAddr,
			unsigned long regAddr,
			unsigned long value);

/*** TWSI ***/
/*******************************************************************************
* bspTwsiInitDriver
*
* DESCRIPTION:
*       Init the TWSI interface
*
* INPUTS:
*       None.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       MV_OK               - on success
*       MV_ERROR   - on hardware error
*
* COMMENTS:
*
*******************************************************************************/
int bspTwsiInitDriver(void);

/*******************************************************************************
* bspTwsiWaitNotBusy
*
* DESCRIPTION:
*       Wait for TWSI interface not BUSY
*
* INPUTS:
*       None.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       MV_OK               - on success
*       MV_ERROR   - on hardware error
*
* COMMENTS:
*
*******************************************************************************/
int bspTwsiWaitNotBusy(void);

/*******************************************************************************
* bspTwsiMasterReadTrans
*
* DESCRIPTION:
*       do TWSI interface Transaction
*
* INPUTS:
*    devId - I2c slave ID
*    pData - Pointer to array of chars (address / data)
*    len   - pData array size (in chars).
*    stop  - Indicates if stop bit is needed.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       MV_OK               - on success
*       MV_ERROR   - on hardware error
*
* COMMENTS:
*
*******************************************************************************/
int bspTwsiMasterReadTrans(unsigned char	devId,
				unsigned char	*pData,
				unsigned char	len,
				int		stop);

/*******************************************************************************
* bspTwsiMasterWriteTrans
*
* DESCRIPTION:
*       do TWSI interface Transaction
*
* INPUTS:
*    devId - I2c slave ID
*    pData - Pointer to array of chars (address / data)
*    len   - pData array size (in chars).
*    stop  - Indicates if stop bit is needed.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       MV_OK               - on success
*       MV_ERROR   - on hardware error
*
* COMMENTS:
*
*******************************************************************************/
int bspTwsiMasterWriteTrans(unsigned char	devId,
				unsigned char	*pData,
				unsigned char	len,
				int		stop);

/*** Ethernet Driver ***/
/*******************************************************************************
* bspEthPortRxInit
*
* DESCRIPTION: Init the ethernet port Rx interface
*
* INPUTS:
*       rxBufPoolSize   - buffer pool size
*       rxBufPool_PTR   - the address of the pool
*       rxBufSize       - the buffer requested size
*       numOfRxBufs_PTR - number of requested buffers, and actual buffers created
*       headerOffset    - packet header offset size
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       MV_OK if successful, or
*       MV_FAIL otherwise.
*
* COMMENTS:
*       None.
*
*******************************************************************************/
int bspEthPortRxInit(unsigned long rxBufPoolSize,
			unsigned char *rxBufPool_PTR,
			unsigned long rxBufSize,
			unsigned long *numOfRxBufs_PTR,
			unsigned long headerOffset,
			unsigned long rxQNum,
			unsigned long rxQbufPercentage[]);

/*******************************************************************************
* bspEthPortTxInit
*
* DESCRIPTION: Init the ethernet port Tx interface
*
* INPUTS:
*       numOfTxBufs - number of requested buffers
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       MV_OK if successful, or
*       MV_FAIL otherwise.
*
* COMMENTS:
*       None.
*
*******************************************************************************/
int bspEthPortTxInit(unsigned long numOfTxBufs);

/*******************************************************************************
* bspEthPortEnable
*
* DESCRIPTION: Enable the ethernet port interface
*
* INPUTS:
*       None.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       MV_OK if successful, or
*       MV_FAIL otherwise.
*
* COMMENTS:
*       None.
*
*******************************************************************************/
int bspEthPortEnable(void);

/*******************************************************************************
* bspEthPortDisable
*
* DESCRIPTION: Disable the ethernet port interface
*
* INPUTS:
*       None.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       MV_OK if successful, or
*       MV_FAIL otherwise.
*
* COMMENTS:
*       None.
*
*******************************************************************************/
int bspEthPortDisable(void);

/*******************************************************************************
* bspEthPortTx
*
* DESCRIPTION:
*       This function transmits a packet.
*
* INPUTS:
*       segmentList     - A list of pointers to the packets segments.
*       segmentLen      - A list of segment length.
*       numOfSegments   - The number of segment in segment list.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       MV_OK if successful, or
*       MV_FAIL otherwise.
*
* COMMENTS:
*       None.
*
*******************************************************************************/
int bspEthPortTx(unsigned char *segmentList[],
		 unsigned long segmentLen[],
		 unsigned long numOfSegments);

/*******************************************************************************
* bspEthInputHookAdd
*
* DESCRIPTION:
*       This bind the user Rx callback
*
* INPUTS:
*       userRxFunc - the user Rx callback function
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       MV_OK if successful, or
*       MV_FAIL otherwise.
*
* COMMENTS:
*       None.
*
*******************************************************************************/
int bspEthInputHookAdd(BSP_RX_CALLBACK_FUNCPTR userRxFunc);

/*******************************************************************************
* bspEthTxCompleteHookAdd
*
* DESCRIPTION:
*       This bind the user Tx complete callback
*
* INPUTS:
*       userTxFunc - the user Tx callback function
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       MV_OK if successful, or
*       MV_FAIL otherwise.
*
* COMMENTS:
*       None.
*
*******************************************************************************/
int bspEthTxCompleteHookAdd(BSP_TX_COMPLETE_CALLBACK_FUNCPTR userTxFunc);

/*******************************************************************************
* bspEthRxPacketFree
*
* DESCRIPTION:
*       This routine frees the received Rx buffer.
*
* INPUTS:
*       segmentList     - A list of pointers to the packets segments.
*       numOfSegments   - The number of segment in segment list.
*       queueNum        - Receive queue number
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       MV_OK if successful, or
*       MV_FAIL otherwise.
*
* COMMENTS:
*       None.
*
*******************************************************************************/
int bspEthRxPacketFree(unsigned char *segmentList[],
			unsigned long numOfSegments,
			unsigned long queueNum);

/*******************************************************************************
* bspIntConnect
*
* DESCRIPTION:
*       Connect a specified C routine to a specified interrupt vector.
*
* INPUTS:
*       vector    - interrupt vector number to attach to
*       routine   - routine to be called
*       parameter - parameter to be passed to routine
*
* OUTPUTS:
*       None
*
* RETURNS:
*       MV_OK   - on success
*       MV_FAIL - on error
*
* COMMENTS:
*       None
*
*******************************************************************************/
int bspIntConnect(unsigned long		vector,
		  void (*routine) (void) ,
		  unsigned long		parameter);

/*******************************************************************************
* extDrvIntEnable
*
* DESCRIPTION:
*       Enable corresponding interrupt bits
*
* INPUTS:
*       intMask - new interrupt bits
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       MV_OK   - on success
*       MV_FAIL - on error
*
* COMMENTS:
*       None
*
*******************************************************************************/
int bspIntEnable(unsigned long intMask);

/*******************************************************************************
* extDrvIntDisable
*
* DESCRIPTION:
*       Disable corresponding interrupt bits.
*
* INPUTS:
*       intMask - new interrupt bits
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       MV_OK   - on success
*       MV_FAIL - on error
*
* COMMENTS:
*       None
*
*******************************************************************************/
int bspIntDisable(unsigned long intMask);

/*******************************************************************************
* bspEthPortTx
*
* DESCRIPTION:
*       This function transmits a packet.
*
* INPUTS:
*       segmentList     - A list of pointers to the packets segments.
*       segmentLen      - A list of segment length.
*       numOfSegments   - The number of segment in segment list.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       MV_OK if successful, or
*       MV_FAIL otherwise.
*
* COMMENTS:
*       None.
*
*******************************************************************************/
int bspEthPortTx(unsigned char *segmentList[],
		 unsigned long segmentLen[],
		 unsigned long numOfSegments);

/*******************************************************************************
* bspEthPortTxQueue
*
* DESCRIPTION:
*       This function is called after a TxEnd event has been received, it passes
*       the needed information to the Tapi part.
*
* INPUTS:
*       segmentList     - A list of pointers to the packets segments.
*       segmentLen      - A list of segment length.
*       numOfSegments   - The number of segment in segment list.
*       txQueue         - The TX queue.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       GT_OK if successful, or
*       GT_FAIL otherwise.
*
* COMMENTS:
*       None.
*
*******************************************************************************/
int bspEthPortTxQueue(unsigned char *segmentList[],
		      unsigned long segmentLen[],
		      unsigned long numOfSegments,
		      unsigned long txQueue);

/*******************************************************************************
* bspEthCpuCodeToQueue
*
* DESCRIPTION:
*       Binds DSA CPU code to RX queue.
*
* INPUTS:
*       dsaCpuCode - DSA CPU code
*       rxQueue    -  rx queue
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       MV_OK if successful, or
*       MV_FAIL otherwise.
*
* COMMENTS:
*       None.
*
*******************************************************************************/
int bspEthCpuCodeToQueue(unsigned long dsaCpuCode, unsigned char rxQueue);

/*******************************************************************************
* bspPciFindDevReset
*
* DESCRIPTION:
*       Reset gPPDevId to make chance to find internal PP again
*
* INPUTS:
*       None
*
* OUTPUTS:
*       None
*
* RETURNS:
*       MV_OK   - on success,
*
* COMMENTS:
*
*******************************************************************************/
int bspPciFindDevReset(void);

/*******************************************************************************
* bspEthInit
*
* DESCRIPTION: Init the ethernet HW and HAL
*
* INPUTS:
*       port   - eth port number
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       MV_OK if successful, or
*       MV_FAIL otherwise.
*
* COMMENTS:
*       None.
*
*******************************************************************************/
int bspEthInit(unsigned char port);

/*******************************************************************************
* bspEthPortTxModeSet
*
* DESCRIPTION: Set the ethernet port tx mode
*
* INPUTS:
*       if txMode == bspEthTxMode_asynch_E -- don't wait for TX done - free packet when interrupt received
*       if txMode == bspEthTxMode_synch_E  -- wait to TX done and free packet immediately
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       MV_OK if successful
*       MV_NOT_SUPPORTED if input is wrong
*       MV_FAIL if bspTxModeSetOn is zero
*
* COMMENTS:
*       None.
*
*******************************************************************************/
int bspEthPortTxModeSet(void *stub);

unsigned long  bspPhys2Virt(unsigned long pAddr);

#endif /* __MV_PSS_API */
