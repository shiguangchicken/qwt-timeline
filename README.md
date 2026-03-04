# qwt-timeline

A Qt timeline demo app with two synchronized timeline views, tree-based lanes, and a custom Qwt/OpenGL timeline renderer.

## Features

- Two `TimelineView` widgets in one main window
- Hierarchical lanes (`CPU/Core0..10`, `GPU/Stream0..20`)
- Random demo events with colored blocks
- Event selection highlight and boundary markers
- Mouse tracker vertical line (throttled updates)
- Horizontal zoom/pan on time axis

## Project Structure

- `main.cpp` / `main_window.*`: app entry and demo UI setup
- `timeline/`: timeline library (`TimelineView`, `TimelineWidget`, model/node/event)
- `timeline/qwt/`: bundled Qwt source used for plotting

## Requirements

- Linux
- CMake >= 3.16
- C++17 compiler
- Qt (Widgets + OpenGLWidgets), tested with Qt 6.9.3
- `qmake` (used to build bundled Qwt)
- `make`/`gmake`

## Build

From repository root:

```bash
cmake -S . -B build/Desktop_Qt_6_9_3-Debug \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_GENERATOR=Ninja \
  -DCMAKE_COLOR_DIAGNOSTICS=ON \
  -DCMAKE_PREFIX_PATH=/home/meng/Qt/6.9.3/gcc_64 \
  -DTIMELINE_QWT_QMAKE=/home/meng/Qt/6.9.3/gcc_64/bin/qmake

cmake --build build/Desktop_Qt_6_9_3-Debug
```

## Run

```bash
./build/Desktop_Qt_6_9_3-Debug/qwt-timeline
```

## Controls

In `TimelineWidget`:

- `Wheel`: scroll vertical lanes (same behavior as tree view vertical scrolling)
- `Shift + Wheel`: horizontal timeline scroll (time range pan)
- `Ctrl + Wheel`: zoom in/out on x-axis (time axis)
- `Mouse Move`: show vertical tracker line
- `Left Click` on an event: highlight event and draw vertical lines at event start/end

## Notes

- The timeline library builds Qwt from `timeline/qwt` during CMake build.
- If your Qt installation path differs, update `CMAKE_PREFIX_PATH` and `TIMELINE_QWT_QMAKE` accordingly.
