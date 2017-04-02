GDAL_CFLAGS ?= -I$(HOME)/local/gdal/include
GDAL_LDFLAGS ?= -L$(HOME)/local/gdal/lib -lgdal -lopenjp2
CFLAGS ?= -ggdb3 -O0
CFLAGS += -std=c99 $(GDAL_CFLAGS)
LDFLAGS += $(GDAL_LDFLAGS) -lm


all: streetlevel

streetlevel: main.o rasterio.o viewshed.o
	$(CC) $^ $(LDFLAGS) -o $@

main.o: main.c *.h
	$(CC) $(CFLAGS) $< -c -o $@

%.o: %.c %.h Makefile
	$(CC) $(CFLAGS) %< -c -o $@

%.o: %.c %.h Makefile
	$(CC) $(CFLAGS) $< -c -o $@

test: streetlevel
	streetlevel /tmp/ned.tif /tmp/viewshed.tif

clean:
	rm -f *.o

cleaner: clean
	rm -f streetlevel

cleanest: cleaner
