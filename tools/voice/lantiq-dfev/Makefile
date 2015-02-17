CC := $(CROSS_COMPILE)gcc
LD := $(CROSS_COMPILE)ld
AR := $(CROSS_COMPILE)ar

LIBTAPI := libtapi.o libtapi-fw.o
LIBS := libtapi.a
TOOL := mv_voice_tool

CFLAGS += $(EXTRA_CFLAGS)

all:	$(TOOL)

$(TOOL): $(TOOL).c $(LIBS)
	$(CC) $(CFLAGS) -static -o mv_voice_tool $^

libtapi.o: libtapi.c
	$(CC) $(CFLAGS) -static -c -o $@ $<

libtapi.a: $(LIBTAPI)
	$(AR) rcs $@ $^

clean:
	rm -f *.o *.a mv_voice_tool
