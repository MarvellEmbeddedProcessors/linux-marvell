
                    Switch driver in DSDT Release version 3.3
                   ============================================

Table of Content:
-----------------
1) Release History
2) Source Code Organization
3) General Introduction
4) HOW TO - Build qdDrv.o for vxWorks
5) HOW TO - Build qdDrv.lib for WinCE
6) HOW TO - Build qdDrv.o for Linux
7) HOW TO - Use phy driver (madDrv) in qdDrv.o
8) HOW TO - select features in msApiSelect.h
9) HOW TO - Build kinds of driver
10) Changes - DSDT 3.3 change list


1) Release History
------------------
DSDT_3.3/switch - Feb.1. 2013.
          0. Added and completed to support 88E6320 family.
		  1. Fixed errors.

DSDT_3.2/switch - Mar.1. 2012.
          0. Added to support 88E6320 family.
          1. Added to support full functions of 88E6352 family, and SpinnakerAv.
          2. Added TCAM function of 88E6320.
          3. Fixed page phy access problem of 88E6352.

DSDT_3.1/switch - Mar.31. 2011.
          0. Update phy.
          1. Added new PTP and TAI feature of 88E6352.
          2. Added msApiSelect.h to allow Customer to select DSDT functions.
          3. Moved port mapping functions into Dev structure, which is selected by Customer.
          4. Fixed bugs of Melody.
          5. Added 88E6352 family.
          6. Added semaphore for Remote management access.
          7. Added port mapping functions into system config.
          8. Fixed 6152/6155 port vector.
          9. Fixed warning problems for long to short.
         10. Fixed API gatuGetViolation DB Number error.
DSDT_3.0/switch - May 26. 2010.
          0. Based on DSDT2.8a.
          1. Support Marvell F2R (Remote management) function.
          2. Added device group 1 to extend device number.
          3. Added to support 88EC0xx.
          4. Added to use Marvell Phy driver (madDrv).
          5. Changed name form DSDT* to DSDT*/switch. The name QD is kept to indicate DSDT switch driver.
          6. Added to support 88EC0XX and 88E6250 group.
DSDT2.8a.zip - Jan. 2009. Fixed problem.
              1. Deleted to support 6095 family for Ingress Rate Limit withFlow control.
              2. Deleted unused definition in GT_QPRI_TBL_ENTRY.
DSDT2.8.zip - Nov. 2008. added support for 88E6351 family (88E6351, 88E6175, 88E6124)
          1. New APIs are added to support new devices.

DSDT2.7a.zip - March. 2008.
          1. Fixed known bugs.
          2. Enhanced some of the APIs.

DSDT2.7.zip - May. 2007. added support for 88E6165 family (88E6123, 88E6125, 88E6140, 88E6161)
          1. New APIs are added to support new devices.
          2. Bug fix
            GT_PIRL2_DATA structure includes GT_PIRL_COUNT_MODE enum type,
            which should be GT_PIRL2_COUNT_MODE.
            88E6083 support Static Management frame.
            gprtSetForwardUnknown deals with wrong bit.
          3. Removed Diag program that make user confused with missing files.

DSDT2.6b.zip - Jan. 2007.
          1. Bug Fixes
          2. PIRL Rate Limiting Parameter update

DSDT2.6a.zip - Nov. 2006. added support for 88E6045.

DSDT2.6.zip - Jul. 2006. added support for 88E6097, 88E6096, 88E6046, 88E6047, and 88E6085.
          1. New APIs are added to support new devices.
          2. Bug fixes those were in 2.6 preliminary release.

DSDT2.6pre.zip - Apr. 2006. added preliminary support for 88E6097.
          1. New features are added.
          2. Some parameters in the existing APIs are modified to support extended feature.

DSDT2.5b.zip - Jan. 2006.
          1. added gtDelay function after disabling PPU
              Since delay function is system/OS dependent, it is required that DSDT user
              fill up the gtDelay function based its platform parameters.
              gtDelay function is located in src\msApi\gtUtils.c
          2. Unused GT_STATUS definitions are removed.

DSDT2.5a.zip - Jan. 2006, added support for 88E6122 and 88E6121 and new feature that bypasses
          initial device setup, and bug fixes in the previous release.
          1. Bypass initial configuration when loading driver.
          2. Bug fixes:
              1) synchronization issues.
              2) port vector of 0xFF was treated as an invalid vector.

