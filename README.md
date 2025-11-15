This repository contains source code to be compiled and run on a raspberry pico 2

**Aim**
- read temperature from the TMP117
- read magnetic strength in the x, y and z axis from the CMPS12
- read UVA, UVB, UVcomp1, and UVcomp intensity from the VEML6075
- read pressure (and possibly temperature) from the ICP10125
- read humitidy from the DollaTek humidity sensor
- save the measurements to the SD card
- Repeat several times
  
**Upload Any Code Related To These Aims Here**

**Pictorial Representation of This Repositories Aim**

![sensor_subdivision](https://github.com/user-attachments/assets/60aeb2ed-6e5f-4fe3-92c4-a83734b6f7ad)

**Hardware:**
- TMP117 sensor
- CMPS12 sensor
- VEML6075
- ICP10125
- DollaTek humidity sensor
- SD card module
- Raspberry Pi Pico 2


## Changes

### Prerequisites
- Python 3.8+
- CMake 3.20+
- Ninja (recommended)
- Git

### Build
```bash
# Automated build (recommended)
python build.py

# Or manual build
cmake --preset release
cmake --build --preset release
```

### Flash to Pico
1. Hold BOOTSEL while plugging in Pico
2. Copy `build/release/sensors_rpi_pico.uf2` or extract the packaged tarball in packages/specific_package.tar.gz to RPI-RP2 drive

## Build Presets

| Preset | Platform | Compiler | Status |
|--------|----------|----------|--------|
| `release` | Cross-platform | GCC | Working |
| `debug` | Cross-platform | GCC | Working |
| `msys2-gcc` | Windows | GCC | **Recommended** |
| `linux-gcc` | Linux | GCC | Working |
| `msys2-clang` | Windows | Clang | Not functional |
| `msvc-clang-cl` | Windows | Clang-CL | Not functional |

**Note:** Clang builds fail due to Pico SDK inline assembly compatibility issues. Use GCC presets. I havent been able to solve this problem it inherits from Pico SDK using ARM-specific inline assembly constraints (=t) that Clang doesn't support, either make a pull request to the Pico SDK for compiler specific intrinsics or find a way for a local override of the function.

## Project Structure

```
├── target/standalone/src/    # Main application
├── lib/                     # Sensor libraries
├── tests/                   # Unit tests
├── build.py                 # Build automation
└── CMakePresets.json        # Build configurations
```

## Testing

Run unit tests:
```bash
python build.py --preset host-tests --run-tests
```

## License

MIT License

