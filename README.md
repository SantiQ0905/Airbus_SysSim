# AIRBUS PRIM Flight Control Simulator

![Build Status](https://github.com/SantiQ0905/Airbus_SysSim/workflows/Build%20and%20Validate/badge.svg)

A high-fidelity Airbus Primary Flight Control Computer (PRIM) simulator with realistic flight dynamics, ECAM displays, and Airbus-style protections.

## Features

- **Authentic Airbus PFD (Primary Flight Display)**
  - Artificial horizon with pitch ladder and roll indicator
  - Speed and altitude tapes
  - Flight Mode Annunciator (FMA)
  - Real-time AoA and Mach display

- **ECAM E/WD Display**
  - Dual engine monitoring (CFM56-5B simulation)
  - Real-time N1, N2, EGT, and fuel flow
  - Color-coded warnings, cautions, and memos
  - Flaps configuration display

- **Flight Control System**
  - Normal, Alternate, and Direct control laws
  - Alpha Protection and Alpha Floor
  - High-speed protection
  - ELAC/SEC computer monitoring

- **Realistic Flight Physics**
  - Thrust-based propulsion model
  - Drag and lift simulation
  - Flaps effects (5 configurations: 0, 1, 2, 3, FULL)
  - Energy management (speed/altitude trade-off)

- **QF72 Simulation Mode**
  - Manual sensor override for fault injection
  - Simulate false ADR data scenarios
  - Educational tool for understanding sensor failures

## Building

### Prerequisites

- CMake 3.28 or higher
- C++20 compatible compiler
- vcpkg (for dependency management)
- SDL2 (installed via vcpkg)

### Build Instructions

#### Windows (Visual Studio)

```bash
# Clone with submodules
git clone --recursive https://github.com/YOUR_USERNAME/PRIM_sim.git
cd PRIM_sim

# Configure and build
cmake -B build -DCMAKE_TOOLCHAIN_FILE=[path-to-vcpkg]/scripts/buildsystems/vcpkg.cmake
cmake --build build --config Release
```

#### Linux/macOS

```bash
# Install dependencies
# Ubuntu/Debian:
sudo apt-get install build-essential cmake ninja-build

# macOS:
brew install cmake ninja

# Configure and build
cmake -B build -DCMAKE_BUILD_TYPE=Release -G Ninja
cmake --build build
```

### Running

```bash
./build/PRIM_sim
```

## Usage

### Normal Flight
1. Set flaps configuration (0-FULL)
2. Adjust thrust levers (IDLE → CLIMB → TOGA)
3. Control pitch and roll with sliders
4. Monitor PFD, ECAM, and F/CTL displays

### QF72 Fault Injection
1. Enable "MANUAL SENSOR OVERRIDE (QF72 Mode)"
2. Inject false sensor readings
3. Observe ECAM warnings and flight control degradation
4. Study protection system behavior

### Realistic Scenarios

**Takeoff:**
- CONF 2 or 3 flaps
- TOGA thrust
- Rotate at 140-150kt

**Climb:**
- CONF 0 (clean)
- CLIMB thrust (0.5-0.6)
- Pitch 5-10°
- Maintain 250kt

**Landing:**
- CONF FULL
- IDLE thrust
- Approach at 130-140kt

## Controls

- **Pitch/Roll Sliders**: Sidestick input
- **Thrust Lever**: 0.0 (IDLE) to 1.0 (TOGA)
- **Flaps Buttons**: 0, 1, 2, 3, FULL
- **QF72 Mode**: Toggle manual sensor override

## Architecture

```
PRIM_sim/
├── src/
│   ├── main.cpp           # Main application loop
│   ├── prim_core.cpp      # Flight control logic
│   ├── alerts.cpp         # ECAM alert management
│   ├── ui_panels.cpp      # ImGui interface
│   └── sim_types.h        # Data structures
├── external/
│   └── imgui/             # Dear ImGui library
├── CMakeLists.txt
└── README.md
```

## License

Educational project for flight simulation and systems understanding.

## Credits

- **Dear ImGui** - UI framework
- **SDL2** - Window and rendering
- Inspired by Airbus A320 family flight control systems

## Disclaimer

This is a simplified educational simulator. It does NOT represent actual Airbus systems and should NOT be used for flight training.

---

**Author:** Santiago Quintana Moreno (A01571222)
**Date:** December 24, 2025
