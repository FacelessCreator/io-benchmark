CC=gcc
APP_COMPILE_ARGS=-Wall -Wextra -Werror -g

.PHONY: all clear

all: build build/io-benchmark-reader build/io-benchmark-writer build/io-benchmark build/filebomb-benchmark-reader build/filebomb-benchmark-writer build/filebomb-benchmark

clear:
	rm -r build

build:
	mkdir -p build

build/io-benchmark-reader: src/io-benchmark-reader.c
	$(CC) $(APP_COMPILE_ARGS) -o $@ $<

build/io-benchmark-writer: src/io-benchmark-writer.c
	$(CC) $(APP_COMPILE_ARGS) -o $@ $<

build/io-benchmark: src/io-benchmark.c
	$(CC) $(APP_COMPILE_ARGS) -o $@ $<

build/filebomb-benchmark-writer: src/filebomb-benchmark-writer.c
	$(CC) $(APP_COMPILE_ARGS) -o $@ $<

build/filebomb-benchmark-reader: src/filebomb-benchmark-reader.c
	$(CC) $(APP_COMPILE_ARGS) -o $@ $<

build/filebomb-benchmark: src/filebomb-benchmark.c
	$(CC) $(APP_COMPILE_ARGS) -o $@ $<
