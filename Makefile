BIN?=bin
SRC?=signals/src
EXTRA?=signals/sysroot

SIGNALS_HAS_X11?=0

CFLAGS+=-fPIC

#------------------------------------------------------------------------------

include signals/Makefile
CFLAGS+=-Isrc
CFLAGS+=-std=c++17 # filesystem - put after Signals Makefile

#------------------------------------------------------------------------------

include src/apps/bin2dash/project.mk

#------------------------------------------------------------------------------

include src/apps/bin2dash_app/project.mk

#------------------------------------------------------------------------------

targets: $(TARGETS)

