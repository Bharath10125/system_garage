# System Garage

A repository of lightweight desktop system utilities and tools.

---

## Projects

### 🕒 [Screen Clock (`screen_clock`)](screen_clock/README.md)
A modern, minimalist full-screen flip-card digital clock application with smooth flip animations, built with **C++17** and **Qt 6**. Compatible with both **Linux** and **Windows**.

- **Features**:
  - Fullscreen minimalist black aesthetic.
  - Realistic split-panel flip animation on digit change.
  - Displays hours, minutes, seconds (`HH:MM:SS`) and date (`Day, DD MMM YYYY`).
  - Simple controls: Press `Esc`, `Q`, or click anywhere with the mouse to exit.

---

## Quick Start: Building Screen Clock

### Linux (Ubuntu / Debian / Fedora / Arch)

1. **Install dependencies**:
   - **Ubuntu / Debian**:
     ```bash
     sudo apt update && sudo apt install -y build-essential cmake qt6-base-dev
     ```
   - **Fedora / RHEL**:
     ```bash
     sudo dnf install -y gcc-c++ cmake qt6-qtbase-devel
     ```
   - **Arch Linux**:
     ```bash
     sudo pacman -S --needed base-devel cmake qt6-base
     ```

2. **Build and run**:
   ```bash
   cd screen_clock
   cmake -B build -DCMAKE_BUILD_TYPE=Release
   cmake --build build --config Release
   ./build/fedora_clock
   ```

---

### Windows (MSVC + Qt6)

1. **Prerequisites**:
   - [Visual Studio 2022](https://visualstudio.microsoft.com/) with **Desktop development with C++**
   - [CMake](https://cmake.org/download/) (v3.20+)
   - **Qt 6** (e.g. Qt 6.8+ / 6.9+ MSVC 2022 64-bit). You can install it via [aqtinstall](https://github.com/miurahr/aqtinstall):
     ```cmd
     pip install aqtinstall
     aqt install-qt windows desktop 6.9.3 win64_msvc2022_64 -O C:\Qt
     ```

2. **Build** (run from *x64 Native Tools Command Prompt for VS 2022* or *Developer PowerShell*):
   ```cmd
   cd screen_clock
   cmake -B build -DCMAKE_PREFIX_PATH="C:\Qt\6.9.3\msvc2022_64"
   cmake --build build --config Release
   ```

3. **Deploy Qt Runtime DLLs (windeployqt)**:
   ```cmd
   C:\Qt\6.9.3\msvc2022_64\bin\windeployqt.exe build\Release\fedora_clock.exe
   ```

4. **Run**:
   ```cmd
   .\build\Release\fedora_clock.exe
   ```

---

For detailed project documentation, configuration options, and troubleshooting, see the [screen_clock README](screen_clock/README.md).
