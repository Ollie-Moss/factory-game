cmake_minimum_required(VERSION 3.10)
project(build.exec)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Add source files
file(GLOB_RECURSE SRC_FILES 
    src/*.cpp 
    src/*.c 
    third-party/imgui/*.cpp
    include/*.c 
    include/*.cpp
)

# Create the executable
add_executable(build.exec ${SRC_FILES})

# Include directories
target_include_directories(build.exec PRIVATE
    include
    third_party/glfw/include
    third-party/imgui
    third_party/freetype/include
)

# Link system OpenGL
find_package(OpenGL REQUIRED)

# Link libraries
target_link_libraries(build.exec
    glfw
    freetype
    OpenGL::GL
    X11 Xrandr pthread dl Xi Xxf86vm Xinerama Xcursor  # Needed for OpenGL/GLFW on Linux
)

# Optional: Platform-specific configs
if (WIN32)
    target_compile_definitions(build.exec PRIVATE _CRT_SECURE_NO_WARNINGS)
elseif(APPLE)
    target_link_libraries(build.exec "-framework Cocoa" "-framework OpenGL" "-framework IOKit")
endif()
