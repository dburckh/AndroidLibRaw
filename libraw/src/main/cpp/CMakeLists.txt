# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html

# Sets the minimum version of CMake required to build the native library.

cmake_minimum_required(VERSION 3.10.2)

# Declares and names the project.
project("androidlibraw")

# Creates and names a library, sets it as either STATIC
# or SHARED, and provides the relative paths to its source code.
# You can define multiple libraries, and CMake builds them for you.
# Gradle automatically packages shared libraries with your APK.

add_library( # Sets the name of the library.
            androidraw

             # Sets the library as a shared library.
             SHARED

             # Provides a relative path to your source file(s).
        anrdroidraw.cpp LibRaw_fd_datastream.cpp AndroidLibRaw.h AndroidLibRaw.cpp)

# Searches for a specified prebuilt library and stores the path as a
# variable. Because CMake includes system libraries in the search path by
# default, you only need to specify the name of the public NDK library
# you want to add. CMake verifies that the library exists before
# completing its build.

find_library( # Sets the name of the path variable.
              log-lib

              # Specifies the name of the NDK library that
              # you want CMake to locate.
              log )
set_target_properties(androidraw PROPERTIES COMPILE_OPTIONS -fopenmp)

# Specifies libraries CMake should link to your target library. You
# can link multiple libraries, such as libraries you define in this
# build script, prebuilt third-party libraries, or system libraries.
target_link_libraries( # Specifies the target library.
                       androidraw
                       -landroid
                       -ljnigraphics
# this is broken see below
#        -fopenmp
                       # Links the target library to the log library
                       # included in the NDK.
                       ${log-lib} )
add_subdirectory(LibRaw-cmake)
# You can manually include openMP by renameing jniLibsBak to jniLibs and commenting the entry below.
# I noticed not performance increase, so I disabled it to reduce APK size
# Also, the openMP build is broken, so it seems like it's fate.
set(ENABLE_OPENMP OFF CACHE BOOL "DBB")
# Disable Jasper (JPEG 2000)
set(ENABLE_JASPER OFF CACHE BOOL "DBB")

add_subdirectory(androidraw26)

target_link_libraries(androidraw libraw::libraw)

