
# Linux Temperature Monitor

A Linux kernel character device driver and C++ user-space application that simulates CPU temperature monitoring using `read()` and `ioctl()` system calls.

## Features

- Linux kernel character device (`/dev/tempsensor`)
- Real-time temperature monitoring
- State machine:
  - **NORMAL** (< 60°C)
  - **WARNING** (60–80°C)
  - **CRITICAL** (> 80°C)
- `ioctl()` support for:
  - Resetting the sensor
  - Applying temperature drift

## Project Structure

```
capstone-temp-monitor/
├── driver/
│   ├── tempsensor.c
│   ├── tempsensor_ioctl.h
│   └── Makefile
└── app/
    └── temp_monitor.cpp
```

## Requirements

- Ubuntu 26.04 LTS
- GCC / G++
- Linux kernel headers
- Make

Install dependencies:

```bash
sudo apt update
sudo apt install build-essential g++ make linux-headers-$(uname -r)
```

## Build & Run

### 1. Build the kernel module

```bash
cd driver
make
```

### 2. Load the module

```bash
sudo insmod tempsensor.ko
lsmod | grep tempsensor
```

### 3. Build the application

```bash
cd ../app
g++ -Wall -O2 -o temp_monitor temp_monitor.cpp
```

### 4. Start monitoring

```bash
sudo ./temp_monitor
```

## IOCTL Commands

Reset the sensor:

```bash
sudo ./temp_monitor --reset
```

Apply a drift of **40.0°C** per reading:

```bash
sudo ./temp_monitor --drift 400
```

## Sample Output

```
[13:11:37] Monitoring started.
[13:11:37] Initial state: NORMAL
[13:11:38] temp = 25.8 C (state: NORMAL)
[13:14:54] TRANSITION: NORMAL -> CRITICAL (temp = 89.1 C)
[13:14:56] TRANSITION: CRITICAL -> WARNING (temp = 73.8 C)
```

## Concepts Demonstrated

- Linux Kernel Modules (LKM)
- Character Device Drivers
- Device Files (`/dev`)
- `read()` system call
- `ioctl()` communication
- User Space ↔ Kernel Space interaction
- Finite State Machine (FSM)

## Author

**Ashish Singh Chauhan**
Regd no. 2241003022
Wipro Capstone Project
