MYDIR=$(call get-my-dir)

#------------------------------------------------------------------------------
BIN2DASH_SRCS:=\
  $(LIB_MEDIA_SRCS)\
  $(LIB_MODULES_SRCS)\
  $(LIB_PIPELINE_SRCS)\
  $(LIB_UTILS_SRCS)\
  $(MYDIR)/bin2dash.cpp

$(BIN)/bin2dash.so: $(BIN2DASH_SRCS:%=$(BIN)/%.o)
TARGETS+=$(BIN)/bin2dash.so

#------------------------------------------------------------------------------
$(BIN)/%.so:
	$(CXX) $(CFLAGS) -static-libstdc++ -shared -o "$@" $^ \
		$(LDFLAGS)

