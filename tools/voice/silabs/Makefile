# kernel sources
srctree  := $(shell /bin/pwd)

ifndef KSRC
KSRC  := $(srctree)/../../..
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

ifeq ($(CONFIG_PLAT_ARMADA),y)
CFLAGS += -I$(KSRC)/arch/arm/plat-armada/mv_drivers_lsp/mv_phone/
else
CFLAGS += -I$(KSRC)/arch/arm/plat-feroceon/mv_drivers_lsp/mv_phone/
endif
CFLAGS += -I$(SILABS_SLIC)/inc -I$(SILABS_SLIC)/custom
CFLAGS += -I$(SILABS_SLIC)/silabs_dev -I$(SILABS_SLIC)/config_inc

all: mv_voice_tool-si3217x mv_voice_tool-si3226x

mv_voice_tool-si3217x: mv_voice_tool.c silabs_lib.c pesqdsp.c pesqio.c pesqmain.c  dsp.c pesqmod.c
	$(CC) $(CFLAGS) -DMV_TDM_USE_DCO -DSI3217x -static -o $@ $^ -lm

mv_voice_tool-si3226x: mv_voice_tool.c silabs_lib.c pesqdsp.c pesqio.c pesqmain.c  dsp.c pesqmod.c
	$(CC) $(CFLAGS) -DMV_TDM_USE_DCO -DSI3226x -static -o $@ $^ -lm

clean:
	rm -f mv_voice_tool-si3217x mv_voice_tool-si3226x
