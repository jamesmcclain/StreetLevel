all: pdal.o

pdal.o: pdal.cpp pdal.h pdal_point.h Makefile
	$(CXX) -fopenmp -Wno-terminate $(CXXFLAGS) $< -c -o $@

clean:
	rm -f *.o

cleaner: clean

cleanest: cleaner
