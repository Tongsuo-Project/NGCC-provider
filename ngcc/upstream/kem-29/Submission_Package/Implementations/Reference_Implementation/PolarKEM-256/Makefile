# Portable strict-C99 build for the official API_PKC KEM KAT driver.

CC ?= cc
CPPFLAGS ?=
CFLAGS ?= -O2
LDFLAGS ?=
LDLIBS ?=

STRICT_CFLAGS = -std=c99 -Wall -Wextra -Wpedantic -Werror
CFLAGS += $(STRICT_CFLAGS)

ifeq ($(OS),Windows_NT)
ifeq ($(origin CC),default)
CC = gcc
endif
EXEEXT = .exe
CLEAN_COMMAND = del /Q *.o *.d $(PROGRAM) 2>NUL
else
EXEEXT =
CLEAN_COMMAND = $(RM) $(OBJECTS) $(SANITIZED_OBJECTS) $(AUXILIARY_SAN_OBJECTS) $(PROGRAM)
endif

PROGRAM = KAT_KEM$(EXEEXT)
PROJECT_SOURCES = KEM_AlgorithmInstance.c polarkem_core.c polarkem_polar.c \
                  polarkem_pack.c polarkem_ct.c KAT_KEM.c
AUXILIARY_SOURCES = auxfunc.c drng.c
SOURCES = $(PROJECT_SOURCES) $(AUXILIARY_SOURCES)
OBJECTS = $(SOURCES:.c=.o)
SANITIZED_OBJECTS = $(PROJECT_SOURCES:.c=.san.o)
AUXILIARY_SAN_OBJECTS = $(AUXILIARY_SOURCES:.c=.auxiliary.o)
HEADERS = KEM_AlgorithmInstance.h auxfunc.h drng.h polarkem_core.h \
          polarkem_params.h polarkem_polar.h polarkem_pack.h polarkem_ct.h
SANITIZER_CFLAGS = -O1 -g $(STRICT_CFLAGS) -fsanitize=address,undefined \
                   -fno-omit-frame-pointer
AUXILIARY_SAN_CFLAGS = $(SANITIZER_CFLAGS) -fno-sanitize=shift
SANITIZER_LDFLAGS = -fsanitize=address,undefined

.PHONY: all run sanitizer clean

all: $(PROGRAM)

$(PROGRAM): $(OBJECTS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(OBJECTS) $(LDLIBS)

%.o: %.c $(HEADERS)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

%.san.o: %.c $(HEADERS)
	$(CC) $(CPPFLAGS) $(SANITIZER_CFLAGS) -c $< -o $@

# Keep ASan/UBSan enabled for the exact official helpers, disabling only the
# known shift check in their upstream SM3 rotate macro.
%.auxiliary.o: %.c $(HEADERS)
	$(CC) $(CPPFLAGS) $(AUXILIARY_SAN_CFLAGS) -c $< -o $@

run: $(PROGRAM)
	./$(PROGRAM)

sanitizer:
	$(MAKE) clean
	$(MAKE) $(SANITIZED_OBJECTS) $(AUXILIARY_SAN_OBJECTS)
	$(CC) $(SANITIZER_LDFLAGS) -o $(PROGRAM) $(SANITIZED_OBJECTS) $(AUXILIARY_SAN_OBJECTS) $(LDLIBS)

clean:
	-$(CLEAN_COMMAND)
