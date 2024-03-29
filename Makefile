CC = gcc
PROFILE ?= DEBUG

CCFLAGS_DEBUG = -ggdb -O0 -fno-builtin -DDEBUG
CCFLAGS_RELEASE = -O2

SRC_DIR := src
OBJ_DIR := obj
INC_DIR := include
BIN_DIR := bin
TEST_DIR := test

TGT_INC_DIR := /usr/include/
TGT_BIN_DIR := /usr/lib/

SRCS := $(wildcard $(SRC_DIR)/*.c)
DEPS := $(wildcard $(SRC_DIR)/*.h)
OBJS := $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SRCS))

CCFLAGS += $(CCFLAGS_$(PROFILE)) -I$(INC_DIR) -std=c99 -Wall -Wextra -Wformat #-Werror
LDFLAGS += -lc -lm -lz -lcheck

BINS := $(BIN_DIR)/libimc.a $(BIN_DIR)/libimc.so

# Create static and dynamic libraries
all: $(BINS)

# Copy libraries to /usr/lib
install: all
	cp $(BINS) $(TGT_BIN_DIR)
	cp $(INC_DIR)/*.h $(TGT_INC_DIR)

# Remove object files and binaries
clean:
	rm -f $(OBJ_DIR)/*.o $(BIN_DIR)/*

# Rebuild project
rebuild: clean all

# Create static library
$(BIN_DIR)/libimc.a: $(OBJS)
	ar rcs $@ $^

# Create dynamic library
$(BIN_DIR)/libimc.so: $(SRCS) $(DEPS)
	$(CC) -o $@ $(SRCS) $(DEPS) -shared -fPIC $(CCFLAGS) $(LDFLAGS)
	#strip ./bin/libimc.so

# Create objects
$(OBJ_DIR)/%.o: $(SRCS)
	$(CC) $< -c -o $@ $(CCFLAGS)

# TODO: Modify test to include all tests
test: install
	$(CC) $(TEST_DIR)/test.c -o $(BIN_DIR)/test $(CCFLAGS) $(LDFLAGS) -limc

.PHONY: all install clean rebuild test
