BIN?=bin
SRC?=signals/src
EXTRA?=signals/sysroot

SIGNALS_HAS_X11?=0

CFLAGS+=-fPIC

ifeq ($(DEBUG), 1)
  CFLAGS+=-g3
  LDFLAGS+=-g
else
  CFLAGS+=-O3
endif

ifeq ($(DEBUG), 0)
  # disable all warnings in release mode:
  # the code must always build, especially old versions with recent compilers
  CFLAGS+=-w -DNDEBUG
  LDFLAGS+=-Xlinker -s
endif

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

