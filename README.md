# Concurrent Task Engine

A lightweight, multi-threaded C++ task execution engine that prioritizes jobs using a custom Max-Heap queue, complete with an in-place Python CLI monitoring dashboard backed by Redis.

## Overview
This project showcases low-level multi-threading infrastructure patterns in C++. Instead of spinning up threads on the fly, it creates a fixed worker pool at startup to eliminate thread creation overhead. Tasks are pushed dynamically with different priority levels, forcing urgent jobs to jump to the head of the line automatically.

## Key Architecture Details
* **Fixed Worker Pool**: Instantiates a user-defined number of persistent background threads at system boot.
* **Thread Synchronization**: Uses `std::mutex` and `std::condition_variable` with predicate checks to handle worker signaling safely while preventing spurious wakeups.
* **Lock-Free Work Execution**: Workers release the queue lock *before* running the underlying task (`task.func()`), preventing the thread pool from bottlenecking.
* **Thread-Safe Metrics Context**: Synchronizes external C-library database mutations across parallel threads via an isolated `redis_mutex`.

## Tech Stack
* **Core Systems**: C++17, POSIX Threads (`-lpthread`), `hiredis` (C Redis client driver)
* **Storage Layer**: Redis (In-memory datastore for fast counter caching)
* **Reporting UI**: Python 3 (`redis-py` using cursor-return `\r` updates)

## Installation & Setup

1. **Install Base System Tools (Ubuntu/WSL)**:
   ```bash
   sudo apt update && sudo apt install build-essential redis-server python3 python3-pip -y
   pip3 install redis
   ```

2. **Compile Hiredis Source**:
   ```bash
   git clone https://github.com
   cd hiredis && make && sudo make install && sudo ldconfig && cd ..
   ```

## Running the Demo

1. **Start Redis Server**:
   ```bash
   redis-server
   ```

2. **Launch Terminal Dashboard**:
   ```bash
   python3 dashboard.py
   ```

3. **Compile & Run Task Engine**:
   ```bash
   g++ -std=c++17 Engine.cpp -o task_engine -lpthread -lhiredis
   ./task_engine
   ```
