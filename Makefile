.PHONY: all clean

# Program names
PROGRAMS   := RGBCaptureAndSave RGBCaptureAndDisplay RGBCaptureAndSave_Multithread

# Installation directories for pylon
PYLON_ROOT ?= /opt/pylon

# Build tools and flags
LD         := $(CXX)
CPPFLAGS := -I$(PYLON_ROOT)/include \
            $(shell $(PYLON_ROOT)/bin/pylon-config --cflags) \
            $(shell pkg-config --cflags opencv4)
CXXFLAGS   := # e.g., -g -O0 for debugging
LDFLAGS    := $(shell $(PYLON_ROOT)/bin/pylon-config --libs-rpath)
LDLIBS     := $(shell $(PYLON_ROOT)/bin/pylon-config --libs) \
              $(shell pkg-config --libs opencv4)

# Rules for building
all: $(PROGRAMS)

%: %.o
	$(LD) $(LDFLAGS) -o $@ $^ $(LDLIBS)

%.o: %.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c -o $@ $<

clean:
	$(RM) $(PROGRAMS) *.o

