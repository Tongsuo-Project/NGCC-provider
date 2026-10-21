CC ?= gcc
CFLAGS ?= -O2 -std=c99 -Wpedantic -Wall -Wextra
CPPFLAGS ?= -DVIPER_LEVEL=128 -DVIPER_USE_E8_CODEC=1
SRC = KAT_KEM.c KEM_AlgorithmInstance.c kem.c viper.c viper_arith.c viper_e8.c viper_message_codec.c verify.c drng.c auxfunc.c viper_api_rng.c viper_api_hash.c
BIN = KAT_KEM_API_PKC
.PHONY: all clean kat
all: $(BIN)
$(BIN): $(SRC)
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ $(SRC)
kat: $(BIN)
	./$(BIN)
	mkdir -p ../../../Test_Vectors
	cp output/KAT_KEM_MAMBA-Viper-128.txt ../../../Test_Vectors/KAT_KEM_MAMBA-Viper-128.txt
clean:
	rm -rf $(BIN) output *.o
