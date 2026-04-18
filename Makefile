CC            := clang
LDFLAGS       := -flto
SRC           := ./examples/kgz_gunzip.c ./lib/kgz_gzip.c ./lib/kgz_deflate.c ./lib/kgz_huffman.c ./lib/kgz_bitstream.c ./lib/kgz_buffer.c ./lib/kgz_arena.c
COMMON_CFLAGS := -Wall -Wextra -std=c23 -g -O2 -flto -I./lib

MAIN_TARGET    := kgz_gunzip
MAIN_BUILD_DIR := obj
MAIN_CFLAGS    := $(COMMON_CFLAGS)
MAIN_OBJ       := $(SRC:%.c=$(MAIN_BUILD_DIR)/%.o)
MAIN_DEP       := $(MAIN_OBJ:.o=.d)

.PHONY: all release clean

all: main

main: $(MAIN_TARGET)

$(MAIN_TARGET): $(MAIN_OBJ)
	$(CC) $(MAIN_OBJ) $(LDFLAGS) -o $@

$(MAIN_BUILD_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(MAIN_CFLAGS) -c $< -o $@

-include $(MAIN_DEP) $(DBG_DEP)

clean:
	rm -rf obj $(MAIN_TARGET) $(DBG_TARGET)
