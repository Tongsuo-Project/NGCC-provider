
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

RBC_SRC:=src/rbc-163
RBC_INCLUDE:=-I src/rbc-163

SRC:=src
INCLUDE:=-I src $(RBC_INCLUDE)
LIB:=$(DRNG_INCLUDE) $(AUXFUNC_INCLUDE) $(RANDOM_SOURCE_INCLUDE) $(XKCP_INCLUDE) $(WRAPPER_INCLUDE)

MAIN_BRQC:=main_brqc.c
MAIN_KAT:=KAT_KEM.c

BRQC_OBJS:=rbc_elt.o rbc_vec.o rbc_poly.o rbc_qre.o rbc_vspace.o parsing.o qpoly.o gabidulin.o brqc.o kem.o
BRQC_OBJS_VERBOSE:=rbc_elt.o rbc_vec.o rbc_poly.o rbc_qre.o rbc_vspace.o parsing.o qpoly.o gabidulin-verbose.o brqc-verbose.o kem-verbose.o
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



KEM_BRQC-256.o: KEM_BRQC-256.c | folders
	@echo -e "\n### Compiling $@\n"
	$(CC) $(C_FLAGS) -c $< $(INCLUDE) $(LIB) -o $(BUILD)/$@

BRQC-256: $(BRQC_OBJS) KEM_BRQC-256.o $(LIB_OBJS) | folders
	@echo -e "\n### Compiling BRQC-256\n"
	$(CC) $(C_FLAGS) $(MAIN_BRQC) $(addprefix $(BUILD)/, $^) $(INCLUDE) $(LIB) -o $(BIN)/$@

BRQC-256-kat: $(BRQC_OBJS) KEM_BRQC-256.o $(LIB_OBJS) | folders
	@echo -e "\n### Compiling BRQC-256 KAT\n"
	$(CC) $(C_FLAGS) $(MAIN_KAT) $(addprefix $(BUILD)/, $^) $(INCLUDE) $(LIB) -o $(BIN)/$@

BRQC-256-verbose: $(BRQC_OBJS_VERBOSE) KEM_BRQC-256.o $(LIB_OBJS) | folders
	@echo -e "\n### Compiling BRQC-256 (verbose mode)\n"
	$(CC) $(C_FLAGS) $(MAIN_BRQC) $(addprefix $(BUILD)/, $^) $(INCLUDE) $(LIB) -D VERBOSE -o $(BIN)/$@



clean:
	rm -f vgcore.*
	rm -rf ./bin
	rm -rf ./output
