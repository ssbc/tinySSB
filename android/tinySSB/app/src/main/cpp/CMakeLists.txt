
# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html

# Sets the minimum version of CMake required to build the native library.

cmake_minimum_required(VERSION 3.18.1)

# Declares and names the project.

project("codec2")

# --------------------

# Sets lib_src_DIR to the path of the target CMake project.
set( codec2_src_DIR ../codec2/ )

# Sets lib_build_DIR to the path of the desired output directory.
set( codec2_build_DIR ../codec2/ )
file(MAKE_DIRECTORY ${codec2_build_DIR})

add_subdirectory( ${codec2_src_DIR} ${codec2_build_DIR} )

include_directories(
            ${codec2_src_DIR}/src
            ${CMAKE_CURRENT_BINARY_DIR}/../codec2
)

# Creates and names a library, sets it as either STATIC
# or SHARED, and provides the relative paths to its source code.
# You can define multiple libraries, and CMake builds them for you.
# Gradle automatically packages shared libraries with your APK.

add_library( # Sets the name of the library.
             codec2lib

             # Sets the library as a shared library.
             STATIC

             # Provides a relative path to your source file(s).
             ../codec2/ )

# add_library( # Sets the name of the library.
#         libCodec2JNI
#         ## Sets the library as a shared library.
#         STATIC
#         ## Provides a relative path to your source file(s).
#         ../codec2/Codec2JNI.cpp)

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
        # libCodec2JNI
        codec2lib

                       # Links the target library to the log library
                       # included in the NDK.
                       ${log-lib} )

set_target_properties(codec2lib PROPERTIES LINKER_LANGUAGE CXX)
