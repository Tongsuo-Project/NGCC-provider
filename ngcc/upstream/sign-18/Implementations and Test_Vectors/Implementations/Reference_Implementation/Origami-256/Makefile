CC = gcc
KAT_NUM_TESTS ?= 1
BENCH_ITER ?= 3
BENCH_MESSAGE_BYTES ?= 64
CFLAGS = -std=c11 -O3 -Wall -Wextra -Wpedantic -Wshadow -Wpointer-arith -DKAT_NUM_TESTS=$(KAT_NUM_TESTS) -DORIGAMI_OPT=REF

INSTANCE = Origami-256
KAT = KAT_SIG_$(INSTANCE)
BENCH = bench_$(INSTANCE)
BENCH_DRIVER = ../../Common/bench_sig.c
COMM_COST = comm_cost_$(INSTANCE)
COMM_COST_DRIVER = ../../Common/comm_cost.c
OUT = output/KAT_SIG_$(INSTANCE).txt
TV_OUT = ../../../Test_Vectors/KAT_SIG_$(INSTANCE).txt

OBJS = SIG_AlgorithmInstance.o KAT_SIG.o drng.o auxfunc.o symmetric_iccs.o origami_ref.o aes.o
BENCH_OBJS = SIG_AlgorithmInstance.o bench_sig.o drng.o auxfunc.o symmetric_iccs.o origami_ref.o aes.o
COMM_COST_OBJS = SIG_AlgorithmInstance.o comm_cost.o drng.o auxfunc.o symmetric_iccs.o origami_ref.o aes.o

.PHONY: all kat bench run-bench comm-cost run-comm-cost clean

all: $(KAT) $(BENCH) $(COMM_COST)

$(KAT): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

$(BENCH): $(BENCH_OBJS)
	$(CC) $(CFLAGS) -o $@ $(BENCH_OBJS)

bench_sig.o: $(BENCH_DRIVER)
	$(CC) $(CFLAGS) -I. -c -o $@ $(BENCH_DRIVER)

kat: $(KAT)
	./$(KAT)
	mkdir -p ../../../Test_Vectors
	cp $(OUT) $(TV_OUT)

bench: $(BENCH)

run-bench: $(BENCH)
	./$(BENCH) $(BENCH_ITER) $(BENCH_MESSAGE_BYTES)

clean:
	rm -f $(OBJS) bench_sig.o comm_cost.o $(BENCH) $(KAT) $(COMM_COST)
	rm -rf output

$(COMM_COST): $(COMM_COST_OBJS)
	$(CC) $(CFLAGS) -o \$@ $(COMM_COST_OBJS)

comm_cost.o: $(COMM_COST_DRIVER)
	$(CC) $(CFLAGS) -I. -c -o \$@ $(COMM_COST_DRIVER)

comm-cost: $(COMM_COST)

run-comm-cost: $(COMM_COST)
	./$(COMM_COST)
