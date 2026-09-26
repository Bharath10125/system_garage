# Screen Clock

A high-performance, minimalist full-screen flip digital clock written in **C++17** using **Qt 6** (`Qt6::Core`, `Qt6::Gui`, `Qt6::Widgets`). Runs seamlessly on both **Linux** and **Windows**.

---

## Features

- **Split-Panel Flip Animation**: Animated flip transitions for time and date digit changes with smooth easing.
- **Full Display**:
  - **Time**: 6 flip-card panels displaying Hours, Minutes, and Seconds (`HH : MM : SS`).
  - **Date**: 4 flip-card panels displaying Day of Week, Day, Month, and Year (`DDD DD MMM YYYY`).
- **High-DPI Support**: Automatically scales on 1080p, 1440p, 4K, and Ultrawide displays via Qt 6's native High-DPI handling.
- **Low Overhead**: Hardware-accelerated rendering using standard Qt paint events and 60 FPS timer updates during flips.
- **Controls**:
  - `Esc` or `Q`: Exit full-screen clock.
  - Left Mouse Click: Exit full-screen clock.

---

## Project Structure

```text
screen_clock/
├── CMakeLists.txt        # CMake build configuration
├── README.md             # Documentation (this file)
└── src/
    ├── main.cpp          # Application entry point & Qt event loop
    ├── ClockWindow.h     # Clock widget declaration & FlipDigit struct
    └── ClockWindow.cpp   # Custom paint engine, easing curves & event handlers
```

---

## Prerequisites

- **C++ Compiler**: C++17 compliant compiler
  - Linux: GCC 9+ or Clang 10+
  - Windows: MSVC 2019 / 2022 (Visual Studio 2022 recommended)
- **CMake**: Version 3.20 or newer
- **Qt 6**: Includes `Qt6Core`, `Qt6Gui`, and `Qt6Widgets`

---

## Building on Linux

### 1. Install Dependencies

#### Fedora / RHEL / CentOS Stream:
```bash
sudo dnf install -y gcc-c++ cmake qt6-qtbase-devel
```

#### Ubuntu / Debian / Linux Mint:
```bash
sudo apt update
sudo apt install -y build-essential cmake qt6-base-dev
```

#### Arch Linux / Manjaro:
```bash
sudo pacman -S --needed base-devel cmake qt6-base
```

#### openSUSE:
```bash
sudo zypper install cmake gcc-c++ qt6-base-devel
```

---

### 2. Configure & Build

From the `screen_clock` directory:

```bash
# Configure the build with Release optimizations
cmake -B build -DCMAKE_BUILD_TYPE=Release

# Compile the executable
cmake --build build --config Release -j$(nproc)
```

---

### 3. Run

```bash
./build/fedora_clock
```

*(Optional) Install system-wide:*
```bash
sudo cmake --install build
```

---

## Building on Windows

### 1. Prerequisites Setup

1. **Visual Studio 2022**:
   - Download from [visualstudio.microsoft.com](https://visualstudio.microsoft.com/).
   - When installing, check the workload **Desktop development with C++**.
2. **CMake**:
   - Download and install from [cmake.org/download](https://cmake.org/download/) (ensure CMake is added to system `PATH`).
3. **Qt 6**:
   - **Option A (Fastest via CLI - `aqtinstall`)**:
     Open PowerShell or Command Prompt:
     ```powershell
     pip install aqtinstall
     aqt install-qt windows desktop 6.9.3 win64_msvc2022_64 -O C:\Qt
     ```
     *(This installs Qt 6.9.3 MSVC 64-bit into `C:\Qt\6.9.3\msvc2022_64`)*
   - **Option B (Official Qt Online Installer)**:
     Download from [qt.io/download](https://www.qt.io/download) and select **Qt 6.x -> MSVC 2022 64-bit**.

---

### 2. Configure & Build (Command Line)

Open **x64 Native Tools Command Prompt for VS 2022** (or **Developer PowerShell for VS 2022**):

```cmd
cd path\to\system_garage\screen_clock

# Generate build configuration pointing to your Qt6 installation directory
cmake -B build -DCMAKE_PREFIX_PATH="C:\Qt\6.9.3\msvc2022_64"

# Build the Release binary
cmake --build build --config Release
```

*Note: Replace `C:\Qt\6.9.3\msvc2022_64` with your actual Qt 6 installation path if installed elsewhere.*

---

### 3. Deploy Qt Dependencies (`windeployqt`)

To run the binary standalone outside Visual Studio, bundle the required Qt runtime DLLs and plugins using `windeployqt`:

```cmd
C:\Qt\6.9.3\msvc2022_64\bin\windeployqt.exe build\Release\fedora_clock.exe
```

This automatically copies `Qt6Core.dll`, `Qt6Gui.dll`, `Qt6Widgets.dll`, and platform plugins into the `build\Release\` folder.

---

### 4. Run

```cmd
.\build\Release\fedora_clock.exe
```

---

### Alternative: Building via Visual Studio GUI

1. Open **Visual Studio 2022**.
2. Select **Open a local folder** and choose the `screen_clock` directory.
3. If Qt is not automatically found, configure CMake Settings in VS (`Project` > `CMake Settings for FedoraClock`):
   - Add CMake command argument: `-DCMAKE_PREFIX_PATH=C:/Qt/6.9.3/msvc2022_64`
4. Set configuration dropdown to **x64-Release**.
5. Press `F5` or `Ctrl + F5` to compile and run.

---

## Troubleshooting

- **Qt6 package not found during CMake configure**:
  - Specify `-DCMAKE_PREFIX_PATH=<path-to-qt6>` when configuring.
  - Linux: Ensure `qt6-base-dev` (Debian/Ubuntu) or `qt6-qtbase-devel` (Fedora) is installed.
  - Windows: Verify the path contains `lib/cmake/Qt6`.
- **Missing DLL error on Windows when launching `fedora_clock.exe`**:
  - Run `windeployqt build\Release\fedora_clock.exe` as described above, or add the Qt `bin` directory (`C:\Qt\6.9.3\msvc2022_64\bin`) to your user/system `PATH`.
- **Display looks stretched or misaligned**:
  - Qt6 handles High-DPI scaling automatically. Ensure your OS display scale factor is standard (100%, 125%, 150%, 200%).
