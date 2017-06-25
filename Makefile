GDAL_CFLAGS ?= -I$(HOME)/local/gdal/include
GDAL_LDFLAGS ?= -L$(HOME)/local/gdal/lib -lgdal -lopenjp2
PDAL_CXXFLAGS ?= -I$(HOME)/local/pdal/include
PDAL_LDFLAGS ?= -L$(HOME)/local/pdal/lib -lpdalcpp -llaszip
STXXL_CXXFLAGS ?= -I$(HOME)/local/stxxl/include
STXXL_LDFLAGS ?= -L$(HOME)/local/stxxl/lib -lstxxl
OPENCL_LDFLAGS ?= -lOpenCL
CFLAGS ?= -Wall -march=native -mtune=native -Ofast -g
CXXFLAGS ?= -Wall -march=native -mtune=native -Ofast -g
CFLAGS += -std=c11 $(GDAL_CFLAGS)
CXXFLAGS += -std=c++11


all: point_index dem curve/libHilbert2D.so.1.0.1 curve/libMorton2D.so.1.0.1 curve/libTrivial2D.so.1.0.1

# Components

curve/curve_interface.o curve/libHilbert2D.so.1.0.1 curve/libMorton2D.so.1.0.1 curve/libTrivial2D.so.1.0.1: curve/*.[ch]
	CC="$(CC)" CFLAGS="$(CFLAGS)" make -C curve

index/index.o: index/index.[ch]
	CC="$(CC)" CFLAGS="$(CFLAGS)" make -C index

pdal/pdal.o: pdal/pdal.cpp pdal/*.h
	CXX=$(CXX) CXXFLAGS="$(CXXFLAGS) $(PDAL_CXXFLAGS) $(STXXL_CXXFLAGS) -I$(shell pwd)" make -C pdal

# Programs

dem: dem.o curve/curve_interface.o index/index.o
	$(CC) $^ -ldl -o $@

point_index: point_index.o pdal/pdal.o curve/curve_interface.o index/index.o
	$(CC) -rdynamic -fopenmp $^ $(PDAL_LDFLAGS) $(STXXL_LDFLAGS) -ldl -lstdc++ -lm -o $@

# Test Programs

viewshed_test: viewshed_test.o rasterio.o opencl.o viewshed.o
	$(CC) $^ $(GDAL_LDFLAGS) $(OPENCL_LDFLAGS) -o $@

sort_test: sort_test.o opencl.o bitonic.o partition.o
	$(CC) $^ $(OPENCL_LDFLAGS) -o $@

# Object Files

%.o: %.c %.h Makefile
	$(CC) $(CFLAGS) $< -c -o $@

%.o: %.c Makefile
	$(CC) $(CFLAGS) $< -c -o $@

# Additional Targets

test: sort_test viewshed_test
	sort_test 24
	rm -f /tmp/viewshed.tif* ; viewshed_test /tmp/ned.tif /tmp/viewshed.tif

valgrind: sort_test viewshed_test
	valgrind --leak-check=full viewshed_test /tmp/ned.tif /tmp/viewshed.tif

cachegrind: sort_test viewshed_test
	valgrind --tool=cachegrind --branch-sim=yes viewshed_test /tmp/ned.tif /tmp/viewshed.tif

clean:
	rm -f *.o
	make -C curve clean
	make -C index clean
	make -C pdal clean

cleaner: clean
	rm -f point_index dem viewshed_test sort_test
	rm -f stxxl.errlog stxxl.log
	make -C curve cleaner
	make -C index cleaner
	make -C pdal cleaner

cleanest: cleaner
	rm -f cachegrind.out.*
	make -C curve cleanest
	make -C index cleanest
	make -C pdal cleanest
