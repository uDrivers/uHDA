# uHDA
## A portable HDA driver

### Features
- Playback (both with an async queue function and a callback)
- An api to find outputs to which you can play different content at the same time

### Todo
- Unified subset of kernel API shared with [uACPI](https://github.com/UltraOS/uACPI)
- Document usage of API
- Recording
- Multichannel to different outputs (likely used for surround)
- More output info, right now only the output type is exposed

### Usage

### 1. Add uHDA sources and include directories into your project

#### If using CMake
Add the following lines to your cmake file:
```cmake
include(uhda/uhda.cmake)

target_sources(my-kernel PRIVATE ${UHDA_SOURCES})
target_include_directories(my-kernel PRIVATE ${UHDA_INCLUDES})
```

#### Other build systems
- Add all .cpp files from [src](src) into your target sources
- Add [include](include) into your target include directories

### 2. Implement kernel API
uHDA needs kernel functions to e.g. map memory and manage PCI interrupts.
This API is declared in [kernel_api.h](include/uhda/kernel_api.h) and is implemented by your kernel.

### 3. Use uHDA as the driver for supported PCI devices
In [uhda.h](include/uhda/uhda.h) there are macros/functions that you can use to match the PCI devices supported by uHDA so you know which devices to initialize uHDA for.
Other API and their usage is also documented in that same file,
generally the order you use them in is the same order that they are declared within
the file.

