CC            := clang

SANITIZE := -fsanitize=address,undefined,leak,integer,nullability,float-divide-by-zero,unsigned-integer-overflow,implicit-conversion,local-bounds -fsanitize-address-use-after-scope -fsanitize-undefined-trap-on-error

LDFLAGS       := -flto $(SANITIZE)
MAIN_SRC      := ./examples/kgz_gunzip.c ./lib/kgz_gzip.c ./lib/kgz_deflate.c ./lib/kgz_huffman.c ./lib/kgz_bitstream.c ./lib/kgz_buffer.c ./lib/kgz_arena.c
CPP_SRC       := ./examples/kgz_gunzip_cpp.cpp ./lib/kgz_gzip.c ./lib/kgz_deflate.c ./lib/kgz_huffman.c ./lib/kgz_bitstream.c ./lib/kgz_buffer.c ./lib/kgz_arena.c
COMMON_CFLAGS := -Wall -Wextra -Wshadow -Wconversion -Wpedantic -Wvla -Werror -g -O2 -flto -I./lib $(SANITIZE)

MAIN_TARGET    := kgz_gunzip
MAIN_BUILD_DIR := obj
MAIN_CFLAGS    := $(COMMON_CFLAGS)
MAIN_OBJ       := $(MAIN_SRC:%.c=$(MAIN_BUILD_DIR)/%.o)
MAIN_DEP       := $(MAIN_OBJ:.o=.d)

CPP_TARGET    := kgz_gunzip_cpp
CPP_BUILD_DIR := obj_cpp
CPP_CFLAGS    := $(COMMON_CFLAGS)
CPP_OBJ       := $(CPP_SRC:%.cpp=$(CPP_BUILD_DIR)/%.o)
CPP_OBJ       := $(CPP_OBJ:%.c=$(CPP_BUILD_DIR)/%.o)
CPP_DEP       := $(CPP_OBJ:.o=.d)

.PHONY: all release clean cpp

all: main

main: $(MAIN_TARGET)
c++: $(CPP_TARGET)

$(MAIN_TARGET): $(MAIN_OBJ)
	$(CC) $(MAIN_OBJ) $(LDFLAGS) -o $@

$(MAIN_BUILD_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(MAIN_CFLAGS) -c $< -o $@

$(CPP_TARGET): $(CPP_OBJ)
	$(CC) $(CPP_OBJ) $(LDFLAGS) -o $@

$(CPP_BUILD_DIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CC) $(CPP_CFLAGS) -c $< -o $@

$(CPP_BUILD_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CPP_CFLAGS) -c $< -o $@

-include $(MAIN_DEP)
-include $(CPP_DEP)

clean:
	rm -rf obj obj_cpp $(MAIN_TARGET) $(CPP_TARGET)
