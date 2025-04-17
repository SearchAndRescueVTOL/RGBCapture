CC = gcc
CFLAGS = -Wall -g `pkg-config --cflags gstreamer-1.0 gstreamer-app-1.0`
LDFLAGS = -lpigpio `pkg-config --libs gstreamer-1.0 gstreamer-app-1.0`
TARGET = capture
SRC = gStreamFLIR.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC) $(LDFLAGS)

clean:
	rm -f $(TARGET)
