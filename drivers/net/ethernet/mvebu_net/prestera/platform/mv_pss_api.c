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

*******************************************************************************/

/*******************************************************************************
* pssBspApis.c - bsp APIs
*
* DESCRIPTION:
*       API's supported by BSP.
*
* DEPENDENCIES:
*       None.
*
*******************************************************************************/
#include "mvOs.h"
#include "mv_pss_api.h"
#include "mv_prestera.h"

/* Defines */
#define  SMI_WRITE_ADDRESS_MSB_REGISTER	(0x00)
#define  SMI_WRITE_ADDRESS_LSB_REGISTER	(0x01)
#define  SMI_WRITE_DATA_MSB_REGISTER	(0x02)
#define  SMI_WRITE_DATA_LSB_REGISTER	(0x03)

#define  SMI_READ_ADDRESS_MSB_REGISTER	(0x04)
#define  SMI_READ_ADDRESS_LSB_REGISTER	(0x05)
#define  SMI_READ_DATA_MSB_REGISTER	(0x06)
#define  SMI_READ_DATA_LSB_REGISTER	(0x07)

static inline void smiWaitForStatus(unsigned long devSlvId)
{
#ifdef SMI_WAIT_FOR_STATUS_DONE
	unsigned long stat;
	unsigned int timeOut;
	int rc;

	/* wait for write done */
	timeOut = SMI_TIMEOUT_COUNTER;
	do {
		rc = smiReadReg(devSlvId, SMI_STATUS_REGISTER, &stat);
		if (rc != MV_OK)
			return;
		if (--timeOut < 1)
			return;
	} while ((stat & SMI_STATUS_WRITE_DONE) == 0);
#endif
}

#define  SMI_STATUS_REGISTER		(0x1f)

#define SMI_STATUS_WRITE_DONE		(0x02)
#define SMI_STATUS_READ_READY		(0x01)

#define SMI_WAIT_FOR_STATUS_DONE
#define SMI_TIMEOUT_COUNTER		10000

#define STUB_FAIL do { printk(KERN_INFO "stub function %s returning MV_NOT_SUPPORTED\n", __func__);\
		return MV_NOT_SUPPORTED; } while (1)

#define STUB_FAIL_NULL do { printk(KERN_INFO "stub function %s returning MV_NOT_SUPPORTED\n", __func__); \
		return NULL; } while (1)

#define STUB_OK do {   printk(KERN_INFO "stub function %s returning MV_OK\n", __func__); \
		return MV_OK; } while (1)

#define STUB_TBD do { printk(KERN_INFO "stub function TBD %s returning MV_FAIL\n", __func__); \
		return MV_FAIL; } while (1)

static inline struct pci_dev *find_bdf(u32 bus, u32 device, u32 func)
{
	return pci_get_bus_and_slot(bus, PCI_DEVFN(device, func));
}

/* interrupt routine pointer */
void(*bspIsrRoutine)(void) = NULL;
static unsigned long bspIsrParameter = -1;
static unsigned int lionSpecificRegMode;
static unsigned int  ethPhySmiReg;		/* Ethernet unit PHY SMI register offset */

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
int bspResetInit(void)
{
	return MV_OK;
}

/*******************************************************************************
* bspReset
*
* DESCRIPTION:
*       This routine calls to reset of CPU.
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
int bspReset(void)
{
	kernel_restart(NULL);
	return MV_OK;
}

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
			size_t			size)
{
	switch (cacheType) {
	case bspCacheType_InstructionCache_E:
		return MV_BAD_PARAM; /* only data cache supported */

	case bspCacheType_DataCache_E:
		break;

	default:
		return MV_BAD_PARAM;
	}

	/* our area doesn't need cache flush/invalidate	*/
	return MV_OK;
}

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
				size_t		size)
{
	switch (cacheType) {
	case bspCacheType_InstructionCache_E:
		return MV_BAD_PARAM; /* only data cache supported */

	case bspCacheType_DataCache_E:
		break;

	default:
		return MV_BAD_PARAM;
	}

	/* our area doesn't need cache flush/invalidate */
	return MV_OK;
}

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
				unsigned long burstLimit)
{
	STUB_FAIL;
}

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
int bspDmaRead(unsigned long address,
			unsigned long length,
			unsigned long burstLimit,
			unsigned long  *buffer)
{
	STUB_FAIL;
}

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
static unsigned long dma_malloc_base = -1;
static void *dma_area_base = (void *)-1;

