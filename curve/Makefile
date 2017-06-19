all: curve_interface.o libHilbert2D.so.1.0.1 libMorton2D.so.1.0.1 libTrivial2D.so.1.0.1

curve_interface.o: curve_interface.[ch]
	$(CC) $(CFLAGS) $< -c -o $@

%.o: %.c %.h Makefile
	$(CC) $(CFLAGS) -fpic $< -c -o $@

%.o: %.c Makefile
	$(CC) $(CFLAGS) -fpic $< -c -o $@

libHilbert2D.so.1.0.1: hilbert.o
	$(CC) -shared -fpic -Wl,-soname=libHilbert2D.so.1 -o $@ $^

libMorton2D.so.1.0.1: morton.o
	$(CC) -shared -fpic -Wl,-soname=libMorton2D.so.1 -o $@ $^

libTrivial2D.so.1.0.1: trivial.o
	$(CC) -shared -fpic -Wl,-soname=libTrivial2D.so.1 -o $@ $^

clean:
	rm -f *.o

cleaner: clean
	rm -f libHilbert2D.so.1.0.1 libMorton2D.so.1.0.1 libTrivial2D.so.1.0.1

cleanest: cleaner
