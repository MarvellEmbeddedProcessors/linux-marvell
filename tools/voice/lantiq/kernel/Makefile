# kernel sources
srctree  := $(shell /bin/pwd)

ifndef KSRC
KSRC  := $(srctree)/../../../..
endif

include $(KSRC)/.config

CFLAGS := $(EXTRA_CFLAGS)
ifeq ($(CONFIG_PLAT_ARMADA),y)
CFLAGS += -I$(KSRC)/arch/arm/plat-armada/mv_drivers_lsp/mv_phone/
else
CFLAGS += -I$(KSRC)/arch/arm/plat-feroceon/mv_drivers_lsp/mv_phone/
endif
ifdef CONFIG_CPU_BIG_ENDIAN
CFLAGS  += -DCONFIG_CPU_BIG_ENDIAN
endif

ifeq ($(CONFIG_MV_TDM_USE_DCO),y)
 CFLAGS += -DMV_TDM_USE_DCO
endif

ifeq ($(CONFIG_ARCH_MVEBU),y)
CFLAGS += -I$(KSRC)/drivers/telephony/mvebu_phone/mv_phone/
endif

CC := $(CROSS_COMPILE)gcc
LD := $(CROSS_COMPILE)ld
AR := $(CROSS_COMPILE)ar

LIBTAPI := libtapi.o libtapi-fw.o
LIBS := libtapi.a
TOOL := mv_voice_tool

all:	$(TOOL)

$(TOOL): $(TOOL).c $(LIBS)
	$(CC) $(CFLAGS) -static -o mv_voice_tool $^

libtapi.o: libtapi.c
	$(CC) $(CFLAGS) -static -c -o $@ $<

libtapi.a: $(LIBTAPI)
	$(AR) rcs $@ $^

clean:
	rm -f *.o *.a mv_voice_tool
	@for subdir in $(SUBDIRS); do \
                (cd $$subdir && $(MAKE) $@) || exit 1; \
        done
