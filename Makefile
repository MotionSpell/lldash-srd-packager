BIN?=bin
SRC?=signals/src
EXTRA?=signals/sysroot

SIGNALS_HAS_X11?=0

#------------------------------------------------------------------------------

include signals/Makefile
CFLAGS+=-Isrc

#------------------------------------------------------------------------------

include src/apps/pcl2dash/project.mk
CFLAGS+=-Iextra/include

targets: $(TARGETS)

