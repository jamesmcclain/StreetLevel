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


.PHONY: lib idx

all: point_index lib idx

lib:
	CC="$(CC)" CFLAGS="$(CFLAGS)" make -C curve

idx:
	CC="$(CC)" CFLAGS="$(CFLAGS)" make -C index

point_index: point_index.o opencl.o pdal.o curve/curve_interface.o index/write.o
	$(CC) -rdynamic -fopenmp $^ $(PDAL_LDFLAGS) $(OPENCL_LDFLAGS) $(STXXL_LDFLAGS) -ldl -lstdc++ -lm -o $@

viewshed_test: viewshed_test.o rasterio.o opencl.o viewshed.o
	$(CC) $^ $(GDAL_LDFLAGS) $(OPENCL_LDFLAGS) -o $@

sort_test: sort_test.o opencl.o bitonic.o partition.o
	$(CC) $^ $(OPENCL_LDFLAGS) -o $@

%.o: %.c %.h Makefile
	$(CC) $(CFLAGS) $< -c -o $@

%.o: %.c Makefile
	$(CC) $(CFLAGS) $< -c -o $@

%.o: %.cpp %.h Makefile
	$(CXX) $(CXXFLAGS) $< -c -o $@

pdal.o: pdal.cpp pdal.h Makefile
	$(CXX) -fopenmp -Wno-terminate $(CXXFLAGS) $< $(PDAL_CXXFLAGS) $(STXXL_CXXFLAGS) -c -o $@

%o: %.cpp Makefile
	$(CXX) $(CXXFLAGS) $< -c -o $@

test: sort_test viewshed_test
	sort_test 24
	rm -f /tmp/viewshed.tif* ; viewshed_test /tmp/ned.tif /tmp/viewshed.tif

valgrind: sort_test viewshed_test
	valgrind --leak-check=full dem_test /tmp/1422.las blah
	valgrind --leak-check=full viewshed_test /tmp/ned.tif /tmp/viewshed.tif

cachegrind: sort_test viewshed_test
	valgrind --tool=cachegrind --branch-sim=yes dem_test /tmp/1422.las blah
	valgrind --tool=cachegrind --branch-sim=yes viewshed_test /tmp/ned.tif /tmp/viewshed.tif

clean:
	rm -f *.o
	make -C curve clean
	make -C index clean

cleaner: clean
	rm -f point_index viewshed_test sort_test
	rm -f stxxl.errlog stxxl.log
	make -C curve cleaner
	make -C index cleaner

cleanest: cleaner
	rm -f cachegrind.out.*
	make -C curve cleanest
	make -C index cleanest