void *bspCacheDmaMalloc(size_t bytes)
{
	unsigned long dma_len = bytes;
	void *dma_area;

	if (dma_malloc_base == -1)
		dma_malloc_base = __pa(high_memory);

	request_mem_region(dma_malloc_base, dma_len, "prestera-dma");
	dma_area = (unsigned long *)ioremap_nocache(dma_malloc_base, dma_len);

	if (dma_area_base == (void *)-1)
		dma_area_base = dma_area;

	return dma_area;
}

/*** PCI ***/
/*******************************************************************************
* bspPciConfigWriteReg
*
* DESCRIPTION:
*       This routine write register to the PCI configuration space.
*
* INPUTS:
*       busNo    - PCI bus number.
*       devSel   - the device devSel.
*       funcNo   - function number.
*       regAddr  - Register offset in the configuration space.
*       data     - data to write.
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
int bspPciConfigWriteReg(unsigned long busNo,
					unsigned long devSel,
					unsigned long funcNo,
					unsigned long regAddr,
					unsigned long data)
{
	struct pci_dev *dev;

	dev = find_bdf(busNo, devSel, funcNo);
	if (dev) {
		pci_write_config_dword(dev, regAddr, data);
		pci_dev_put(dev);
		return MV_OK;
	} else
		return MV_FAIL;
}

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
					unsigned long	*data)
{
	struct pci_dev *dev;

	dev = find_bdf(busNo, devSel, funcNo);
	if (dev) {
		pci_read_config_dword(dev, (int)regAddr, (unsigned int *)data);
		pci_dev_put(dev);
		return MV_OK;
	} else
		return MV_FAIL;
}

/*******************************************************************************
* bspPciGetResourceStart
*
* DESCRIPTION:
*       This routine performs pci_resource_start.
*       In INTEL64 this function must be used instead of reading the bar
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
int bspPciGetResourceStart(unsigned long		busNo,
				 unsigned long		devSel,
				 unsigned long		funcNo,
				 unsigned long		barNo,
				 unsigned long long	*resourceStart)
{
	struct pci_dev *dev;

	dev = find_bdf(busNo, devSel, funcNo);
	if (dev) {
		*resourceStart = pci_resource_start(dev, barNo);
		pci_dev_put(dev);
		return MV_OK;
	}
	return MV_FAIL;
}

/*******************************************************************************
* bspPciGetResourceLen
*
* DESCRIPTION:
*       This routine performs pci_resource_len.
*       In INTEL64 this function must be used instead of reading the bar
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
					unsigned long long	*resourceLen)
{
	struct pci_dev *dev;

	dev = find_bdf(busNo, devSel, funcNo);
	if (dev) {
		*resourceLen = pci_resource_len(dev, barNo);
		pci_dev_put(dev);
		return MV_OK;
	}
	return MV_FAIL;
}

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
			unsigned long		*funcNo)
{
	struct pci_dev *dev = NULL;
	int count = 0;

	*busNo = *devSel = *funcNo = 0;

	for_each_pci_dev(dev) {
		if ((vendorId == 0xffff || dev->vendor == vendorId) &&
		    (devId == 0xffff || dev->device == devId) &&
		    /* skip the virtual bridge : 11ab8888 */
		    (!((vendorId == MARVELL_VEN_ID) && (dev->device == 0x8888))) && (count++ == instance)) {
			*busNo = dev->bus->number;
			*devSel = PCI_SLOT(dev->devfn);
			*funcNo = PCI_FUNC(dev->devfn);
			return MV_OK;
		}
	}

	return MV_FAIL;
}

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
int bspPciGetIntVec(enum bspPciInt_PCI_INT pciInt, void **intVec)
{
	struct pci_dev *dev;
	unsigned long b, d, f;
	unsigned int devnum;
	int rc = MV_FAIL;

	devnum = get_founddev();

	/* Support MSYS with Internal PP */
	if ((devnum == 1) &&  (mvDevIdGet() != (-1))) {
		*intVec = (void *)(unsigned long)(IRQ_AURORA_SW_CORE0);
		printk(KERN_INFO "%s int vector 0x%x, internal Int 0x%x\n", __func__, (int)*intVec, pciInt);
		rc = MV_OK;
	} else {
		/* Support MSYS in B2B Internal PP */
	/* iterate to the instance of pp_core_number */
	if (bspPciFindDev(MARVELL_VEN_ID, 0xffff, pciInt, &b, &d, &f) == MV_OK) {
		dev = find_bdf(b, d, f);
		if (dev != NULL) {
			*intVec = (void *)(unsigned long)(dev->irq);
			printk(KERN_INFO "%s int vector 0x%x, pci Int 0x%x\n", __func__, (int)*intVec, pciInt);
			pci_dev_put(dev);
			rc = MV_OK;
		}
	} else if (mvDevIdGet() != (-1)) {
		*intVec = (void *)(unsigned long)(IRQ_AURORA_SW_CORE0);
		printk(KERN_INFO "%s int vector 0x%x, internal Int 0x%x\n", __func__, (int)*intVec, pciInt);
		rc = MV_OK;
		}
	}

	return rc;
}

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
			   unsigned long		*intMask)
{
	return MV_OK;
}

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
				     int enRdCombine)
{
	STUB_FAIL;
}

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
int bspEthInit(unsigned char port)
{
	STUB_OK;
}

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
*       MV_FAIL   - on hardware error
*
* COMMENTS:
*
*******************************************************************************/
int bspSmiInitDriver(enum bspSmiAccessMode  *smiAccessMode)
{
	ethPhySmiReg = ETH_SMI_REG(0/*MV_ETH_SMI_PORT*/);
	STUB_FAIL;
}

