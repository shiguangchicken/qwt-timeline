---
name: build-run
description: build or run the project
---
# Build commands
- Configure + build (repo root):
```bash
cmake -S . -B ./build/Desktop_Qt_6_9_3-Debug -DCMAKE_BUILD_TYPE:STRING=Debug -DCMAKE_GENERATOR:STRING=Ninja -DCMAKE_COLOR_DIAGNOSTICS:BOOL=ON -DCMAKE_PREFIX_PATH:PATH=/home/meng/Qt/6.9.3/gcc_64 && cmake --build ./build/Desktop_Qt_6_9_3-Debug/
```
# Run commands

```bash
./build/Desktop_Qt_6_9_3-Debug/ai-timline
```