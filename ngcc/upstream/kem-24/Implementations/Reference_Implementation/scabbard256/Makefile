CC=/usr/bin/gcc
CFLAGS += -O3 -march=native -fomit-frame-pointer
LDFLAGS=-lcrypto

SOURCES= auxfunc.c cbd.c drng.c indcpa.c KEM_scabbard256.c packing.c poly.c polyvec.c verify.c minal.c
HEADERS= auxfunc.h cbd.h drng.h indcpa.h KEM_scabbard256.h packing.h params.h poly.h polyvec.h verify.h minal.h

genKAT: $(HEADERS) $(SOURCES) KAT_KEM.c
	$(CC) $(CFLAGS) -o $@ $(SOURCES) KAT_KEM.c $(LDFLAGS)

.PHONY: clean bench_speed

bench_speed: $(HEADERS) $(SOURCES) test/cpucycles.c test/cpucycles.h test/speed_print.c test/speed_print.h test/test_speed.c
	$(CC) $(CFLAGS) -o $@ $(SOURCES) test/cpucycles.c test/speed_print.c test/test_speed.c $(LDFLAGS)

clean:
	-rm genKAT bench_speed -rf output result_bench_speed.txt