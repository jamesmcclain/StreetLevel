GDAL_CFLAGS ?= -I$(HOME)/local/gdal/include
GDAL_LDFLAGS ?= -L$(HOME)/local/gdal/lib -lgdal -lopenjp2
CFLAGS ?= -march=native -mtune=native -Ofast -g
CFLAGS += -std=c11 $(GDAL_CFLAGS)
LDFLAGS += $(GDAL_LDFLAGS) -lm -lOpenCL


all: streetlevel

streetlevel: main.o rasterio.o opencl.o viewshed_cpu.o viewshed_cl.o
	$(CC) $^ $(LDFLAGS) -o $@

main.o: main.c *.h
	$(CC) $(CFLAGS) $< -c -o $@

%.o: %.c %.h Makefile
	$(CC) $(CFLAGS) $< -c -o $@

%.o: %.c Makefile
	$(CC) $(CFLAGS) $< -c -o $@

test: streetlevel
	rm -f /tmp/viewshed.tif /tmp/viewshed.tif.aux.xml
	streetlevel /tmp/ned.tif /tmp/viewshed.tif

valgrind: streetlevel
	valgrind --leak-check=full streetlevel /tmp/ned.tif /tmp/viewshed.tif

cachegrind: streetlevel
	valgrind --tool=cachegrind --branch-sim=yes streetlevel /tmp/ned.tif /tmp/viewshed.tif

clean:
	rm -f *.o

cleaner: clean
	rm -f streetlevel

cleanest: cleaner
	rm -f cachegrind.out.*