/*******************************************************************************
* _ethPhyRegRead
*
*
*******************************************************************************/
static inline int _ethPhyRegRead(unsigned long phyAddr, unsigned long regOffs, unsigned short *data)
{
	unsigned int smiReg;
	unsigned int timeout;

	/* check parameters */
	if ((phyAddr << ETH_PHY_SMI_DEV_ADDR_OFFS) & ~ETH_PHY_SMI_DEV_ADDR_MASK) {
		printk(KERN_ERR "mvEthPhyRegRead: Err. Illegal PHY device address %lx\n", phyAddr);
		return MV_FAIL;
	}
	if ((regOffs <<  ETH_PHY_SMI_REG_ADDR_OFFS) & ~ETH_PHY_SMI_REG_ADDR_MASK) {
		printk(KERN_ERR "mvEthPhyRegRead: Err. Illegal PHY register offset %lx\n", regOffs);
		return MV_FAIL;
	}

	timeout = ETH_PHY_TIMEOUT;
	/* wait till the SMI is not busy*/
	do {
		/* read smi register */
		smiReg = MV_REG_READ(ethPhySmiReg);
		if (timeout-- == 0) {
			printk(KERN_ERR "mvEthPhyRegRead: SMI busy timeout\n");
			return MV_FAIL;
		}
	} while (smiReg & ETH_PHY_SMI_BUSY_MASK);

	/* fill the phy address and regiser offset and read opcode */
	smiReg = (phyAddr <<  ETH_PHY_SMI_DEV_ADDR_OFFS) | (regOffs << ETH_PHY_SMI_REG_ADDR_OFFS)|
			   ETH_PHY_SMI_OPCODE_READ;

	/* write the smi register */
	MV_REG_WRITE(ethPhySmiReg, smiReg);

	timeout = ETH_PHY_TIMEOUT;

	/*wait till readed value is ready */
	do {
		/* read smi register */
		smiReg = MV_REG_READ(ethPhySmiReg);

		if (timeout-- == 0) {
			printk(KERN_ERR "mvEthPhyRegRead: SMI read-valid timeout\n");
			return MV_FAIL;
		}
	} while (!(smiReg & ETH_PHY_SMI_READ_VALID_MASK));

	/* Wait for the data to update in the SMI register */
	for (timeout = 0; timeout < ETH_PHY_TIMEOUT; timeout++)
		;

	*data = (unsigned short)(MV_REG_READ(ethPhySmiReg) & ETH_PHY_SMI_DATA_MASK);

	return MV_OK;
}

