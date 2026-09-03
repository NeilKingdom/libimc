CC := gcc
PROFILE ?= DEBUG

CFLAGS_DEBUG := -ggdb -O0 -fno-builtin -DDEBUG
CFLAGS_RELEASE := -O2

SRC_DIR := src
INC_DIR := include
TGT_DIR := build
OBJ_DIR := $(TGT_DIR)/obj

TGT_INC_DIR := /usr/include/imc
TGT_LIB_DIR := /usr/lib/imc

SRCS := $(wildcard $(SRC_DIR)/*.c)
OBJS := $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SRCS))

CPPFLAGS += -I$(INC_DIR) -Ilib/ezc/include
CFLAGS += $(CFLAGS_$(PROFILE)) -std=c99 -Wall -Wextra -fPIC
LDLIBS += -lm -lz

LIBS := $(TGT_DIR)/libimc.a $(TGT_DIR)/libimc.so

# Default goal
all: $(LIBS)

# Remove object files and binaries
clean:
	rm -rf $(TGT_DIR)/*

# Clean -> build
rebuild: clean all

# Add headers/libraries to TGT_INC_DIR + TGT_LIB_DIR
install: all
	install -d $(TGT_INC_DIR)
	install -d $(TGT_LIB_DIR)
	install -m644 $(INC_DIR)/*.h $(TGT_INC_DIR)
	install -m644 $(TGT_DIR)/libimc.a $(TGT_LIB_DIR)
	install -m755 $(TGT_DIR)/libimc.so $(TGT_LIB_DIR)

# Remove installed headers/libraries
uninstall:
	rm -rf $(TGT_INC_DIR)
	rm -rf $(TGT_LIB_DIR)

# Compile static library
$(TGT_DIR)/libimc.a: $(OBJS)
	ar rcs $@ $^

# Compile dynamic library
$(TGT_DIR)/libimc.so: $(OBJS)
	$(CC) -shared -o $@ $^ $(LDLIBS)

# Create objects
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

# Create obj dir (and in turn build dir) if they don't exist
$(OBJ_DIR):
	mkdir -p $@

.PHONY: all clean rebuild install uninstall
