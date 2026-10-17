# Tempbar

A lightweight, native macOS menu bar status monitor designed to display accurate real-time hardware thermals and cooling metrics directly via Apple System Management Controller (SMC) and IOKit.

Supports Apple Silicon (M1, M2, M3, M4, M5 series) and Intel-based Mac computers.

---

## Architecture Overview

Tempbar operates via a zero-subprocess, in-process C++ bridge linking SwiftUI directly to native IOKit interfaces. 

- Single-Pass Hardware Discovery: Sensor keys are scanned and indexed once during application initialization. Subsequent live polls execute in under 0.2 milliseconds directly in memory.
- Energy Optimized: Near 0.0% CPU overhead with full timer coalescing, preventing unnecessary processor core wakeups and preserving battery life.
- Menu Bar Exclusive: Runs as a native background status item (`LSUIElement = true`) without cluttering the macOS Dock.

---

## Features

- Real-Time Hardware Telemetry:
  - Processor (CPU): Average and peak core temperatures.
  - Graphics (GPU): Average and peak cluster temperatures.
  - Memory (RAM): Aggregate multi-sensor temperature readings.
  - Power & Logic: Battery pack and SoC / Fabric thermal values.
  - Active Cooling: Fan tachometer readings with passive zero-RPM silent mode detection.
- Liquid Glass Opacity Control: Adjustable background material translucency (30% to 100%).
- Multi-Language Localization: Built-in support for English, Turkish, French, Italian, and Russian.
- System Appearance Integration: Automatically adapts to macOS Light and Dark appearance modes.
- Configurable Polling Rate: Selectable interval timings (1s, 3s default, 5s).

---

## Project Structure

- `TempbarApp.swift`: Main application entry point utilizing SwiftUI MenuBarExtra.
- `PopoverContentView.swift`: Floating glass popover interface and telemetry views.
- `HardwareMonitor.swift`: In-process telemetry manager and background timer service.
- `Localization.swift`: Localization engine supporting runtime language switching.
- `Models.swift`: Decodable telemetry data models.
- `smc_bridge.cpp`: In-process C++ implementation bridging AppleSMC queries to C interfaces.
- `TempbarBridge.h`: C bridging header exposing in-memory query methods to Swift.
- `macos.cpp`: Low-level IOKit client and AppleSMC command parser.
- `Info.plist`: Application bundle configuration and system permissions.
- `build.sh`: Automated compilation and packaging script.

---

## Building and Running

Execute the build script to compile the in-process bridge, Swift modules, and bundle the application:

```bash
./build.sh
open Tempbar.app
```

---

## Requirements

- macOS 13.0 (Ventura) or newer
- Xcode Command Line Tools (`xcode-select --install`)
- Architecture: Apple Silicon (ARM64) or Intel (x86_64)

---

## License

This project is licensed under the MIT License.

Developed by reuzen.
