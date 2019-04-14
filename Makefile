BIN?=bin
SRC?=signals/src
EXTRA?=signals/sysroot

SIGNALS_HAS_X11?=0

CFLAGS+=-fPIC

#------------------------------------------------------------------------------

include signals/Makefile
CFLAGS+=-Isrc

#------------------------------------------------------------------------------

include src/apps/pcl2dash/project.mk
CFLAGS+=-Iextra/include

#------------------------------------------------------------------------------

include src/apps/bin2dash/project.mk

#------------------------------------------------------------------------------

include src/apps/bin2dash_app/project.mk

#------------------------------------------------------------------------------

targets: $(TARGETS)

