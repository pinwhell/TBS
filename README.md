# TBS Library

**TBS (Thunder-Byte-Scan)** is a C++ **Single Header** library designed to provide efficient memory pattern scanning with multithreading and SIMD support. It offers a high-performance framework for scanning memory patterns efficiently, making it valuable for applications requiring fast pattern matching.
Features

    - Efficient memory pattern scanning
    - Multithreading support for improved performance
    - SIMD (Single Instruction, Multiple Data) support for parallel processing
    - Simple interface for defining and scanning patterns
    - Flexible pattern results transformation capabilities
    - Pattern Scan load distribution horizontally (even for single thread setups)

## Usage

```c++
// Example usage of TBS library

#include <TBS.hpp>

int main() {
    // Create a state object
    TBS::State state;

    // Add patterns to the state
    state.AddPattern(
        state.PatternBuilder()
        .setUID("pattern_uid")
        .setPattern("00 FF ?? 12")
        .setScanStart(0x10000000)
        .setScanEnd(0x20000000)
        .AddTransformer([](TBS::Pattern::Description& desc, TBS::U64 result) -> TBS::U64 {
          // Middleware for results found
          return result;
        })
        .Build()
    );

    // Perform memory scanning    
    if(!TBS::Scan(state)) {
      // Handle not found...
    }

    // Access scan results
    auto results = state["pattern_uid"];

    // Process results...
    
    return 0;
}
```
## Installation
### Add TBS as a Sub-directory:

Add the TBS library as a sub-directory in your `CMakeLists.txt` file using the `add_subdirectory()` command.

```cmake
add_subdirectory(path/to/TBS)
```

### Link Against TBS Interface:

Link your target against the TBS CMake Interface library using the `target_link_libraries()` command.

```cmake
target_link_libraries(your_target_name TBS)
```

### Configure Options (Optional):

Customize TBS options by setting CMake variables before including the sub-directory.

```cmake
set(TBS_MT ON)          # Enable multithreading
set(TBS_USE_SSE2 ON)    # Enable SSE2 support
set(TBS_USE_AVX ON)    # Enable SSE2 support
```

Simply include the `TBS.hpp` header file in your project

## License

This library is licensed under the **MIT License**. See the LICENSE file for details.

## Contribution

Contributions are welcome! Feel free to open issues or pull requests on the GitHub repository.
