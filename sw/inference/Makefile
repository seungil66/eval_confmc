SHELL=/bin/bash
ARCH=$(shell uname)

ARCH=$(shell uname -s)
ifeq ($(findstring MINGW,$(ARCH)),MINGW)
      PLATFORM:=mingw
else ifeq ($(findstring CYGWIN,$(ARCH)),CYGWIN)
      PLATFORM:=cygwin
else ifeq ($(findstring Linux,$(ARCH)),Linux)
      PLATFORM:=linux
endif
KERN:=$(PLATFORM)
MACHINE:=$(shell uname -m)
MACH:=$(MACHINE)
PROCESSOR:=$(uname -p)

ifeq ($(MAKECMDGOALS),$(findstring $(MAKECMDGOALS), "" "all" "run" "run_help"))
    ifndef CONFMC_HOME
        $(error "CONFMC_HOME is not defined.")
    endif
endif

DIR_SRC    := src
DIR_UTILS  := library
DIR_INC    := $(DIR_SRC) $(DIR_UTILS)
DIR_OBJ    := obj
DUMMY      := $(shell [ -d $(DIR_OBJ) ] || mkdir $(DIR_OBJ) )

PROG       := infer
C_SRCS     := main.c load_params.c inference.c
TARGET_LIB := utils

ifeq ($(MAKECMDGOALS),$(findstring $(MAKECMDGOALS), "" "all" "run" "run_help"))
    ifeq ("$(wildcard $(DIR_UTILS)/lib$(TARGET_LIB).a)", "")
        $(error "$(DIR_UTILS)/lib$(TARGET_LIB).a not fould.")
    endif
endif

C_OBJS     := $(addprefix $(DIR_OBJ)/,$(patsubst %.c,%.o,$(notdir $(C_SRCS))))
INCS       := $(addprefix -I,$(DIR_INC))

DEFINES     = -DRIGOR -DDEBUG
CFLAGS     := -O3 -m64 -mcmodel=large -c $(INCS) $(DEFINES)
LDFLAGS    := -O3 -m64
LDFLAGS    += -L$(DIR_UTILS) -l$(TARGET_LIB)
LDFLAGS    += -L$(CONFMC_HOME)/lib/$(PLATFORM)_$(MACHINE) -lconapi
LDFLAGS    += -lusb-1.0
LDFLAGS    += -lm

vpath %.h   $(DIR_SRC) $(DIR_UTILS)
vpath %.c   $(DIR_SRC) $(DIR_UTILS)

$(DIR_OBJ)/%.o: %.c
	gcc -g $(CFLAGS) -o $@ $<

all: $(C_OBJS)
	gcc -o $(PROG) $(C_OBJS) $(LDFLAGS)

run:
	./$(PROG) --images=test_image_list.txt\
	          --ground=test_gt_list.txt\
	          --params=test_param_list.txt\
	          --evaluation\
	          --results=results

run.eval:
	./$(PROG) --images=eval_image_list.txt\
	          --ground=eval_gt_list.txt\
	          --params=test_param_list.txt\
	          --evaluation\
	          --results=results

run_help:
	./$(PROG) --help_all

eval: 
	./$(PROG) --eval $(shell ls ../test_images/*.png) | tee evaluation.log

clean:
	/bin/rm -f   *.o
	/bin/rm -fr  $(DIR_OBJ)
	/bin/rm -f   compile.log
	/bin/rm -f   resized.png  reverted.png
	/bin/rm -f   predictions.png predictions_sw.png
	/bin/rm -f   result.log memcheck.log evaluation.log
	/bin/rm -f   predict_result.txt
	/bin/rm -f   resized_*.png

cleanup clobber: clean
	/bin/rm -f   $(PROG)
	#/bin/rm -f   $(DIR_UTILS)/lib$(TARGET_LIB).a
	#/bin/rm -f   $(DIR_UTILS)/lib$(TARGET_LIB).so

cleanupall distclean: cleanup
	/bin/rm -fr  results

.PHONY: debug
