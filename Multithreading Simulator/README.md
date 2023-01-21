# Multithreading-Simulator

This is a simulator that simulates a multi-threaded processor in two configurations: Block Multithreading (MT)
and Fine-grained MT.

## Microarchitecture
Both micro-architectures are Single-Cycle based, so an instruction takes a single clock cycle (excluding Load or Store commands). 
The architecture supports the following command set:
* Memory access commands: `LOAD`, `STORE`
* Arithmetic commands: `ADD`, `ADDI`, `SUB`, `SUBI`
* `HALT` - a special command that will be used to stop the currently running thread (also takes a single cycle).

### In addition, given:
* In the given architecture there are 8 general registers, R0 - R7.
* Arithmetic commands will be executed between registers or between a register and a fixed number.
* Memory access commands will take a number of clock cycles (defined in the parameters file).
* At most one command is executed in each clock cycle (𝐼𝑃𝐶𝑚𝑎𝑥=1.)
* The address space of all threads is e. It can be assumed that there are no information dependencies between threads.
* The virtual address space is the same as the physical address space (we do not refer to Virtual Memory in this exercise).
* For a Fine-grained MT configuration:
  - The context switch is done every one clock cycle.
  - There is no penalty for changing the connection.
* For configuration of Blocked MT:
  - There is a penalty for changing the connection (defined in the parameters file).

## Memory Access
The interface to the simulator is defined in the `sim_api.h` file and the implementation in the `sim_api.c` file. 
The memory interface allows the simulator to read commands and read/write data. 
Since we assume that there are no information dependencies between threads, the actual time writing and/or reading to memory is irrelevant. 
The memory simulator is limited to hold 100 commands for each thread and 100 sequential data.

## Threads
Each processor will run a number of threads as determined in the img file. Context switching by using the Round-Robin (RR) method with initialization to thread 0. 
If any thread has finished (reached the HALT command), it is skipped.
In the context of Fine-grained MT, the realization is in a Flexible configuration. 
This means that the Round-Robin mechanism does not choose a thread that has reached HALT and is wasting a clock cycle on it.
A thread that reaches the halt command is a thread that has finished its work.

## CORE_API
* `CORE_BlockedMT()` – the function will contain a complete simulation of the Blocked MT machine. 
Actually, the function will return when all threads reach the HALT command. 
In this case the data structures will contain the register files and the amount of clock cycles that the program took to run.
* `CORE_BlockedMT_CTX(tcontext context[], int threaded)` – the function will return through the context pointer, for a specific thread, indicates the state of the register file.
* `CORE_BlockedMT_CPI()` – the function returns the system performance in the CPI index.

The above functions are also available for the Fine-grained MT configuration.

### Example:
![Screenshot 2023-01-21 195842](https://user-images.githubusercontent.com/71281877/213880836-9c34b1cb-68c8-4635-b777-5cabab8b6bc2.jpg)


## How to run
The makefile is provided, after building, you will receive an executable file called sim_main to run as follows:
* ./sim_main <test_file>
