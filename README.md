# AndroidLibRaw
Another Android LibRaw implementation.  LibRaw is used to render camera raw image files (e.g Nikon NEF, Canon CR2, Sony ARW).   The project contains a sample app that shows a simple implementation.

## Cloning the Project
Because the project has submodule links, it requires an extra parameter.

`git clone --recurse-submodules https://github.com/dburckh/AndroidLibRaw.git`

A note on submodules:

If you want to new version of the LibRaw or LibRawCMake, you can update the submodules.  Use caution updating the actual LibRaw/LibRawCMake code, updating submodule code can be tricky.

## Building the libraw.aar
No pre-builts just yet, but you can build the aar from the command line:

`gradlew libraw::assemble`

It usually ends up under ./libraw/build/outputs/aar

## Benefits
- Works with modern Android File security constraints (uses FileDescriptors or File paths).
- Works with Kotlin or Java.
- Not a custom version of LibRaw!  Should work with future releases with little or no modification.
- Uses CMake build system.

## Limitations
- Requires API 24+ (Nougat).
- Uses NDK 22, (21 is the current default).
- Uses Git Submodules.
- TODO: Pre-built artifacts.
- TODO: Unit tests

## Sources
Most of the code I did for this was either glue or tweaking of somebody else work.
- [LibRaw](https://github.com/LibRaw/LibRaw) The brains of the operation
- [LibRaw-cmake](https://github.com/LibRaw/LibRaw-cmake) Made building LibRaw in Studio/gradle, much easier.
- [LibRaw-Android](https://github.com/TSGames/Libraw-Android) Ground breaking "glue" code I borrowed heavily from.