/*******************************************************************************
* ethPhyRegWrite
*
*
*******************************************************************************/
static inline int ethPhyRegWrite(unsigned long phyAddr, unsigned long regOffs, unsigned short data)
{
	unsigned int smiReg;
	unsigned int timeout;

	/* check parameters */
	if ((phyAddr <<  ETH_PHY_SMI_DEV_ADDR_OFFS) & ~ETH_PHY_SMI_DEV_ADDR_MASK) {
		printk(KERN_ERR "mvEthPhyRegWrite: Err. Illegal phy address 0x%lx\n", phyAddr);
		return MV_BAD_PARAM;
	}
	if ((regOffs <<  ETH_PHY_SMI_REG_ADDR_OFFS) & ~ETH_PHY_SMI_REG_ADDR_MASK) {
		printk(KERN_ERR "mvEthPhyRegWrite: Err. Illegal register offset 0x%lx\n", regOffs);
		return MV_BAD_PARAM;
	}

	timeout = ETH_PHY_TIMEOUT;

	/* wait till the SMI is not busy*/
	do {
		/* read smi register */
		smiReg = MV_REG_READ(ethPhySmiReg);
		if (timeout-- == 0) {
			printk(KERN_ERR "mvEthPhyRegWrite: SMI busy timeout\n");
		return MV_TIMEOUT;
		}
	} while (smiReg & ETH_PHY_SMI_BUSY_MASK);

	/* fill the phy address and regiser offset and write opcode and data*/
	smiReg = (data << ETH_PHY_SMI_DATA_OFFS);
	smiReg |= (phyAddr <<  ETH_PHY_SMI_DEV_ADDR_OFFS) | (regOffs << ETH_PHY_SMI_REG_ADDR_OFFS);
	smiReg &= ~ETH_PHY_SMI_OPCODE_READ;

	/* write the smi register */
	MV_REG_WRITE(ethPhySmiReg, smiReg);

	return MV_OK;
}

/*******************************************************************************
* smiReadReg
*
*
*******************************************************************************/
int smiReadReg(unsigned long devSlvId, unsigned long regAddr, unsigned long *value)
{
	int ret;
	unsigned short temp1 = 0;

	ret = _ethPhyRegRead(devSlvId, regAddr, &temp1);
	*value = temp1;
	return (MV_OK == ret) ? MV_OK : MV_FAIL;
}

/*******************************************************************************
* smiWriteReg
*
*
*******************************************************************************/
int smiWriteReg(unsigned long devSlvId, unsigned long regAddr, unsigned long value)
{
	/* Perform direct smi write reg */
	int ret;

	ret = ethPhyRegWrite(devSlvId, regAddr, value);
	return (MV_OK == ret) ? MV_OK : MV_FAIL;
}


/*******************************************************************************
* bspSmiReadRegLionSpecificSet
*
*
*******************************************************************************/
void bspSmiReadRegLionSpecificSet(void)
{
	lionSpecificRegMode = 1;
}

