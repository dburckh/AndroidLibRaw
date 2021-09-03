# AndroidLibRaw
Another Android LibRaw implementation.  LibRaw is used to render camera raw images files (e.g NEF, CR2, ARW).   Contains a sample app that show a simple implementation.

## Cloning the Project
Because the project has submodule links, it requires an extra parameter.

`git clone --recurse-submodules https://github.com/dburckh/AndroidLibRaw.git`

A note on submodules:

If you want to new version of the LibRaw or LibRawCMake, you can update the submodules.  Use caution updating the actual LibRaw/LibRawCMake code, updating submodule code can be tricky.

## Benefits
- Works with modern Android File security constraints (uses FileDescriptors or File paths).
- Works with Kotlin or Java.
- Not a custom version of LibRaw!  Should work with future releases with little or no modification.
- Uses CMake build system.

## Limitations
- Requires API 24+ (Nougat).
- Usage NDK 22, (21 is the current default).
- Uses Git Submodules.
- Currently no pre-built artifacts.  You have to copy the libraw module into your project.
- Currently no unit tests

## Sources
Most of the code I did for this was either glue or tweaking of somebody else work.
- [LibRaw](https://github.com/LibRaw/LibRaw) The brains of the operation
- [LibRaw-cmake](https://github.com/LibRaw/LibRaw-cmake) Made building LibRaw in Studio/gradle, much easier.
- [LibRaw-Android](https://github.com/TSGames/Libraw-Android) Ground breaking "glue" code I borrowed heavily from.
