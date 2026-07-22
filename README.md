
![Calculator](./resources/calculator.png)

[中文](README_zh.md)

# Calculator \| Operator Lambda

A scientific/graphical calculator firmware for Raspberry Pi Pico, with CAS (Computer Algebra System) and graphical display support.

PCB Preview ---->

<img src="./resources/pcb_front.png" alt="PCB Preview" width="300" align="right" />

**Status**: Under active development. Core CAS and display subsystem are partially implemented; basic arithmetic, expression parsing, and simplification are functional. Integration and matrix operations are in progress.

---

## Features

- CAS core: expression parsing, simplification, differentiation, integration (partial), rational arithmetic, symbolic manipulation.
- Display: 1.8" ST7735 TFT (160×128) with custom font rendering (ClassWiz-style fonts included).
- Input: 6×6 matrix keypad, debounced and mapped to mathematical operators.
- Storage: SD card support (WIP) for program/data loading.
- Modular design: separated CAS, display interface, input handling, and hardware abstraction.

---

## Hardware Requirements

- MCU: Raspberry Pi Pico (primary), Arduino Nano (power/control)
- Display: ST7735-based 1.8" TFT (SPI, with optional SD slot)
- Keypad: 6×6 matrix keypad (custom PCB – see `hardware/kicad` directory)
- Optional: SD card module (SPI)

Refer to `wiring.txt` (deprecated) and `hardware/kicad/` for schematics and PCB design. Gerber files are available in `hardware/gerber*.zip`.

---

## Project Structure

```
calculator/
├── CMakeLists.txt          # Build configuration
├── build.sh                # Convenience build script
├── inc/                    # Header files
│   ├── cas/                # CAS core (parser, simplifier, integrator, etc.)
│   └── dispinterface/      # Display abstraction
├── src/                    # Source files
│   ├── main.cpp            # Entry point
│   └── cas/                # CAS implementation
├── fonts/                  # Generated font data (ClassWiz style)
├── font2h/                 # Font conversion tools (font2h, preview)
├── hardware/               # PCB design (KiCad, Fritzing(deprecated), Gerber)
├── test/                   # Unit tests (CAS functions)
├── resources/              # Images (logo, PCB preview)
└── LICENSE                 # GPLv3
```

---

## Build Instructions

1. Install the [Raspberry Pi Pico SDK](https://github.com/raspberrypi/pico-sdk) and toolchain (arm-none-eabi-g++, CMake).
2. Clone this repository:
   ```
   git clone https://github.com/hdkghc/calculator.git
   cd calculator
   ```
3. Build:
   - Recommended: run `./build.sh` (automates CMake configuration and build).
   - Manual:
     ```
     mkdir build && cd build
     cmake ..
     make
     ```
4. Flash: drag the generated `.uf2` file to your Pico's mass storage device, or use `picotool`.

---

## Dependencies

- Raspberry Pi Pico SDK (required)
- C++17 compiler (`arm-none-eabi-g++`)

---

## License

This project is licensed under the **GNU General Public License v3.0**.  
See [LICENSE](LICENSE) for details.

---

## Disclaimer

This is an amateur, non-commercial open‑source project. All hardware and software are provided “as is”, without any warranty. The author assumes no liability for any damage or injury resulting from the use, assembly, or modification of this project.

---

## Contributing

Contributions are welcome! Please read [CONTRIBUTING.md](CONTRIBUTING.md) for guidelines.