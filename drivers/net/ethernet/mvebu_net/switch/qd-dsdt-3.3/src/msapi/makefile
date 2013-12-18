# Source files in this directory
TARGET =
CSOURCES	= gtBrgFdb.c gtBrgStp.c gtBrgVlan.c \
			gtEvents.c \
			gtPortCtrl.c gtPortStat.c gtPortStatus.c \
			gtQosMap.c gtPIRL.c	\
			gtPhyCtrl.c gtPhyInt.c \
			gtSysConfig.c gtSysCtrl.c gtVersion.c gtUtils.c \
			gtBrgVtu.c gtPortRmon.c gtSysStatus.c \
			gtPortRateCtrl.c gtPortPav.c gtVct.c gtAdvVct.c gtPCSCtrl.c \
			gtBrgStu.c gtCCPVT.c gtPIRL2.c gtPolicy.c \
			gtTCAM.c \
			gtPriTable.c gtWeight.c gtAVB.c gtPTP.c gtPortLed.c gtMisc.c

ASOURCES	=

# Include common variable definitions
ifeq ($(OS_RUN),VXWORKS)
include $(DSDT_TOOL_DIR)\make.defs
endif
ifeq ($(OS_RUN),WIN32)
include $(DSDT_TOOL_DIR)\makewce.defs
endif
ifeq ($(OS_RUN),LINUX)
include $(DSDT_TOOL_DIR)/makelnx.defs
endif

# Add in extra stuffs
EXTRA_INCLUDE	+=
EXTRA_DEFINE	+=
ADDED_CFLAGS	+=

ifeq ($(DSDT_USE_MAD),TRUE)
#CSOURCES	+= gtPhyCtrl_mad.c gtPhyInt_mad.c gtVct_mad.c gtAdvVct_mad.c
ADDED_CFLAGS	+= -DGT_USE_MAD
endif

# Include common build rules
ifeq ($(OS_RUN),VXWORKS)
include $(DSDT_TOOL_DIR)\make.rules
endif
ifeq ($(OS_RUN),WIN32)
include $(DSDT_TOOL_DIR)\makewce.rules
endif
ifeq ($(OS_RUN),LINUX)
include $(DSDT_TOOL_DIR)/makelnx.rules
endif

# end of file
