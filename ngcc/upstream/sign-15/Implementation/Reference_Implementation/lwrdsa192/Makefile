CC=/usr/bin/gcc
CFLAGS += -std=c99 -Wpedantic -Wall -Wextra -O2
#LDFLAGS=-lcrypto

SOURCES = SIG_lwrdsa192.c polyvec.c packing.c poly.c rounding.c auxfunc.c drng.c
HEADERS = api.h params.h SIG_AlgorithmInstance.h polyvec.h packing.h poly.h rounding.h auxfunc.h drng.h


genKAT: $(HEADERS) $(SOURCES) KAT_SIG.c
	$(CC) $(CFLAGS) -o $@ $(SOURCES) KAT_SIG.c $(LDFLAGS)

.PHONY: clean bench_speed

bench_speed: $(HEADERS) $(SOURCES) test/cpucycles.c test/cpucycles.h test/speed_print.c test/speed_print.h test/test_speed.c
	$(CC) $(CFLAGS) -o $@ $(SOURCES) test/cpucycles.c test/speed_print.c test/test_speed.c $(LDFLAGS)

clean:
	-rm genKAT bench_speed -rf output result_bench_speed.txt
