TEST_NAME=MatMul
HSA_RUNTIME_LIB_PATH?=../../core/debug64
LFLAGS= -Wl,--unresolved-symbols=ignore-in-shared-libs
INCS += -I ../../inc -I .
C_FILES := $(wildcard *.c)
OBJ_FILES := $(notdir $(C_FILES:.c=.o))

all: $(TEST_NAME)

$(TEST_NAME): $(OBJ_FILES)
	$(CC) $(LFLAGS) $(OBJ_FILES) -L$(HSA_RUNTIME_LIB_PATH) -lhsa-runtime64 -o $(TEST_NAME)

%.o: %.c
	$(CC) -g -c $(CFLAGS) $(INCS) -o $@ $<

clean:
	rm -rf $(TEST_NAME) main.o
