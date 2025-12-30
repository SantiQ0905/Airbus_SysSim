# AIRBUS PRIM Flight Control Simulator

![Build Status](https://github.com/SantiQ0905/Airbus_SysSim/workflows/Build%20and%20Validate/badge.svg)

A high-fidelity Airbus Primary Flight Control Computer (PRIM) simulator with realistic flight dynamics, ECAM displays, Airbus-style protections, and GPWS callout system.

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
  - Alpha Protection with hysteresis (prevents oscillation)
  - Alpha Floor with auto-TOGA
  - High-speed protection
  - ELAC/SEC computer monitoring
  - Smooth control law transitions

- **Realistic Flight Physics**
  - Thrust-based propulsion model
  - Drag and lift simulation
  - Smoothed flaps effects (5 configurations: 0, 1, 2, 3, FULL)
  - Realistic 10-second landing gear extension/retraction animation
  - Energy management (speed/altitude trade-off)
  - Weather effects (wind, turbulence, windshear)

- **GPWS (Ground Proximity Warning System)**
  - Altitude callouts (2500, 1000, 500, 400, 300, 200, 100, 50, 40, 30, 20, 10 ft)
  - PULL UP terrain warning
  - WINDSHEAR warning
  - RETARD callout during landing
  - Visual display with color-coded alerts (ready for audio integration)

- **QF72 Simulation Mode**
  - Manual sensor override for fault injection
  - Simulate false ADR data scenarios
  - Educational tool for understanding sensor failures

## Recent Improvements (v1.1 - December 30, 2025)

### Bug Fixes
- **Landing Gear System**: Fixed LG DISAGREE alert stuck issue
  - Added automatic transit completion after 10-second animation
  - Implemented target position tracking for proper gear state management
  - Landing gear now correctly transitions: UP ↔ TRANSIT (10s) ↔ DOWN

- **Flaps Oscillation**: Eliminated pitch oscillations during flap configuration changes
  - Implemented smoothed flaps effects (~2 second time constant)
  - Flaps lift and drag now transition gradually instead of instant changes
  - More realistic flap deployment behavior

- **Alpha Protection Oscillation**: Improved alpha protection stability
  - Added 2-degree hysteresis (engage at 11°, disengage at 9° AoA)
  - Reduced auto nose-down aggressiveness (from -0.5~-1.5 to -0.2~-0.7)
  - Smoothed protection strength transitions
  - Alpha Floor now has hysteresis (engage at 14°, disengage at 12.5°)
  - Significantly improved flight stability in Normal Law

### New Documentation
- **Audio Implementation Guide** (`AUDIO_IMPLEMENTATION.txt`)
  - Complete guide for integrating SDL_mixer audio system
  - Directory structure and file organization
  - Full list of 37 required audio files (GPWS callouts, alerts, system sounds)
  - Implementation code examples and integration instructions
  - Audio acquisition sources and testing procedures

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
- **Landing Gear**: GEAR DOWN / GEAR UP buttons
  - 10-second extension/retraction animation
  - Cannot retract with weight on wheels
- **QF72 Mode**: Toggle manual sensor override
- **Weather Controls**: Wind, turbulence, and windshear settings
- **Fault Injection**: Simulate various system failures (ELAC, SEC, hydraulics, etc.)

## Architecture

```
PRIM_sim/
├── src/
│   ├── main.cpp              # Main application loop
│   ├── prim_core.cpp         # Flight control logic and flight dynamics
│   ├── prim_core.h           # PRIM core class definition
│   ├── alerts.cpp            # ECAM alert management
│   ├── alerts.h              # Alert system definitions
│   ├── ui_panels.cpp         # ImGui interface panels
│   ├── ui_panels.h           # UI panel declarations
│   └── sim_types.h           # Data structures and enums
├── external/
│   └── imgui/                # Dear ImGui library
├── assets/                   # (To be created for audio)
│   └── audio/
│       ├── gpws/             # GPWS callout sounds
│       ├── alerts/           # Alert and warning sounds
│       ├── systems/          # System operation sounds
│       └── environment/      # Environmental sounds
├── AUDIO_IMPLEMENTATION.txt  # Complete audio integration guide
├── CMakeLists.txt
└── README.md
```

## Audio Integration

The simulator is ready for audio integration! See `AUDIO_IMPLEMENTATION.txt` for:
- Complete implementation guide using SDL_mixer
- Directory structure and file organization
- List of all 37 required audio files
- Code examples and integration steps
- Testing procedures and troubleshooting

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
**Initial Release:** December 24, 2025
**Latest Update:** December 30, 2025 (v1.1)
