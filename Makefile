GDAL_CFLAGS ?= -I$(HOME)/local/gdal/include
GDAL_LDFLAGS ?= -L$(HOME)/local/gdal/lib
CFLAGS ?= -ggdb3 -O0
LDFLAGS += $(GDAL_LDFLAGS)
CFLAGS += $(GDAL_CFLAGS)

all: streetlevel

streetlevel: main.o
	$(CC) $(LDFLAGS) $^ -o $@

%.o: %.c %.h
	$(CC) $(CFLAGS) %< -c -o $@

%.o: %.c %.h
	$(CC) $(CFLAGS) $< -c -o $@

clean:
	rm -f *.o

cleaner: clean
	rm -f streetlevel

cleanest: cleaner
