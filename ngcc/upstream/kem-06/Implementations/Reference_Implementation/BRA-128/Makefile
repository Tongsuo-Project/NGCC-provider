
CC=gcc
C_FLAGS:=-std=c99 -Wpedantic -Wall -Wextra -O2

DRNG_SRC:=drng.c
DRNG_INCLUDE:=-I .

AUXFUNC_SRC:=auxfunc.c
AUXFUNC_INCLUDE:=-I .

RANDOM_SOURCE_SRC:=lib/random_source/random_source.c
RANDOM_SOURCE_INCLUDE:=-I lib/random_source

XKCP_SRC:=lib/XKCP
XKCP_INCLUDE:=-I lib/XKCP -I lib/XKCP/opt64

WRAPPER_INCLUDE:=-I lib/wrapper

RBC_SRC:=src/rbc-67
RBC_INCLUDE:=-I src/rbc-67

SRC:=src
INCLUDE:=-I src $(RBC_INCLUDE)
LIB:=$(DRNG_INCLUDE) $(AUXFUNC_INCLUDE) $(RANDOM_SOURCE_INCLUDE) $(XKCP_INCLUDE) $(WRAPPER_INCLUDE)

MAIN_BRA:=main_bra.c
MAIN_KAT:=KAT_KEM.c

BRA_OBJS:=rbc_elt.o rbc_vec.o rbc_poly.o rbc_qre.o rbc_vspace.o parsing.o qpoly.o augmented_gabidulin.o bra.o kem.o
BRA_OBJS_VERBOSE:=rbc_elt.o rbc_vec.o rbc_poly.o rbc_qre.o rbc_vspace.o parsing.o qpoly.o augmented_gabidulin-verbose.o bra-verbose.o kem-verbose.o
LIB_OBJS:=drng.o auxfunc.o random_source.o KeccakHash.o KeccakHashtimes4.o KeccakSponge.o KeccakSpongetimes4.o SimpleFIPS202.o KeccakP-1600-opt64.o KeccakP-1600-times4-on1.o

BUILD:=bin/build
BIN:=bin



folders:
	@echo -e "\n### Creating build folders\n"
	mkdir -p $(BUILD)

drng.o: folders
	@echo -e "\n### Compiling $@\n"
	$(CC) $(C_FLAGS) -c $(DRNG_SRC) $(DRNG_INCLUDE) -o $(BUILD)/$@

auxfunc.o: folders
	@echo -e "\n### Compiling $@\n"
	$(CC) $(C_FLAGS) -c $(AUXFUNC_SRC) $(AUXFUNC_INCLUDE) -o $(BUILD)/$@

random_source.o: folders
	@echo -e "\n### Compiling $@\n"
	$(CC) $(C_FLAGS) -c $(RANDOM_SOURCE_SRC) $(RANDOM_SOURCE_INCLUDE) $(DRNG_INCLUDE) -o $(BUILD)/$@

KeccakHash.o: folders
	@echo -e "\n### Compiling $@\n"
	$(CC) $(C_FLAGS) -c $(XKCP_SRC)/KeccakHash.c $(XKCP_INCLUDE) -o $(BUILD)/$@

KeccakHashtimes4.o: folders
	@echo -e "\n### Compiling $@\n"
	$(CC) $(C_FLAGS) -c $(XKCP_SRC)/KeccakHashtimes4.c $(XKCP_INCLUDE) -o $(BUILD)/$@

KeccakSponge.o: folders
	@echo -e "\n### Compiling $@\n"
	$(CC) $(C_FLAGS) -c $(XKCP_SRC)/KeccakSponge.c $(XKCP_INCLUDE) -o $(BUILD)/$@

KeccakSpongetimes4.o: folders
	@echo -e "\n### Compiling $@\n"
	$(CC) $(C_FLAGS) -c $(XKCP_SRC)/KeccakSpongetimes4.c $(XKCP_INCLUDE) -o $(BUILD)/$@

SimpleFIPS202.o: folders
	@echo -e "\n### Compiling $@\n"
	$(CC) $(C_FLAGS) -c $(XKCP_SRC)/SimpleFIPS202.c $(XKCP_INCLUDE) -o $(BUILD)/$@

KeccakP-1600-opt64.o: folders
	@echo -e "\n### Compiling $@\n"
	$(CC) $(C_FLAGS) -c $(XKCP_SRC)/opt64/KeccakP-1600-opt64.c $(XKCP_INCLUDE) -o $(BUILD)/$@

KeccakP-1600-times4-on1.o: folders
	@echo -e "\n### Compiling $@\n"
	$(CC) $(C_FLAGS) -c $(XKCP_SRC)/opt64/KeccakP-1600-times4-on1.c $(XKCP_INCLUDE) -o $(BUILD)/$@



rbc_%.o: $(RBC_SRC)/rbc_%.c | folders
	@echo -e "\n### Compiling $@\n"
	$(CC) $(C_FLAGS) -c $< $(RBC_INCLUDE) $(LIB) -o $(BUILD)/$@

%.o: $(SRC)/%.c | folders
	@echo -e "\n### Compiling $@\n"
	$(CC) $(C_FLAGS) -c $< $(INCLUDE) $(LIB) -o $(BUILD)/$@

%-verbose.o: $(SRC)/%.c | folders
	@echo -e "\n### Compiling $@ (verbose mode)\n"
	$(CC) $(C_FLAGS) -c $< $(INCLUDE) $(LIB) -D VERBOSE -o $(BUILD)/$@



KEM_BRA-128.o: KEM_BRA-128.c | folders
	@echo -e "\n### Compiling $@\n"
	$(CC) $(C_FLAGS) -c $< $(INCLUDE) $(LIB) -o $(BUILD)/$@

BRA-128: $(BRA_OBJS) KEM_BRA-128.o $(LIB_OBJS) | folders
	@echo -e "\n### Compiling BRA-128\n"
	$(CC) $(C_FLAGS) $(MAIN_BRA) $(addprefix $(BUILD)/, $^) $(INCLUDE) $(LIB) -o $(BIN)/$@

BRA-128-kat: $(BRA_OBJS) KEM_BRA-128.o $(LIB_OBJS) | folders
	@echo -e "\n### Compiling BRA-128 KAT\n"
	$(CC) $(C_FLAGS) $(MAIN_KAT) $(addprefix $(BUILD)/, $^) $(INCLUDE) $(LIB) -o $(BIN)/$@

BRA-128-verbose: $(BRA_OBJS_VERBOSE) KEM_BRA-128.o $(LIB_OBJS) | folders
	@echo -e "\n### Compiling BRA-128 (verbose mode)\n"
	$(CC) $(C_FLAGS) $(MAIN_BRA) $(addprefix $(BUILD)/, $^) $(INCLUDE) $(LIB) -D VERBOSE -o $(BIN)/$@



clean:
	rm -f vgcore.*
	rm -rf ./bin
	rm -rf ./output
