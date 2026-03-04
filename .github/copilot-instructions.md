# Project Guidelines

## Code Style
- Use C++17 and existing Qt style patterns from `timeline/` and `main_window.cpp`.
- Keep class/type names in `CamelCase`, file names in `snake_case`, and members with trailing underscore (for example `timeline_widget_`, `rootNode_`).
- Prefer early-return guards for invalid/null inputs (see `timeline/timeline_node.cpp`, `timeline/timeline_view.cpp`).
- Keep constants as `constexpr`/`static constexpr` where possible (see `timeline/timeline_view.cpp`).
- Follow current ownership patterns: `std::unique_ptr` at app shell boundaries and raw-pointer tree ownership inside `TimelineNode`.

## Architecture
- `main.cpp` + `main_window.*` are app shell/demo wiring only.
- Timeline functionality lives in `timeline/` as static library target `timeline_widgets`.
- Data model flow:
  - `TimelineNode` / `TimelineEvent` store timeline data.
  - `TimelineNodeModel` (`QAbstractItemModel`) exposes hierarchical rows.
  - `TimelineView` composes `QTreeView`, `TimelineWidget`, and synchronized scrollbars.
  - `TimelineWidget` renders rows/events and interaction using Qwt/OpenGL canvas.
- Keep timeline behavior changes inside `timeline/` unless UI shell wiring is explicitly requested.

## Project Conventions
- App target/binary name is `qwt-timeline` (note spelling). Do not rename unless explicitly requested.
- `timeline/qwt` is vendored and built during CMake via `qmake` + `make` from `timeline/CMakeLists.txt`.
- `TIMELINE_QWT_QMAKE` defaults to `/home/meng/Qt/6.9.3/gcc_64/bin/qmake`; override when using a different Qt install.
- Preserve model/view synchronization behavior in `TimelineView` (tree scroll ↔ timeline draw updates).

## Integration Points
- Qt dependencies: `Widgets`, `OpenGLWidgets` (root `CMakeLists.txt`).
- Qwt integration: includes from `timeline/qwt/src`, library from local `qwt-build/lib`.
- Rendering backend path uses `QwtPlotOpenGLCanvas` in `timeline/timeline_widget.cpp`.

## Security
- None specific discovered in this workspace (desktop demo app; no auth/network/secret handling found).