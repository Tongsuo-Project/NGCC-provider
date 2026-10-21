all: kat_kem 

CC=/usr/bin/gcc
CFLAGS += -O3  -march=native -flto -fomit-frame-pointer 
LDFLAGS=  

SOURCES= auxfunc.c  drng.c  mod.c ntt.c poly.c KEM_Cheetah.c 
HEADERS= params.h auxfunc.h drng.h mod.h ntt.h  poly.h KEM_Cheetah.h


KAT_KEM = KAT_KEM.c


.PHONY: all

kat_kem: $(HEADERS) $(SOURCES) $(KAT_KEM)
	$(CC) $(CFLAGS) -o $@ $(SOURCES) $(KAT_KEM) $(LDFLAGS)


.PHONY: clean

clean:
	-rm  kat_kem  