# Android LibRaw
Another Android LibRaw implementation.  LibRaw is used to render camera raw image files (e.g Nikon NEF, Canon CR2, Sony ARW).   The project contains a sample app that shows a simple implementation.

## Using the artifact
Follow the instructions on [Jitpack.io](https://jitpack.io/) to include it as a repository.

Add this line to your build.gradle.

`implementation 'com.github.dburckh:AndroidLibRaw:2.0.5'`

## Cloning the Project
Because the project has submodule links, it requires an extra parameter.

`git clone --recurse-submodules https://github.com/dburckh/AndroidLibRaw.git`

A note on submodules:

If you want to new version of the LibRaw or LibRawCMake, you can update the submodules.  Use caution updating the actual LibRaw/LibRawCMake code, updating submodule code can be tricky.

## Benefits
- Decodes the actual raw data.  BitmapFactory shows the embedded JPEG, if available
- Works with modern Android File security constraints (uses FileDescriptors or File paths).
- Works with Kotlin or Java.
- Not a custom version of LibRaw!  Should work with future releases with little or no modification.
- Uses CMake build system.

## Limitations
- Requires API 24+ (Nougat).
- Uses NDK 22, (21 is the current default).
- Uses Git Submodules.
- TODO: Unit tests

## Sources
Most of the code I did for this was either glue or tweaking of somebody else work.
- [LibRaw](https://github.com/LibRaw/LibRaw) The brains of the operation
- [LibRaw-cmake](https://github.com/LibRaw/LibRaw-cmake) Made building LibRaw in Studio/gradle, much easier.
- [LibRaw-Android](https://github.com/TSGames/Libraw-Android) Ground breaking "glue" code I borrowed heavily from.

### Change Log
2.0.7
- Update Libraw to 0.21.2
- 16K Page Support

2.0.6
- Faster image processing
- Extend ColorSpace support
- White Balance coeffients by Exif.LightSource and Temperature
- Update Libraw to 0.21.2
- getBitmap() rework

2.0.5
- Update Libraw to 0.21.1
- Migrate build to Jitpack.io

2.0.4
- Add Proguard rules
- Update Libraw/Libraw-CMake

2.0.3
- Removed recycle from getBitmap().  Let me know if you feel it's necessary
- Added LibRaw.newInstance() factory to get the proper version of LibRaw (LibRaw or LibRaw26)
- For Android Q+, now creates RGB_888 (not RGBA_8888) and RGBA_F16 Bitmap.Config.Hardware Bitmaps.  Create a new LibRaw() directly if you don't want Hardware bitmaps for some reason.

2.0.2
- Fix rotation issues
- Add cancel functions

2.0.1
- Change getBitmap\[16\]() to remove intermediate step that creates an RGB memory image.  Now directly creates the Bitmap from the image\[\] data.
- Add methods setCaptureScaleMul(), getColorCurve() and dcrawProcessForce(colorCurve) to force consistent white balance in future runs.

2.0.0
- Moved from static LibRaw to instance.  This allows multiple LibRaw instances to exist at the same time.