DSDT2.5.zip - Nov. 2005, added support for 88E6065, 88E6035, 88E6055, 88E6061, and 88E6031,
          and bug fixes in the previous release.
          1. New APIs are added to support new devices.
          2. Bug fixes:
              1) gfdbGetAtuEntryNext API returns GT_NO_SUCH when Entry's MAC is Broadcast address.
              2) entryState in GT_ATU_ENTRY follows the definition.
              3) gsysSetTrunkMaskTable API does not overwrite HashTrunk value anymore.
              4) 10/100 FastEthernet Phy Reset occurs along with Speed, Duplex modification.


DSDT2.4a.zip - Oct. 2005, added support for 88E6131 and a bug fix.
          1. gprtPortPowerDown(gtPhyCtrl.c) didn't work due to reset - reset is not called after PowerDown bit change.

DSDT2.4.zip - Aug. 2005, bug fixes and modifications
          1. gprtSetPktGenEnable(gtPhyCtrl.c) didn't work with Serdes Device - resolved.
          2. gprtSetPortAutoMode(gtPhyCtrl.c) dropped 1000Mbps Half duplex mode - resolved.
          3. gprtGetPhyLinkStatus(gtPhyCtrl.c) returned LinkOn when there is no phy connected - resolved.
          4. gprtSetPortDuplexMode(gtPhyCtrl.c) reset 1000M Speed - resolved.
          5. gfdbSetAtuSize(gtBrgFdb.c), now, returns GT_NOT_SUPPORT if ATU size of the device
             is not configurable.
          6. gprtSetPortLoopback(gtPhyCtrl.c) treats Fast Ethernet Phy and Gigabit Ethernet Phy
               differently.
          7. GT_GET_SERDES_PORT, now, does the error checking.
          8. IS_CONFIGURABLE_PHY reads PHY ID and returns the ID

DSDT2.4pre.zip - July. 2005, added support for 88E6108
          1. New features are added.
          2. Arguments in gprtSetPause and gprtSetPortSpeed are modified to support
             1000M Phys.
          3. Driver functions are added to support Marvell Alask Phys and to be
             expanded easily for the future Phys.

DSDT2.3c.zip - May. 2005,
          1. New features in Rev1 or Rev2 of 88E6095 are added
          2. gfdbGetAgingTimeout, and gfdbGetLearnEnable are added
          3. Bug fixes in grcSetEgressRate and grcSetPri0Rate
          4. Resetting TrunkID, when gprtSetTrunkPort is called to disable Trunk, is applied
             only to Rev0 of 88E6095 and 88E6185

DSDT2.3b.zip - Mar. 2005,
          1. gstpSetMode function does not modify Port State any more, since STP module
             sets the port state. gstpSetMode sets the switch so that it can receive
              BPDU packets.
          2. gtLoadDriver clears Rsvd2Cpu and Rsvd2CpuEn bits.
          3. TrunkID will be reset when gprtSetTrunkPort is called to disable Trunk.
          4. "Check PPU Status in order to verify PPU disabled" is applied to gtVct.c

DSDT2.3a.zip - Jan. 2005, added support for 88E6152, 88E6155, 88E6182, and 88E6092
          devices, removed non-existing devices, and bug fix in 2.3 release.
          Fix :
          Check PPU Status in order to verify PPU disabled.

DSDT2.3.zip - Nov. 2004, support for 88E6185 and bug fixes in 2.3 preliminary release.
          Fixes :
          1) Provide some delay after disabling PPU.
          2) VCT runs after disabling PPU.

DSDT2.3pre.zip - Nov. 2004, added preliminary support for 88E6185.

DSDT2.2a.zip - Nov. 2004, added semaphore support for MII Access with multi address mode.

DSDT2.2.zip - Oct. 2004, support for 88E6095 and bug fixes in 2.2 preliminary release.

DSDT2.2pre.zip - Sep. 2004, added preliminary support for 88E6095 and work-around for VCT
          based on VCT Application Note.

DSDT2.1a.zip - Apr. 2004, support 88E6093 and bug fixes.
          Device Driver Package name has been changed from QDDriver to DSDT(Datacom
          Same Driver Technology).
          Bug Fixes :
          1) DBNum was not correctly handled while getting entry from VTU Table.
          2) Member Tag in VTU entry was not defined correctly for 88E6183 family.
          3) Correction of 88E6183 RMON Counter Structure and Enum.
          4) ATU Interrupt Handling routine

qdDriver2.1-pre.zip - Apr. 2004, added preliminary support for 88E6093 and bug fixes.
          Bug Fixes :
          1) DBNum was not incorrectly handled while getting entry from
          VTU Table.
          2) Member Tag in VTU entry was not defined correctly for 88E6183 family.

