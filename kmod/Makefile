# Out-of-tree kernel module to read SPI flash using precompiled offsets from CHIPSEC XMLs
# Usage:
#   make PCH=Q170 KDIR=/lib/modules/$(shell uname -r)/build
# Multi-platform (also supports single by passing one entry):
#   make PCHS=Q170,AVN KDIR=/lib/modules/$(shell uname -r)/build
# Optional:
#   CFG_ROOT=/path/to/chipsec/cfg (defaults to ../chipsec/chipsec/cfg relative to this Makefile)

ifneq ($(KERNELRELEASE),)
# kbuild context
obj-m := spi_reader.o
ccflags-y += -Wall -Wextra -Wno-unused-parameter

# Propagate multi-platform define from outer make
ifdef MULTI_PLAT
ccflags-y += -DMULTI_PLAT
endif

else
# external make

PCH ?=
PCHS ?= Q170
CFG_ROOT ?= $(abspath $(CURDIR)/../chipsec/chipsec/cfg)
OUT_HDR  := $(CURDIR)/include/gen_multi_spi_offsets.h
GEN_MULTI := $(abspath $(CURDIR)/../flash_general_framework/gen_multi_spi_offsets.py)

KDIR ?= /lib/modules/$(shell uname -r)/build
PWD  := $(shell pwd)

# Back-compat: allow PCH=... and map it to PCHS if PCHS not set
ifeq ($(strip $(PCHS)),)
  ifneq ($(strip $(PCH)),)
    PCHS := $(PCH)
  endif
endif

export MULTI_PLAT := 1

all: $(OUT_HDR)
	$(MAKE) -C $(KDIR) M=$(PWD) modules

$(OUT_HDR): $(GEN_MULTI)
	@mkdir -p $(dir $(OUT_HDR))
	@echo "[gen] PCHS=$(PCHS) CFG_ROOT=$(CFG_ROOT) -> $@"
	python3 $(GEN_MULTI) --pchs $(PCHS) --cfg-root $(CFG_ROOT) -o $(OUT_HDR)

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
	@rm -f $(OUT_HDR)

.PHONY: all clean

endif
