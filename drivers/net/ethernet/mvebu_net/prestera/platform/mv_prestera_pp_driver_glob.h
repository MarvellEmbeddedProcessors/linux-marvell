/*******************************************************************************
* mv_prestera_pp_driver_glob.h
*
* DESCRIPTION:
*       This file includes the declaration of the struct we want to send to kernel mode,
*       from user mode.
*
* DEPENDENCIES:
*       None.
*
* COMMENTS:
*   Please note: this file is shared for:
*       axp_lsp_3.4.69
*       msys_lsp_3_4
*       msys_lsp_2_6_32
*
*******************************************************************************/
#ifndef __MV_PRESTERA_PP_DRIVER_GLOB__
#define __MV_PRESTERA_PP_DRIVER_GLOB__

#ifndef mv_phys_addr_t
# define mv_phys_addr_t      uintptr_t
# define mv_kmod_uintptr_t   uintptr_t
# define mv_kmod_size_t      size_t
#endif

/*
 * enum mvPpDrvDriverType_ENT
 *
 * Description: Kernel-mode driver registers select
 *
 * Enumerations:
 *    mvPpDrvDriverType_Pci_E     - 4 region address completion (ADDRCOMPL==0)
 *    mvPpDrvDriverType_PciHalf_E - 2 region address completion (ADDRCOMPL==0)
 *                                  32M mapped only
 *    mvPpDrvDriverType_PexMbus_E - 8 region address completion
 *                                  Bobcat2 & Lion3 only now
 */
enum mvPpDrvDriverType_ENT {
	mvPpDrvDriverType_Pci_E,
	mvPpDrvDriverType_PciHalf_E,
	mvPpDrvDriverType_PexMbus_E
};

/*
 * typedef: struct mvPpDrvDriverOpen_STC
 *
 * Description:
 *
 * Kernel-mode driver init data
 *
 * Fields:
 *   busNo      - PCI bus No
 *   devSel     - PCI device No
 *   funcno     - PCI device function No
 *   type       - driver type (PCI/PexMbus,etc)
 *   id         - Driver Id
 *
 * Comments:
 *
 */
struct mvPpDrvDriverOpen_STC {
	uint32_t                   busNo;
	uint32_t                   devSel;
	uint32_t                   funcNo;
	enum mvPpDrvDriverType_ENT type;
	uint32_t                   id;
};



/*
 * enum mvPpDrvDriverIoOps_ENT
 *
 * Description: Kernel-mode driver operations
 *
 * Enumerations:
 *    mvPpDrvDriverIoOps_Reset_E        - reset driver
 *    mvPpDrvDriverIoOps_Destroy_E      - destroy
 *    mvPpDrvDriverIoOps_PpRegRead_E    - read PP register
 *    mvPpDrvDriverIoOps_PpRegWrite_E   - write PP register
 *    mvPpDrvDriverIoOps_PciRegRead_E   - read PCI config register
 *    mvPpDrvDriverIoOps_PciRegWrite_E  - write PCI config register
 *    mvPpDrvDriverIoOps_DfxRegRead_E   - read DFX register
 *    mvPpDrvDriverIoOps_DfxRegWrite_E  - write DFX register
 *    mvPpDrvDriverIoOps_RamRead_E      - read PP ram (from pp registers
 *                                        address space)
 *    mvPpDrvDriverIoOps_RamWrite_E     - write PP ram (to pp registers
 *                                        address space)
 *
 */
enum mvPpDrvDriverIoOps_ENT {
	mvPpDrvDriverIoOps_Reset_E,
	mvPpDrvDriverIoOps_Destroy_E,
	mvPpDrvDriverIoOps_PpRegRead_E,
	mvPpDrvDriverIoOps_PpRegWrite_E,
	mvPpDrvDriverIoOps_PciRegRead_E,
	mvPpDrvDriverIoOps_PciRegWrite_E,
	mvPpDrvDriverIoOps_DfxRegRead_E,
	mvPpDrvDriverIoOps_DfxRegWrite_E,
	mvPpDrvDriverIoOps_RamRead_E,
	mvPpDrvDriverIoOps_RamWrite_E,
};

/*
 * typedef: struct mvPpDrvDriverIoStc
 *
 * Description:
 *
 * Kernel-mode driver I/O data
 *
 * Fields:
 *   id         - Driver Id
 *   op         - Operation type
 *   regAddr    - Register address
 *   length     - number of registers to read/write
 *                byteswap flag for diag mode
 *   dataPtr    - pointer to data for read/write
 *
 * Comments:
 *
 */
struct mvPpDrvDriverIo_STC {
	uint32_t                    id;
	enum mvPpDrvDriverIoOps_ENT op;
	uint32_t                    regAddr;
	uint32_t                    length;
	mv_kmod_uintptr_t           dataPtr;
};


/************************ IOCTLs ****************************/
#define PRESTERA_PP_DRIVER_IOC_MAGIC 'd'
#define PRESTERA_PP_DRIVER_OPEN     _IOWR(PRESTERA_PP_DRIVER_IOC_MAGIC, 0, struct mvPpDrvDriverOpen_STC)
#define PRESTERA_PP_DRIVER_IO       _IOWR(PRESTERA_PP_DRIVER_IOC_MAGIC, 1, struct mvPpDrvDriverIo_STC)

#ifndef __KERNEL__
# ifndef prestera_ctl
#   ifdef PRESTERA_SYSCALLS
#     include <unistd.h>
#     include <sys/syscall.h>
#     define   __NR_prestera_ctl   __NR_setxattr
#     define prestera_ctl(cmd, arg)    syscall(__NR_prestera_ctl, (cmd), (arg))
#   else /* !defined(PRESTERA_SYSCALLS) */
#     include <sys/ioctl.h>
extern GT_32 gtPpFd;
#     define prestera_ctl(cmd, arg)   ioctl(gtPpFd, cmd, arg)
#   endif /* !defined(PRESTERA_SYSCALLS) */
# endif /* !defined(prestera_ctl) */
#endif

#endif /* __MV_PRESTERA_PP_DRIVER_GLOB__ */
