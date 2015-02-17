# kernel sources
srctree  := $(shell /bin/pwd)

ifndef KSRC
KSRC  := $(srctree)/../../../../
endif

include $(KSRC)/.config

CFLAGS  := $(EXTRA_CFLAGS)

ifdef CONFIG_CPU_BIG_ENDIAN
CFLAGS  += -DCONFIG_CPU_BIG_ENDIAN
endif

ifeq ($(CONFIG_MV_TDM_USE_DCO),y)
 CFLAGS += -DMV_TDM_USE_DCO
endif

CC := $(CROSS_COMPILE)gcc
LD := $(CROSS_COMPILE)ld
AR := $(CROSS_COMPILE)ar

ifeq ($(CONFIG_ARCH_MVEBU),y)
CFLAGS += -I$(KSRC)/drivers/telephony/mvebu_phone/mv_phone/
endif

ifeq ($(CONFIG_PLAT_ARMADA),y)
CFLAGS += -I$(KSRC)/arch/arm/plat-armada/mv_drivers_lsp/mv_phone/
else
CFLAGS += -I$(KSRC)/arch/arm/plat-feroceon/mv_drivers_lsp/mv_phone/
endif
CFLAGS_VE792 = -I$(ZARLINK_VPAPI)/zarlink-ve792/api_lib/includes -I$(ZARLINK_VPAPI)/vp_hal -I$(ZARLINK_VPAPI)/vpapi_dev
CFLAGS_VE8XX = -I$(ZARLINK_VPAPI)/api_lib/includes -I$(ZARLINK_VPAPI)/vp_hal -I$(ZARLINK_VPAPI)/vpapi_dev

all: mv_voice_tool-ve792 mv_voice_tool-ve880 mv_voice_tool-ve886 mv_voice_tool-ve888 mv_voice_tool-ve890

mv_voice_tool-ve792: mv_voice_tool.c vpapi_lib.c profile_le51hr0172.c
	$(CC) $(CFLAGS) $(CFLAGS_VE792) -DZARLINK_SLIC_VE792 -static -o $@ $^

mv_voice_tool-ve880: mv_voice_tool.c vpapi_lib.c profile_88266.c
	$(CC) $(CFLAGS) $(CFLAGS_VE8XX) -DZARLINK_SLIC_VE880 -static -o $@ $^

mv_voice_tool-ve886: mv_voice_tool.c vpapi_lib.c profile_88601.c
	$(CC) $(CFLAGS) $(CFLAGS_VE8XX) -DZARLINK_SLIC_VE886 -static -o $@ $^

mv_voice_tool-ve888: mv_voice_tool.c vpapi_lib.c profile_88801.c
	$(CC) $(CFLAGS) $(CFLAGS_VE8XX) -DZARLINK_SLIC_VE887 -static -o $@ $^

mv_voice_tool-ve890: mv_voice_tool.c vpapi_lib.c profile_89116.c
	$(CC) $(CFLAGS) $(CFLAGS_VE8XX) -DZARLINK_SLIC_VE890 -static -o $@ $^

clean:
	rm -f mv_voice_tool-ve792 mv_voice_tool-ve880 mv_voice_tool-ve886 mv_voice_tool-ve888 mv_voice_tool-ve890
