# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html

# Sets the minimum version of CMake required to build the native library.

cmake_minimum_required(VERSION 3.4.1)

# Creates and names a library, sets it as either STATIC
# or SHARED, and provides the relative paths to its source code.
# You can define multiple libraries, and CMake builds them for you.
# Gradle automatically packages shared libraries with your APK.

add_definitions(-Oz)

add_definitions(-fno-rtti)
add_definitions(-fno-exceptions)

include_directories(src/main/cpp/xhook)

add_library( # Sets the name of the library.
        pudge
        # Sets the library as a shared library.
        SHARED
        # Provides a relative path to your source file(s).
        IMPORTED)

set_target_properties(
        pudge
        PROPERTIES IMPORTED_LOCATION
        ${PROJECT_SOURCE_DIR}/libs/${ANDROID_ABI}/libpudge.so)



add_library( # Sets the name of the library.
             doom

             # Sets the library as a shared library.
             SHARED

             # Provides a relative path to your source file(s).
             src/main/cpp/doom.cpp
             src/main/cpp/doom_dalvik.cpp
             src/main/cpp/doom_marshmallow.cpp
             src/main/cpp/doom_nougat.cpp
             src/main/cpp/doom_oreo.cpp
             src/main/cpp/verify.cpp
             src/main/cpp/checkjni.cpp
             src/main/cpp/webview.cpp
             )

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

# Specifies libraries CMake should link to your target library. You
# can link multiple libraries, such as libraries you define in this
# build script, prebuilt third-party libraries, or system libraries.

target_link_libraries( # Specifies the target library.
                       doom
                       pudge
                       # Links the target library to the log library
                       # included in the NDK.
                       ${log-lib} )