/*******************************************************************************
* bspSmiReadReg
*
* DESCRIPTION:
*       Reads a register from SMI slave.
*
* INPUTS:
*       devSlvId - Slave Device ID
*      actSmiAddr - actual smi addr to use (relevant for SX PPs)
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
			unsigned long *valuePtr)
{
	/* Perform indirect smi read reg */
	int		rc;
	unsigned long	msb;
	unsigned long	lsb;

	static int first_time = 1;

	if (first_time) {
		enum bspSmiAccessMode smiAccessMode;
		first_time = 0;
		bspSmiInitDriver(&smiAccessMode);
	}
	/*  fix erroneous devSlvId from cpss */
	if (lionSpecificRegMode)
		MV_FIX_DEV_SLAVE_ID_4_LION(lionSpecificRegMode);

	/* write addr to read */
	msb = regAddr >> 16;
	lsb = regAddr & 0xFFFF;
	rc = smiWriteReg(devSlvId, SMI_READ_ADDRESS_MSB_REGISTER, msb);
	if (rc != MV_OK)
		return rc;

	rc = smiWriteReg(devSlvId, SMI_READ_ADDRESS_LSB_REGISTER, lsb);
	if (rc != MV_OK)
		return rc;

	smiWaitForStatus(devSlvId);

	/* read data */
	rc = smiReadReg(devSlvId, SMI_READ_DATA_MSB_REGISTER, &msb);
	if (rc != MV_OK)
		return rc;

	rc = smiReadReg(devSlvId, SMI_READ_DATA_LSB_REGISTER, &lsb);
	if (rc != MV_OK)
		return rc;

	*valuePtr = ((msb & 0xFFFF) << 16) | (lsb & 0xFFFF);
	return 0;
}

/*******************************************************************************
* bspSmiWriteReg
*
* DESCRIPTION:
*       Writes a register to an SMI slave.
*
* INPUTS:
*       devSlvId - Slave Device ID
*       actSmiAddr - actual smi addr to use (relevant for SX PPs)
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
			 unsigned long value)
{
	/* Perform indirect smi write reg */
	int rc;
	unsigned long msb;
	unsigned long lsb;

	/* write addr to read */
	msb = regAddr >> 16;
	lsb = regAddr & 0xFFFF;
	rc = smiWriteReg(devSlvId, SMI_READ_ADDRESS_MSB_REGISTER, msb);
	if (rc != 0)
		return rc;

	rc = smiWriteReg(devSlvId, SMI_READ_ADDRESS_LSB_REGISTER, lsb);
	if (rc != 0)
		return rc;

	/* write data to write */
	msb = value >> 16;
	lsb = value & 0xFFFF;
	rc = smiWriteReg(devSlvId, SMI_WRITE_DATA_MSB_REGISTER, msb);
	if (rc != MV_OK)
		return rc;

	rc = smiWriteReg(devSlvId, SMI_WRITE_DATA_LSB_REGISTER, lsb);
	if (rc != MV_OK)
		return rc;

	smiWaitForStatus(devSlvId);

	return MV_OK;
}


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
int bspTwsiInitDriver(void)
{
	STUB_FAIL;
}

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
int bspTwsiWaitNotBusy(void)
{
	STUB_FAIL;
}

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
				 int	stop)
{
	STUB_FAIL;
}

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
				  int	stop)
{
	STUB_FAIL;
}

/*******************************************************************************
* bspIsr
*
* DESCRIPTION:
*       This is the ISR reponsible for PP.
*
* INPUTS:
*       irq     - the Interrupt ReQuest number
*       dev_id  - the client data used as argument to the handler
*       regs    - holds a snapshot of the CPU context before interrupt
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       IRQ_HANDLED allways
*
* COMMENTS:
*       None.
*
*******************************************************************************/
static irqreturn_t bspIsr(int			irq,
			  void			*dev_id,
			  struct pt_regs	*regs)
{
	if (bspIsrRoutine != NULL)
		bspIsrRoutine();

	return IRQ_HANDLED;
}

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
int bspIntConnect(unsigned long vector,
			void (*routine) (void) ,
			unsigned long parameter)
{
	int rc;

	bspIsrParameter = parameter;
	bspIsrRoutine  = routine;

	rc = request_irq(vector,
			(irq_handler_t)bspIsr,
			IRQF_DISABLED, "PP_interrupt", (void *)&bspIsrParameter);

	return (0 == rc) ? MV_OK : MV_FAIL;

}

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
int bspIntEnable(unsigned long intMask)
{
	enable_irq(intMask);
	return MV_OK;
}

