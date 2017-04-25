GDAL_CFLAGS ?= -I$(HOME)/local/gdal/include
GDAL_LDFLAGS ?= -L$(HOME)/local/gdal/lib -lgdal -lopenjp2
PDAL_CXXFLAGS ?= -I$(HOME)/local/pdal/include
PDAL_LDFLAGS ?= -L$(HOME)/local/pdal/lib -lpdalcpp -llaszip
OPENCL_LDFLAGS ?= -lOpenCL
CFLAGS ?= -Wall -march=native -mtune=native -Ofast -g
CFLAGS += -std=c11 $(GDAL_CFLAGS)
CXXFLAGS += -std=c++11 $(PDAL_CXXFLAGS)


all: streetlevel

viewshed_test: viewshed_test.o rasterio.o opencl.o viewshed.o
	$(CC) $^ $(GDAL_LDFLAGS) $(OPENCL_LDFLAGS) -lm -o $@

dem_test: dem_test.o pdal.o
	$(CXX) $^ $(PDAL_LDFLAGS) -o $@

viewshed_test.o: viewshed_test.c *.h
	$(CC) $(CFLAGS) $< -c -o $@

%.o: %.c %.h Makefile
	$(CC) $(CFLAGS) $< -c -o $@

%.o: %.c Makefile
	$(CC) $(CFLAGS) $< -c -o $@

%.o: %.cpp %.h Makefile
	$(CXX) $(CXXFLAGS) $< -c -o $@

%o: %.cpp Makefile
	$(CXX) $(CXXFLAGS) $< -c -o $@

test: viewshed_test
	rm -f /tmp/viewshed.tif /tmp/viewshed.tif.aux.xml ; viewshed_test /tmp/ned.tif /tmp/viewshed.tif

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
