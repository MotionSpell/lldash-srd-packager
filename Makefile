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

$(BIN)/bin2dash_version.mk:
	@mkdir -p $(BIN)
	$(SRC)/../scripts/version.sh > $(BIN)/bin2dash_version.h
	@echo "" > "$@"
CFLAGS+=-I$(BIN)
ifneq ($(MAKECMDGOALS),clean)
include $(BIN)/bin2dash_version.mk
endif

targets: $(TARGETS) $(BIN)/bin2dash_version.h

