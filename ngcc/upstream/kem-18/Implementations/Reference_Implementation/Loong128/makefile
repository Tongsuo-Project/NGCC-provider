all:   kat_kem  

CC=/usr/bin/gcc
CFLAGS += -O3 -march=native -fomit-frame-pointer 
LDFLAGS= 

SOURCES= auxfunc.c  drng.c  poly.c   KEM_Loong.c
HEADERS= params.h auxfunc.h drng.h poly.h  KEM_Loong.h



KAT_KEM = KAT_KEM.c


.PHONY: all

kat_kem: $(HEADERS) $(SOURCES) $(KAT_KEM)
	$(CC) $(CFLAGS) -o $@ $(SOURCES) $(KAT_KEM) $(LDFLAGS)


.PHONY: clean

clean:
	-rm   kat_kem 