/*******************************************************************************
* bspIntDisable
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
int bspIntDisable(unsigned long intMask)
{
	disable_irq(intMask);
	return MV_OK;
}

unsigned long bspPhys2Virt(unsigned long pAddr)
{
	STUB_FAIL;
}

/*** Ethernet access MII with the Packet Processor ***/
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
int bspEthPortRxInit(unsigned long	rxBufPoolSize,
			   unsigned char	*rxBufPool_PTR,
			   unsigned long	rxBufSize,
			   unsigned long	*numOfRxBufs_PTR,
			   unsigned long	headerOffset,
			   unsigned long	rxQNum,
			   unsigned long	rxQbufPercentage[])
{
	STUB_FAIL;
}

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
int bspEthPortTxInit(unsigned long numOfTxBufs)
{
	STUB_FAIL;
}

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
int bspEthPortEnable(void)
{
	STUB_FAIL;
}

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
int bspEthPortDisable(void)
{
	STUB_FAIL;
}

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
int bspEthInputHookAdd(BSP_RX_CALLBACK_FUNCPTR userRxFunc)
{
	STUB_FAIL;
}

/*******************************************************************************
* bspEthPortTx
*
* DESCRIPTION:
*       This function is called after a TxEnd event has been received, it passes
*       the needed information to the Tapi part.
*
* INPUTS:
*       segmentsList     - A list of pointers to the packets segments.
*       segmentsLen      - A list of segment length.
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
int bspEthPortTx(unsigned char	*segmentsList[],
		       unsigned long	segmentsLen[],
		       unsigned long	numOfSegments)
{
	STUB_FAIL;
}

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
int bspEthPortTxModeSet(void *stub)
{
	STUB_OK;
}

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
int bspEthPortTxQueue(unsigned char	*segmentList[],
			    unsigned long	segmentLen[],
			    unsigned long	numOfSegments,
			    unsigned long	txQueue)
{
	return bspEthPortTx(segmentList, segmentLen, numOfSegments);
}

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
int bspEthTxCompleteHookAdd(BSP_TX_COMPLETE_CALLBACK_FUNCPTR userTxFunc)
{
	STUB_FAIL;
}

/*******************************************************************************
* bspEthRxPacketFree
*
* DESCRIPTION:
*       This routine frees the received Rx buffer.
*
* INPUTS:
*       segmentsList     - A list of pointers to the packets segments.
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
int bspEthRxPacketFree(unsigned char	*segmentsList[],
			     unsigned long	numOfSegments,
			     unsigned long	queueNum)
{
	STUB_FAIL;
}

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
int bspEthCpuCodeToQueue(unsigned long dsaCpuCode,
			       unsigned char rxQueue)
{
	STUB_OK;
}

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
int bspPciFindDevReset(void)
{
	STUB_OK;
}


/*******************************************************************************
* bspWarmRestart
*
* DESCRIPTION:
*       This routine performs warm restart.
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
int bspWarmRestart(void)
{
	STUB_FAIL;
}


int bspSmiScan(int instance, int noisy)
{
	int found1 = 0;
	int found2 = 0;
	int i;
	unsigned long data;

	/* scan for SMI devices */
	for (i = 0; i < 32; i++) {
		bspSmiReadReg(i, 0, 0x3, &data);
		if (data == 0xffffffff || data == 0xffff)
			continue;

		bspSmiReadReg(i, 0, 0x50, &data);
		if (data != 0x000011ab  && data != 0xab110000)
			continue;

		if (instance == found1++) {
			bspSmiReadReg(i, 0, 0x4c, &data);
			printk(KERN_INFO "Smi Scan found Marvell device at smi_addr 0x%x, reg 0x4c=0x%luX\n",
				   i, data);
			found2 = 1;
			break;
		}
	}

	if (!found2) {
		if (noisy)
			printk(KERN_INFO "Smi scan found no device\n");
		return -1;
	}

	return i;
}

