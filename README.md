# Developing a Windows C++ app (MSVC, C++26) on headless Linux

This project is built with the **real MSVC toolchain** on Linux using
[msvc-wine](https://github.com/mstorsjo/msvc-wine), run under **Wine**, and edited in
**Neovim with clangd 22**. Tested on Ubuntu 24.04.

```
source (Linux) --cl.exe via Wine--> game.exe (Windows PE) --Wine--> runs on Linux
        \
         clangd 22 reads compile_commands.json + .clangd for LSP
```

## 1. Prerequisites

```bash
sudo apt install wine64 python3 msitools ca-certificates winbind git ninja-build cmake
```

## 2. Install MSVC and the Windows SDK

```bash
git clone https://github.com/mstorsjo/msvc-wine
cd msvc-wine
./vsdownload.py --dest ~/msvc     # accepts Microsoft's license, downloads the newest toolchain
./install.sh ~/msvc               # creates the cl/link/... wrapper scripts
```

Add the wrappers to your `PATH` (put this in `~/.bashrc` or `~/.zshrc` so every shell has it):

```bash
export PATH=~/msvc/bin/x64:$PATH
```

Verify:

```bash
which cl    # should print ~/msvc/bin/x64/cl
cl          # should print the compiler banner
```

If CMake later says `Could not find compiler set in environment variable CC: cl`, `cl`
is not on the `PATH` of that shell.

## 3. Build with CMake and Ninja

In `CMakeLists.txt`, link the MSVC runtime **statically**. Otherwise the exe needs
`MSVCP140D.dll`, `VCRUNTIME140D.dll`, `ucrtbased.dll`, etc., which Wine does not ship:

```cmake
cmake_minimum_required(VERSION 3.15)
project(game CXX)

set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")  # /MT, /MTd
set(CMAKE_CXX_STANDARD 23)   # becomes /std:c++latest; use 26 if your CMake accepts it
```

Configure and build:

```bash
CC=cl CXX=cl cmake -GNinja -B build \
  -DCMAKE_SYSTEM_NAME=Windows \
  -DCMAKE_CROSSCOMPILING_EMULATOR=/usr/lib/wine/wine64 \
  -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
cmake --build build
ln -sf build/compile_commands.json .    # for clangd
```

Delete `build/` before reconfiguring after a failed configure, since a failed attempt
leaves a stale cache.

C++26 support in MSVC lags GCC and Clang. Check Microsoft's C++ conformance page for
what your installed toolset supports.

## 4. Run

```bash
/usr/lib/wine/wine64 ./build/src/game.exe
ctest --test-dir build        # uses CMAKE_CROSSCOMPILING_EMULATOR
```

Do not run the `.exe` directly: `Exec format error` just means Linux is trying to
execute a Windows binary.

### Wine noise

| Message | Meaning |
|---|---|
| `it looks like wine32 is missing...` | Nag from the Ubuntu `wine` wrapper script. Harmless for x64 binaries. Calling `/usr/lib/wine/wine64` directly avoids it, or install `wine32:i386`. |
| `fixme:kernelbase:AppPolicyGetProcessTerminationMethod` | Wine stub for a UCRT shutdown call. Harmless. |
| `err:module:import_dll Library ...D.dll not found` | Debug runtime DLLs are missing. Link the runtime statically (section 3). |

Hide fixmes but keep real errors:

```bash
export WINEDEBUG=fixme-all
```

If the game opens a window on a headless machine, run it with a virtual display
(`xvfb-run /usr/lib/wine/wine64 ./build/src/game.exe`) or from a machine with a desktop
session. Wine is not Windows: test GUI, DirectX and driver code on real Windows before
shipping.

## 5. Language server (clangd 22)

The MSVC STL needs a recent clang, and Ubuntu 24.04 only ships clang 18. Install 22
from the official LLVM repository (side by side, everything is suffixed `-22`):

```bash
wget https://apt.llvm.org/llvm.sh
chmod +x llvm.sh
sudo ./llvm.sh 22
sudo apt install clangd-22
clangd-22 --version
```

### Why a `.clangd` file is needed

The `cl` wrapper sets the `INCLUDE`/`LIB` environment variables itself, so
`compile_commands.json` contains no system header paths and clangd cannot find
`<iostream>` or `<windows.h>`. Add the paths in a `.clangd` file in the project root.

Find the real versions first:

```bash
ls ~/msvc/vc/tools/msvc/                 # MSVC version, e.g. 14.52.37025
ls ~/msvc/kits/10/Include/               # SDK version, e.g. 10.0.26100.0
find ~/msvc -iname windows.h
```

Then create `.clangd` (absolute paths only, `~` is not expanded; `$HOME` is shown
here as `/home/USER`):

```yaml
CompileFlags:
  Add:
    - --target=x86_64-pc-windows-msvc
    - -fms-compatibility
    - -fms-extensions
    - -imsvc/home/USER/msvc/vc/tools/msvc/14.52.37025/include
    - -imsvc/home/USER/msvc/kits/10/Include/10.0.26100.0/ucrt
    - -imsvc/home/USER/msvc/kits/10/Include/10.0.26100.0/shared
    - -imsvc/home/USER/msvc/kits/10/Include/10.0.26100.0/um
    - -imsvc/home/USER/msvc/kits/10/Include/10.0.26100.0/winrt
  Remove: [/WX, -Werror]      # the build uses /WX, don't show every warning as an error
```

Double-check the `shared` directory spelling. A typo there produces
`'winapifamily.h' file not found` and breaks autocomplete for everything in
`windows.h`.

If `<print>` or other newer STL features show false errors, clangd is pretending to be
an older MSVC. Add `- -fms-compatibility-version=19.52` (or `19.50`).

### Neovim

Point clangd at the 22 binary. With Mason, a plain `clangd` may resolve to Mason's
copy, so name the binary explicitly:

```lua
vim.lsp.config('clangd', {
  cmd = { 'clangd-22', '--background-index', '--clang-tidy', '--header-insertion=iwyu' },
})
```

Then `:LspRestart` and check `:LspInfo` (root dir should be the project directory).

### Debugging the LSP setup

```bash
clangd-22 --check=src/main.cpp
```

Look at the `E[...]` lines. `file not found` means a wrong include path; the compile
command it prints shows exactly which flags clangd is using.

## 6. Spawning GUI windows from the headless server

A Windows GUI program under Wine still needs an X server to draw on. A headless server
has none, which gives this error:

```
Application tried to create a window, but no driver could be loaded.
```

You have two ways to supply a display: show the window on your desktop (X11
forwarding), or give the server a virtual display and look at it remotely (Xvfb + VNC).

### A minimal Win32 window to test with

```cpp
#include <windows.h>

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
        case WM_DESTROY: PostQuitMessage(0); return 0;
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC dc = BeginPaint(hwnd, &ps);
            TextOutW(dc, 20, 20, L"Hello from Wine", 15);
            EndPaint(hwnd, &ps);
            return 0;
        }
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

int WINAPI wWinMain(HINSTANCE inst, HINSTANCE, PWSTR, int show) {
    WNDCLASSW wc{};
    wc.lpfnWndProc   = WndProc;
    wc.hInstance     = inst;
    wc.lpszClassName = L"TestWindow";
    wc.hCursor       = LoadCursorW(nullptr, IDC_ARROW);
    RegisterClassW(&wc);

    HWND hwnd = CreateWindowExW(0, wc.lpszClassName, L"Test", WS_OVERLAPPEDWINDOW,
                                CW_USEDEFAULT, CW_USEDEFAULT, 640, 360,
                                nullptr, nullptr, inst, nullptr);
    ShowWindow(hwnd, show);

    MSG m;
    while (GetMessageW(&m, nullptr, 0, 0) > 0) {
        TranslateMessage(&m);
        DispatchMessageW(&m);
    }
    return 0;
}
```

In `CMakeLists.txt`, add the `WIN32` keyword so the linker uses the GUI subsystem
(entry point `wWinMain` instead of `main`):

```cmake
add_executable(game WIN32 main.cpp)
```

If you want to keep `main()` as the entry point, use the console subsystem (without
`WIN32`) and call `CreateWindowExW` from there; this also works and keeps console output.

### Option A: X11 forwarding over SSH (window appears on your desktop)

**On the server**

```bash
sudo apt install xauth x11-apps       # x11-apps only for the xeyes test
```

Check `/etc/ssh/sshd_config` (Ubuntu's defaults normally already work):

```
X11Forwarding yes
X11UseLocalhost yes
```

Restart if you changed anything: `sudo systemctl restart ssh`.

**On your desktop**

```bash
ssh -X user@$machine_hostname          # use -Y if -X gives "untrusted" errors
echo $DISPLAY                  # should print something like localhost:10.0
xeyes                          # a window should appear on your desktop
/usr/lib/wine/wine64 ./build/src/game.exe
```

To make it permanent, add this to `~/.ssh/config` on the desktop:

```
Host $machine_hostname
    ForwardX11 yes
    ForwardX11Trusted yes
    Compression yes
```

Notes:

- The desktop needs an X server. On a Wayland desktop (GNOME, KDE, sway, ...), `ssh -X`
  works through **XWayland**, which must be enabled. If `xeyes` fails with
  `Can't open display`, XWayland is the first thing to check.
- `DISPLAY` goes stale inside **tmux** or **screen** sessions that outlive an SSH
  connection. After reconnecting, set it again inside the multiplexer
  (`export DISPLAY=localhost:10.0`, using the value from a fresh `echo $DISPLAY` outside
  tmux).
- Closing the SSH session kills the forwarded windows.
- Forwarding is fine for plain Win32/GDI windows and tools. It is slow for games, and
  OpenGL goes through indirect GLX, which is limited to old OpenGL versions. Direct3D
  and Vulkan will not work in practice. Use Option B for anything graphical beyond that.
- First launch is slow because Wine creates its prefix (`~/.wine`).

### Option B: virtual display on the server, viewed over VNC

This keeps the app running on the server, independent of your SSH connection, and it
can use software rendering for OpenGL and Vulkan.

```bash
sudo apt install xvfb x11vnc mesa-utils mesa-vulkan-drivers libgl1-mesa-dri

Xvfb :99 -screen 0 1280x720x24 &
export DISPLAY=:99
/usr/lib/wine/wine64 ./build/src/game.exe &
x11vnc -display :99 -localhost -nopw -forever &
```

On your desktop, tunnel the VNC port through SSH and connect a VNC viewer to
`localhost:5900`:

```bash
ssh -L 5900:localhost:5900 user@$machine_hostname
```

`-nopw` is acceptable here only because x11vnc listens on `localhost` and the SSH tunnel
is the only way in. Do not expose port 5900 directly, and use `-passwd`/`-rfbauth` if
other people use the machine.

Software rendering (Mesa llvmpipe for OpenGL, lavapipe for Vulkan) works but is slow. For
Direct3D 9/10/11 games under Wine, `winetricks dxvk` plus `mesa-vulkan-drivers` lets them
render through lavapipe on a server without a GPU.

Alternatives to x11vnc with the same idea: `xpra` (better performance and clipboard),
or `tigervnc-standalone-server`.

### Option C: no display needed, just run it

For automated tests where you only need the program to start and exit, wrap it in a
throwaway display:

```bash
xvfb-run -a /usr/lib/wine/wine64 ./build/src/game.exe
```

To grab a screenshot of the virtual display from a script, use `import -window root
shot.png` (package `imagemagick`) while `DISPLAY` points at the Xvfb display.

### Option D: run on a real display

If graphics matter, build on the server and run on a machine with a screen: copy the exe
(`rsync -av build/src/game.exe desktop:`) and run it with Wine on that machine, or on
Windows itself.

## Troubleshooting checklist

- `Application tried to create a window, but no driver could be loaded`: `DISPLAY` is
  not set. Use X11 forwarding or Xvfb (section 6).
- `Can't open display` / `xeyes` fails over `ssh -X`: install `xauth` on the server,
  check `X11Forwarding yes`, and make sure XWayland runs on a Wayland desktop.
- Window opens but the game is unusably slow over `ssh -X`: switch to Xvfb + VNC.
- `cl` not found by CMake: `export PATH=~/msvc/bin/x64:$PATH` in that shell.
- Missing `*D.dll` at runtime: set `CMAKE_MSVC_RUNTIME_LIBRARY` (section 3).
- No autocomplete for Windows APIs: run `clangd-22 --check`, fix `-imsvc` paths.
- Bogus errors inside STL headers: clangd is too old, use 22 or newer.
- `CreateWindowExW` missing even though headers load: check you did not define `NOUSER`.

## License note

The downloaded MSVC toolchain and Windows SDK are under Microsoft's license terms,
which restrict where they can be used. Read them before using the output commercially.
