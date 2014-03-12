# Makefile for cesa tools

srctree  := $(shell /bin/pwd)

ifndef KSRC
KSRC  := $(srctree)/../..
endif

CC = $(CROSS_COMPILE)gcc
LD = $(CROSS_COMPILE)ld
CFLAGS = -Wall -Wextra
CFLAGS += -I$(KSRC)/drivers/crypto/mvebu_cesa/

all: mv_cesa_tool
%: %.c
	$(CC) $(CFLAGS) -static mv_cesa_tool.c -o mv_cesa_tool

clean:
	$(RM) mv_cesa_tool
