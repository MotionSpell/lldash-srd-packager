MYDIR=$(call get-my-dir)
TARGET:=$(BIN)/bin2dash_app.exe
TARGETS+=$(TARGET)
EXE_PCL2DASH_SRCS:=\
  $(LIB_UTILS_SRCS)\
  $(LIB_APPCOMMON_SRCS)\
  $(MYDIR)/main.cpp
$(TARGET): $(EXE_BIN2DASH_APP_SRCS:%=$(BIN)/%.o)
