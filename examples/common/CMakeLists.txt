# select the sokol-gfx backend depending on platform
if(FIPS_EMSCRIPTEN)
    add_definitions(-DSOKOL_GLES3)
    set(slang "glsl300es")
elseif(FIPS_ANDROID)
    add_definitions(-DSOKOL_GLES3)
    set(slang "glsl300es")
elseif(FIPS_WINDOWS)
    add_definitions(-DSOKOL_D3D11)
    set(slang "hlsl4")
elseif(FIPS_OSX)
    add_definitions(-DSOKOL_METAL)
    if (FIPS_IOS)
        set(slang "metal_ios")
    else()
        set(slang "metal_macos")
    endif()
else()
    add_definitions(-DSOKOL_GLCORE)
    if (FIPS_IOS)
        set(slang "glsl300es")
    else()
        set(slang "glsl410")
    endif()
endif()

if (FIPS_WINDOWS)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif()

# a lib of common code shared between all emulators
fips_begin_lib(common)
    fips_files(
        common.h
        clock.c clock.h
        fs.c fs.h
        gfx.c gfx.h
        keybuf.c keybuf.h
        prof.c prof.h
        webapi.c webapi.h)
    sokol_shader(shaders.glsl ${slang})
    if (FIPS_OSX)
        fips_files(sokol.m)
        fips_frameworks_osx(Foundation)
        if (FIPS_IOS)
            fips_frameworks_osx(UIKit Metal MetalKit AudioToolbox AVFoundation)
        else()
            fips_frameworks_osx(Cocoa QuartzCore Metal MetalKit AudioToolbox)
        endif()
    else()
        fips_files(sokol.c)
        if (FIPS_ANDROID)
            fips_libs(GLESv3 EGL OpenSLES android log)
        elseif (FIPS_LINUX)
            fips_libs(X11 Xcursor Xi GL m dl asound)
        endif()
    endif()
fips_end_lib()

# optional UI library (using Dear ImGui)
fips_begin_lib(ui)
    fips_files(ui.cc ui.h)
    fips_deps(imgui-docking common)
fips_end_lib()

# a separate library with just keybuf (for the ASCII emulators)
fips_begin_lib(keybuf)
    fips_files(keybuf.c keybuf.h)
fips_end_lib()

fips_begin_lib(webapi)
    fips_files(webapi.c webapi.h)
fips_end_lib()