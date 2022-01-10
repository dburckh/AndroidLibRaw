# Android LibRaw
Another Android LibRaw implementation.  LibRaw is used to render camera raw image files (e.g Nikon NEF, Canon CR2, Sony ARW).   The project contains a sample app that shows a simple implementation.

## Using the artifact
On the surface, this is as easy as:

`implementation 'com.homesoft.android:libraw:2.0.2'`

Unfortunately, you also need to add GitHub Packages to your base project build.gradle, which is kind of a pain.  You'll need to add this to your root project build.gradle.
```groovy
allprojects {
    repositories {
        ...
        maven {
            url = 'https://maven.pkg.github.com/dburckh/AndroidLibRaw'
            credentials {
                username = System.getenv("GPR_USER")
                //This password expires, so it will need to updated in environment
                password = System.getenv("GPR_API_KEY")
            }
        }
    }
}
```
You'll also need to set your GitHub Id into an environment variable called GPR_USER and put your GitHub token in GPR_API_KEY

Might just be easier to download it.  I won't judge.  :)
[Link](https://github.com/dburckh/AndroidLibRaw/packages/1172747)

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
2.0.2
- Fix rotation issues
- Add cancel functions

2.0.1
- Change getBitmap\[16\]() to remove intermediate step that creates an RGB memory image.  Now directly creates the Bitmap from the image\[\] data.
- Add methods setCaptureScaleMul(), getColorCurve() and dcrawProcessForce(colorCurve) to force consistent white balance in future runs.

2.0.0
- Moved from static LibRaw to instance.  This allows multiple LibRaw instances to exist at the same time.
