# IO-benchmark
This is a utility pack for UNIX-like systems file input-output benchmarking. Test IO-schedulers, file systems and storage hardware.

## Build
Utilities have only default C dependencies. To build, launch ```make all```.

Binaries will appear in **build** folder.

## IO-benchmark
Launch ```build/io-benchmark --help``` and view options.

Utility creates one or a few files in the specified folder, writes equal count of bytes in each file, does ```sync``` and gets time of this operations. Then it reads files and gets time again.

## Filebomb-benchmark
Launch ```build/filebomb-benchmark --help``` and view options.

Utility writes lots of small files in the specified folder, then ```sync``` data, reads all files and returns total time of writing and reading.