qdDriver2.0a.zip - Dec. 2003, provides functions, which can read/write
          Switch Port Registers and Switch Global Registers:
          gprtGetSwitchReg,
          gprtSetSwitchReg,
          gprtGetGlobalReg, and
          gprtSetGlobalReg

qdDriver2.0.zip - July. 2003, supports Multi Address Mode for upcoming device.
          AUTO_SCAN_MODE, MANUAL_MODE, and MULTI_ADDR_MODE are added
          to find a QD Family device.
          Supports Sapphire (10 port Gigabit switch).

qdDriver1.4a.zip - Apr. 2003, bug fixes.
          Bug fixes on portVec in GT_ATU_ENTRY structure, which supported only
          total of 8 ports (defined as GT_U8). It's now defined as GT_U32.
          utils.c and testApi.c in Diag directory also modified to support
          the switch with more than 8 ports.

qdDriver1.4.zip - Apr. 2003, added support for Ocatne (6083).
          Removed NO-OPs, which created when DBG_PRINT is undefined.
          Bug fixes on gprtSetIGMPSnoop and gprtGetIGMPSnoop functions,
          and GT_PRI0_RATE enum type.

qdDriver1.3h.zip - Feb. 2003, added support for Extended Cable Status,
          such as Impediance mismatch, Pair Skew, Pair Swap and Pair Polarity.
          Bug fixes on FFox-EG and FFox-XP device ID.

qdDriver1.3g.zip - Dec. 2002, added preliminary support for Octane (6083)

qdDriver1.3.zip - Oct. 2002, added support for ClipperShip (6063)
          This driver works with all released devices, including
          6051, 6052, 6021, and 6063

qdDriver1.2.zip - Aug. 2002, added support for FullSail (6021)

qdDriver1.1.zip - June, 2002 OS independent QuarterDeck Driver Release
          Based on 1.0 release, but removed OS dependency. The driver
          is designed to work with any OS without code changes.

qdDriver1.0.zip - Feb. 2002, Initial QuaterDeck Driver Release
          Based on vxWorks OS, support 6051/6052


2) Source Code Organization
--------------------------
    2.1) src
        Switch Driver Suite Source Code.

    2.2) Include directory
        Switch Driver Suite Header files and Prototype files

    2.3) Library
        Object files for Switch driver Suite

    2.4) Sample
        Sample Code that shows how to use MSAPIs, e.g., init Switch driver, setup VLAN for Home Gateway, etc.

    2.5) Tools
	Building related files

    2.6) Phy
        The phy part of switch has alternative individual driver, Marvell Phy Driver (madDrv).
        The code is in DSDT_X.Y/phy.
        See DSDT_X.Y/phy/README.txt for more detail.


    * The Switch Driver Suite Source Code is OS independent, and fully supported by Marvell.
    * The Sample Codes are tested under vxworks and Linux, and is provided for reference only.


3) General Introduction
-----------------------

The Switch driver suite is standalone program, which is independent of both OS and Platform.
As such, applications of MSAPIs need to register platform specific functions.
This is done by calling qdLoadDriver function. This function returns a pointer (*dev),
which contains the device and platform information. It will then be used for each MSAPI call.

msApiInit.c file in Diag directory and Sample\Initialization directory demonstrate
how you can register those functions.

msApiInit.c
    qdStart is the main function to initialize Switch Driver and does the
    followings:
    a) register the platform specific functions.
       1.1 and 1.2 below are required.
       1.3 to 1.4 is for F2R(Remote management), it is selected
          by flag GT_RMGMT_ACCESS in IncludemsApiDefs.h.
       1.5 to 1.8 is optional.
        1.1) readMii - BSP specific MII read function
        1.2) writeMii - BSP specific MII write function
        1.3) hwAccessMod - BSP supported function mode
        1.4) hwAccess - BSP specific hardware access function
        1.5) semCreate - OS specific semaphore create function.
        1.6) semDelete - OS specific semaphore delete function.
        1.7) semTake - OS specific semaphore take function.
        1.8) semGive - OS specific semaphore give function.

        Notes) The given example will use DB-88E6218 BSP as an example.

    b) Initialize BSP provided routine (if required).

    c) Calls qdLoadDriver routine.
        1.1) Input (GT_SYS_CONFIG) - CPU Port Number (Board Specific) and Port Mode
        (either 1 for Forwarding mode or 0 for Hardware default mode)
        1.2) Output (GT_QD_DEV) - Contains all device (Switch) and platform specific info.
             It will be used for all API calls.

    d) Calls sysEnable (for future use.)


