# Basic-ARM-OS
## Description
This is a primitive, Unix-like clone operating system built for ARM in C. It features the basic parts of an operating system, such as consecutive job execution and handling.

Overall, the OS isequipped to create, run, and kill processes. This is demonstrated by initializing the first job specified in the p2test file.

The OS is split into multiple phases:

* **Phase 1** creates the intial semaphore constructs - the framework for which jobs are placed into in order to hold jobs waiting for execution and line up jobs that are ready.
* **Phase 2** initializes the first job specified in p2 test and runs through various cases - creating new jobs, killing old jobs, simulating all SYS calls, and handles various I/O interrupts. It features a job scheduler which manages processes and assures sequential execution, while accommodating processes that were waiting for an I/O device to finish. Additionally it asserts mutual exclusion and detects deadlock based on Dijkstra’s semaphore concepts.

## Prerequisites
You will need:
* A version of GCC able to compile C code for ARM. A Makfile has been included for easy use. It uses gcc-arm-none-eabi.
* The uARM emulator: https://github.com/mellotanica/uARM. To compile, you need qt5-qmake, qt5-default, make, gcc, libelf1, libelf-dev, and libboost-dev. Running `./compile` should be all that's neccesary. Installation can be done by typing:
`sudo ./install.sh`
`sudo apt-get install gcc-arm-none-eabi`

It is highly recommended that you run this on an Ubuntu-like Linux distribution.

## Usage
After compiling the OS, load the 'kernel.core.uarm' file into uARM and hit run. Test status is printed to the display manual, and a message denoting the passed tests will be displayed until completion (when the final is killed, the OS will shut down).
