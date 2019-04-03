MYDIR=$(call get-my-dir)
TARGET:=$(BIN)/pcl2dash.exe
TARGETS+=$(TARGET)
EXE_PCL2DASH_SRCS:=\
  $(LIB_MEDIA_SRCS)\
  $(LIB_MODULES_SRCS)\
  $(LIB_PIPELINE_SRCS)\
  $(LIB_UTILS_SRCS)\
  $(LIB_APPCOMMON_SRCS)\
  $(MYDIR)/../../../signals/src/lib_appcommon/safemain.cpp\
  $(MYDIR)/main.cpp\
  $(MYDIR)/json2pipeline.cpp\
  $(MYDIR)/cwi_pcl_encoder.cpp\
$(MYDIR)/json_encoder_params.cpp
$(TARGET): $(EXE_PCL2DASH_SRCS:%=$(BIN)/%.o)
