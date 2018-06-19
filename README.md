# Basic-ARM-OS
This is a primitive, Unix-like clone operating system built for ARM in C. It features the basic parts of an operating system, such as consecutive job execution and handling.

Overall, the OS isequipped to create, run, and kill processes. This is demonstrated by initializing the first job specified in the p2test file.

The OS is split into multiple phases:

* **Phase 1** creates the intial semaphore constructs - the framework for which jobs are placed into in order to hold jobs waiting for execution and line up jobs that are ready.
* **Phase 2** initializes the first job specified in p2 test and runs through various cases - creating new jobs, killing old jobs, simulating all SYS calls, and handles various I/O interrupts. It features a job scheduler which manages processes and assures sequential execution, while accommodating processes that were waiting for an I/O device to finish. Additionally it asserts mutual exclusion and detects deadlock based on Dijkstra’s semaphore concepts.
