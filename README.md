# winduino

![winduino](https://github.com/codewitch-honey-crisis/winduino/assets/16417109/37ba76c2-b248-4aff-8fd1-795c4f443b19)

A small bridge for Arduino applications to allow them to be roughly prototyped on a
Windows PC using UIX or DirectX for the display.

A rough port of the UIX benchmark example is included

I have not tested with LVGL yet, but it should work.

To build the project you'll need mingw. I use GCC 12 or above but earlier versions *might* choke.

1. Make a "lib" folder in the project directory.
2. Run fetch_deps.cmd to get all of the dependencies.
3. Use the CMake VS Code extension to Build All Projects on the root CMakeLists.txt
4. Use the ./run.cmd to run the project

