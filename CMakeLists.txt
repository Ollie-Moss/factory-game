cmake_minimum_required(VERSION 3.10)
project(MyGameEngine)

set(CMAKE_CXX_STANDARD 17)

# Add source files
file(GLOB_RECURSE SRC_FILES src/*.cpp src/*.c)

# Create the executable
add_executable(MyGameEngine ${SRC_FILES})

# Include directories
target_include_directories(MyGameEngine PRIVATE
    include
    third_party/glfw/include
    third_party/freetype/include
)

# Link system OpenGL
find_package(OpenGL REQUIRED)

# Link libraries
target_link_libraries(MyGameEngine
    glfw
    freetype
    OpenGL::GL
)

# Optional: Platform-specific configs
if (WIN32)
    target_compile_definitions(MyGameEngine PRIVATE _CRT_SECURE_NO_WARNINGS)
elseif(APPLE)
    target_link_libraries(MyGameEngine "-framework Cocoa" "-framework OpenGL" "-framework IOKit")
endif()
