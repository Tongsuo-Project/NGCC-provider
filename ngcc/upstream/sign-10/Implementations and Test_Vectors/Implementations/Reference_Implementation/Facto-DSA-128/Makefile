CC ?= gcc
CFLAGS = -std=c99 -O2 -Wall -Wextra -Wpedantic
LDFLAGS_SO = -shared
TARGET = kat_sig


FACTODSA_SRC = \
	auxfunc.c \
	drng.c \
	SIG_AlgorithmInstance.c

FACTODSA_OBJ = $(FACTODSA_SRC:.c=.o)

all: $(TARGET)

tests: run_benchmarks.c
	$(CC) $(CFLAGS) -DNDEBUG run_benchmarks.c -o run_benchmarks

bench: test.c SIG_AlgorithmInstance.c SIG_AlgorithmInstance.h auxfunc.c auxfunc.h drng.c drng.h
	$(CC) $(CFLAGS) -DNDEBUG -o bench_sig test.c SIG_AlgorithmInstance.c auxfunc.c drng.c

$(TARGET): KAT_SIG.o SIG_AlgorithmInstance.o auxfunc.o drng.o
	$(CC) $(CFLAGS) -o $(TARGET) KAT_SIG.o SIG_AlgorithmInstance.o auxfunc.o drng.o

KAT_SIG.o: KAT_SIG.c SIG_AlgorithmInstance.h drng.h
	$(CC) $(CFLAGS) -c KAT_SIG.c -o KAT_SIG.o

SIG_AlgorithmInstance.o: SIG_AlgorithmInstance.c SIG_AlgorithmInstance.h drng.h auxfunc.h
	$(CC) $(CFLAGS) -c SIG_AlgorithmInstance.c -o SIG_AlgorithmInstance.o

auxfunc.o: auxfunc.c auxfunc.h
	$(CC) $(CFLAGS) -c auxfunc.c -o auxfunc.o

libfactodsa128-ref.so: $(FACTODSA_OBJ)
	$(CC) $(CFLAGS) -fPIC -DFACTODSA_SO_BUILD -c SIG_AlgorithmInstance.c -o SIG_AlgorithmInstance.o
	$(CC) $(CFLAGS) -fPIC -c auxfunc.c -o auxfunc.o
	$(CC) $(CFLAGS) -fPIC -c drng.c -o drng.o
	$(CC) $(CFLAGS) -fPIC -o $@ $(FACTODSA_OBJ) $(LDFLAGS_SO)


drng.o: drng.c drng.h
	$(CC) $(CFLAGS) -c drng.c -o drng.o

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET) run_benchmarks bench_sig *.o
	rm -f *.so
	rm -rf output

.PHONY: all run clean
