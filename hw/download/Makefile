#-------------------------------------------------------------------------------
# Copyright (c) 2021 by Future Design Systems.
# All right reserved.
#-------------------------------------------------------------------------------
# VIVADO Bit-Stream Download
#-------------------------------------------------------------------------------
SHELL		= /bin/sh
MAKEFILE	= Makefile
#-------------------------------------------------------------------------------
#
# $ make            --> download through any JTAG
# $ make JTAG_ID=0  --> download through any JTAG
# $ make JTAG_ID=1  --> download through JTAG 1
# $ make JTAG_ID=2  --> download through JTAG 2
#-------------------------------------------------------------------------------
ifeq ("$(MAKECMDGOALS)", "all")
      skip=0
else ifeq ("$(MAKECMDGOALS)", "")
      skip=0
else
      skip=1
endif

ifneq ($(skip), 1)
    ifndef XILINX_VIVADO
           $(error XILINX_VIVADO environment variable not defined)
    else
        ifneq ("$(wildcard $(XILINX_VIVADO)/bin/vivado)","")
            export VIVADO   = $(XILINX_VIVADO)/bin/vivado
        else ifneq ("$(wildcard $(XILINX_VIVADO)/bin/vivado_lab)","")
            export VIVADO   = $(XILINX_VIVADO)/bin/vivado_lab
        else
            $(error "XILINX VIVADO not found")
        endif
    endif
    export VIVADO_VERSION = $(shell vivado -version | sed -n 1p | cut -d" " -f 2 | cut -c 2-7)
    $(info Xilinx Vivado ${VIVADO_VERSION} found)
endif
#-------------------------------------------------------------------------------
export HOST    ?= localhost
export PORT    ?= 3121
export DEVICE  ?= xc7a200t_0
export PART    ?= xc7a200tsbg484-1
export SOURCE   = vivado_down.tcl	
export GUI     ?= 0
export JTAG_ID ?= 0
       # 0 means auto detection
       # 1 or other
export BITFILE ?= ../bitstream/nexys_video.bit
#-------------------------------------------------------------------------------
ifneq ($(skip), 1)
    ifeq ("$(wildcard $(BITFILE))","")
        $(error "bit-file not found");
    endif
endif
#-------------------------------------------------------------------------------
all:
	@if [ "$(GUI)" = "1" ]; then\
		$(VIVADO) -mode gui -source $(SOURCE) || exit 1;\
	else\
		$(VIVADO) -mode batch -source $(SOURCE) || exit 1;\
	fi

#$(eval pnum := $(shell pgrep -u ${USER} -f cs_server | wc -l))
#if [ $(pnum) -gt 2 ]; then\
#	pkill -9 -o cs_server;\
#fi
#pgrep -u ${USER} -f cs_server | xargs kill -9

#-------------------------------------------------------------------------------
clean:
	-@/bin/rm -rf .Xil
	-@/bin/rm -rf $(WORK)
	-@/bin/rm -f  *.html
	-@/bin/rm -f  *.xml
	-@/bin/rm -f  *.jou
	-@/bin/rm -f  *.backup*
	-@/bin/rm -f  planAhead.*
	-@/bin/rm -f  vivado.log
	-@/bin/rm -rf .cache

cleanup clobber: clean
	-@/bin/rm -rf ./*.log
	-@/bin/rm -rf $(MODULE).ucf
	-@/bin/rm -rf $(MODULE).ut
	-@/bin/rm -rf $(MODULE).tcf

cleanupall distclean: cleanup

# in order to accept extra arguments rather than throwing an error
%:
	@:
#-------------------------------------------------------------------------------
