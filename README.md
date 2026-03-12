# Raylib Template
A **decent** template for Raylib projects. This template was specifically made for games, but can be used for regular applications as well with some small tweaks.

## Requirements
First you need to have these dependencies installed:
- make
- C/C++ compiler of your choice
- Emscripten (Optional for WASM only)
- Python (Optional)

## Usage
Use the make command to build. The default target is linux.
```bash
make
```

If you want to explicitly set the target platform, use the `PLATFORM` flag.
```bash
make PLATFORM=LINUX
make PLATFORM=WINDOWS # x64 Windows binary
make PLATFORM=WEB
```

## Web Setup
In order to build to the web, you need to clone and install [emscripten](https://github.com/emscripten-core/emscripten) and build it on your computer only then can you build to the web.

When done building run:
```bash
python -m http.server PORT
```
And then open the .html file in `http://localhost:PORT` 

Alternatively, you can run 

```bash
emrun OuputFile.html
```

There is also some web stuff in the main.cpp file. If you don't need this just remove it.

> [!NOTE]
> Stronger support will be added for windows in the future.

Enjoy!
