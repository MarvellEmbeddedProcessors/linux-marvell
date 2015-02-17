# kernel sources
srctree  := $(shell /bin/pwd)

ifndef KSRC
KSRC  := $(srctree)/../..
endif

include $(KSRC)/.config

CFLAGS  := $(EXTRA_CFLAGS)

ifdef CONFIG_CPU_BIG_ENDIAN
CFLAGS  += -DCONFIG_CPU_BIG_ENDIAN
endif

CC := $(CROSS_COMPILE)gcc
LD := $(CROSS_COMPILE)ld
AR := $(CROSS_COMPILE)ar

#CFLAGS += -I$(KSRC)/tools/voice/

ifeq ($(CONFIG_ZARLINK_SLIC_SUPPORT),y)
TARGETS += vpapi_lib.o
LIBS := libvpapi.a
TOOL := mv_voice_tool.c
CFLAGS += -I$(KSRC)/arch/arm/plat-armada/mv_hal/voiceband/slic/zarlink/arch_marvell/
CFLAGS += -I$(KSRC)/arch/arm/plat-armada/mv_drivers_lsp/mv_phone/
ifeq ($(CONFIG_ZARLINK_SLIC_VE792),y)
 CFLAGS += -I$(KSRC)/arch/arm/plat-armada/mv_hal/voiceband/slic/zarlink/vp792_api_lib/includes/
 CFLAGS += -DZARLINK_SLIC_VE792
 PROFILE = profile_79238.c
endif
ifeq ($(CONFIG_ZARLINK_SLIC_VE880),y)
 CFLAGS += -I$(KSRC)/arch/arm/plat-armada/mv_hal/voiceband/slic/zarlink/api_lib/includes/
 CFLAGS += -DZARLINK_SLIC_VE880
 PROFILE = profile_88266.c
endif

CFLAGS += -I$(KSRC)/arch/arm/plat-armada/mv_drivers_lsp/mv_phone/test/

all:
	$(CC) $(CFLAGS) -static -c vpapi_lib.c -o $(TARGETS)
	$(AR) rcs $(LIBS) $(TARGETS)
	$(CC) $(CFLAGS) -static $(PROFILE) $(TOOL) -L. $(LIBS) -o mv_voice_tool

mv_voice_tool: $(PROFILE) $(TOOL)
	$(CC) $(CFLAGS) -static $(PROFILE) $(TOOL) -L. $(LIBS) -o mv_voice_tool

clean:
	rm -f *.o *.a mv_voice_tool
	@for subdir in $(SUBDIRS); do \
                (cd $$subdir && $(MAKE) $@) || exit 1; \
        done

endif #CONFIG_ZARLINK_SLIC_SUPPORT

#ifeq ($(CONFIG_SILABS_SLIC_SUPPORT),y)
#TARGETS +=
#endif