4) HOW TO - Build qdDrv.o for vxWorks
-------------------------------------

1. Extract the given ZIP file into c:\DSDT_3.x\switch directory
   You may change the directory name to your choice, and change the environment variable below accordingly.
2. Edit setenv.bat file in c:\DSDT_3.x\switch\tools
3. Modify the following variables according to your setup.
set DSDT_USER_BASE=C:\DSDT_3.x\switch
set DSDT_PROJ_NAME=qdDrv
set DSDT_USE_MAD=FALSE or TRUE
set WIND_BASE=C:\Tornado
set TARGETCPU=MIPS        ;ARM for ARM Cpu
set WIND_HOST_TYPE=x86-win32
4. run "setenv"
5. Change directory to c:\DSDT_3.x\switch\src
6. run "make"
7. qdDrv.o and qdDrv.map will be created in c:\DSDT_3.x\switch\Library.


5) HOW TO - Build qdDrv.lib for WinCE
-------------------------------------

1. Extract the given ZIP file into c:\DSDT_3.x\switch directory(directory can be changed)
2. Edit setenv.bat file in c:\DSDT_3.x\switch\tools
3. Modify the following variables according to your setup.
set DSDT_USER_BASE=C:\DSDT_3.x\switch
set DSDT_PROJ_NAME=qdDrv
set DSDT_USE_MAD=FALSE or TRUE
set TARGETCPU=x86        ;MIPSIV for MIPS IV
set WCEROOT=C:\WINCE400

4. run "setenv WINCE"
5. Change directory to c:\DSDT_3.x\switch\src
6. run "make"
7. qdDrv.lib will be created in c:\DSDT_3.x\switch\Library.


6) HOW TO - Build qdDrv.o for Linux
-----------------------------------

1. Extract the given ZIP file into $HOME/DSDT_3.x/switch directory(directory can be changed)
 `   in Linux system (verified with Fedore 8.x)
2. Edit setenv file in $HOME/DSDT_3.x/switch/tools
3. Modify the following variables according to your setup.
    declare -x DSDT_USER_BASE=$HOME/DSDT_3.x/switch
    declare -x DSDT_PROJ_NAME=qdDrv
    declare -x DSDT_USE_MAD=FALSE or TRUE
4. run "source setenv"
5. Change directory to $HOME/DSDT_3.x/switch/src
6. run "make"
7. qdDrv.o and qdDrv.map will be created in $HOME/DSDT_3.x/switch/Library.

7) HOW TO - Use Phy driver in qdDrv.o
-------------------------------------

 The Phy driver(DSDT/phy or MAD) supports Marvell Phy products. It includes the phys in switch chips.
 From DSDT 3.0A, the Switch driver added functions to merge MAD driver
 to replace original Phy regarded functions. This is an option.
 There is no any difference from API point of view, if it uses QD over MAD APIs.
 For old chips, it is selectable to use ether original Phy API function or call MAD functions. To use latest Phy functions, it should us QD over MAD. The selection is to define <GT_USE_MAD> in <msApiSelect.h>.

8) HOW TO - select features in msApiSelect.h
---------------------------------------------

 Customer can ignore the selections. The default selections are <undef> for all.

 From DSDT_3.0D, DSDT adds file <msApiSelect.h> in DSDT*.*/Include.
 The file allows customer to select self features.
 Now selections are:
    GT_PORT_MAP_IN_DEV: To use port mapping functions in Dev configuration.
    GT_RMGMT_ACCESS: DSDT uses RMGMT to replace SMI.
    GT_USE_MAD: the phy APIs of switch use Mad driver(DSDT/phy).

 If customer wants to use the selection,
   1. Adds a Micro definition <CHECK_API_SELECT> in customers build environment.
     For example, in <makefile>, adds <CFLAGS += -DCHECK_API_SELECT>.
   2. In file <Include/msApiSelect>, set correct selections.
   3. Re-build DSDT/switch.


9) HOW TO - build driver
-----------------------------------

 Change directory to $HOME/DSDT_3.x/
 <make switch>: to build switch driver image only.
 <make phy>: to build phy driver image only.
 <make>: to build switch and phy driver images, and switch driver does not use MAD APIs.
 <make DSDT_USE_MAD=TRUE>: to build switch and phy driver images, and switch driver uses MAD APIs.

10) CHANGES
-----------
1. Completed to supports Marvell 88E6320.
2. Fixed Phy access problem.
