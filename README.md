# IO-benchmark
This is a utility for UNIX-like systems file input-output benchmarking. Test IO-schedulers, file systems and storage hardware with this benchmark.

## Build
This utility has only default C dependencies. To build, launch ```make all```.

Binaries will appear in **build** folder.

## Using
Launch ```io-benchmark --help``` and view options.

Utility creates one or a few files in the specified folder, writes equal count of bytes in each file, does ```sync``` and gets time of this operations. Then it reads files and gets time again.
