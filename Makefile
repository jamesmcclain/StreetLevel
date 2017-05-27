GDAL_CFLAGS ?= -I$(HOME)/local/gdal/include
GDAL_LDFLAGS ?= -L$(HOME)/local/gdal/lib -lgdal -lopenjp2
PDAL_CXXFLAGS ?= -I$(HOME)/local/pdal/include
PDAL_LDFLAGS ?= -L$(HOME)/local/pdal/lib -lpdalcpp -llaszip
OPENCL_LDFLAGS ?= -lOpenCL
CFLAGS ?= -Wall -march=native -mtune=native -Ofast -g
CXXFLAGS ?= -Wall -march=native -mtune=native -Ofast -g
CFLAGS += -std=c11 $(GDAL_CFLAGS)
CXXFLAGS += -std=c++11 $(PDAL_CXXFLAGS)


all: viewshed_test dem_test

viewshed_test: viewshed_test.o rasterio.o opencl.o viewshed.o
	$(CC) $^ $(GDAL_LDFLAGS) $(OPENCL_LDFLAGS) -o $@

dem_test: dem_test.o pdal.o opencl.o
	$(CC) $^ $(PDAL_LDFLAGS) $(OPENCL_LDFLAGS) -lstdc++ -o $@

sort_test: sort_test.o opencl.o bitonic.o partition.o
	$(CC) $^ $(PDAL_LDFLAGS) $(OPENCL_LDFLAGS) -lstdc++ -o $@

%.o: %.c %.h Makefile
	$(CC) $(CFLAGS) $< -c -o $@

%.o: %.c Makefile
	$(CC) $(CFLAGS) $< -c -o $@

%.o: %.cpp %.h Makefile
	$(CXX) $(CXXFLAGS) $< -c -o $@

%o: %.cpp Makefile
	$(CXX) $(CXXFLAGS) $< -c -o $@

test: viewshed_test dem_test sort_test
	# rm -f /tmp/viewshed.tif* ; viewshed_test /tmp/ned.tif /tmp/viewshed.tif
	# dem_test /tmp/interesting.las 20
	sort_test 20

valgrind: viewshed_test dem_test
	# valgrind --leak-check=full viewshed_test /tmp/ned.tif /tmp/viewshed.tif
	valgrind --leak-check=full dem_test /tmp/interesting.las blah

cachegrind: viewshed_test dem_test
	# valgrind --tool=cachegrind --branch-sim=yes viewshed_test /tmp/ned.tif /tmp/viewshed.tif
	valgrind --tool=cachegrind --branch-sim=yes dem_test /tmp/interesting.las blah

clean:
	rm -f *.o

cleaner: clean
	rm -f viewshed_test dem_test

cleanest: cleaner
	rm -f cachegrind.out.*
