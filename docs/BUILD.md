# Build & Run

## Linux / WSL

### Dependencies (Ubuntu/Debian)

```bash
sudo apt update
sudo apt install -y g++ make \
  libglfw3-dev libglew-dev libglu1-mesa-dev mesa-common-dev
```

### Build

```bash
cd Breakout3D/Breakout3D
make -j
```

### Run

```bash
./breakout3d
```

### Debug build

```bash
make debug -j
./breakout3d_debug
```

## macOS

The `Makefile` links against `OpenGL` and CoreAudio frameworks and expects `GLEW` + `glfw` to be available.

```bash
cd Breakout3D/Breakout3D
make -j
./breakout3d
```


