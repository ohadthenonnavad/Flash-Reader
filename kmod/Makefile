# Out-of-tree kernel module to read SPI flash using precompiled offsets from CHIPSEC XMLs
# Usage:
#   make PCH=Q170 KDIR=/lib/modules/$(shell uname -r)/build
# Or multi-platform:
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

PCH ?= Q170
PCHS ?=
CFG_ROOT ?= $(abspath $(CURDIR)/../chipsec/chipsec/cfg)
OUT_HDR_SINGLE := $(CURDIR)/include/gen_pch_spi_offsets.h
OUT_HDR_MULTI  := $(CURDIR)/include/gen_multi_spi_offsets.h
GEN_SINGLE := $(abspath $(CURDIR)/../flash_general_framework/gen_pch_spi_offsets.py)
GEN_MULTI  := $(abspath $(CURDIR)/../flash_general_framework/gen_multi_spi_offsets.py)

KDIR ?= /lib/modules/$(shell uname -r)/build
PWD  := $(shell pwd)

ifeq ($(strip $(PCHS)),)
OUT_HDR := $(OUT_HDR_SINGLE)
else
OUT_HDR := $(OUT_HDR_MULTI)
endif

export MULTI_PLAT := $(if $(PCHS),1)

all: $(OUT_HDR)
	$(MAKE) -C $(KDIR) M=$(PWD) modules

$(OUT_HDR_SINGLE): $(GEN_SINGLE)
	@mkdir -p $(dir $(OUT_HDR_SINGLE))
	@echo "[gen] PCH=$(PCH) CFG_ROOT=$(CFG_ROOT) -> $@"
	python3 $(GEN_SINGLE) --pch $(PCH) --cfg-root $(CFG_ROOT) -o $(OUT_HDR_SINGLE)

$(OUT_HDR_MULTI): $(GEN_MULTI)
	@mkdir -p $(dir $(OUT_HDR_MULTI))
	@echo "[gen] PCHS=$(PCHS) CFG_ROOT=$(CFG_ROOT) -> $@"
	python3 $(GEN_MULTI) --pchs $(PCHS) --cfg-root $(CFG_ROOT) -o $(OUT_HDR_MULTI)

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
	@rm -f $(OUT_HDR_SINGLE) $(OUT_HDR_MULTI)

.PHONY: all clean

endif
