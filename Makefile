
PROGNAME=libringbuf.a

C_FLAGSA += -fPIC -Werror 

LD_FLAGS += -L.

all:
	gcc -c libringbuf.c $(C_FLAGSA)
	ar rcs $(PROGNAME) *.o 

test:
	gcc -g test.c -o test_ringbuf $(LD_FLAGS) -lringbuf
	
clean:
	rm -rf *.o *.a test_ringbuf $(INSTALLDIR)/$(PROGNAME)
