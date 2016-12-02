cmake_minimum_required (VERSION 2.6)
set (CMAKE_BACKWARDS_COMPATIBILITY 2.6)

set (PROJECT_NAME fbrapp)

Project(${PROJECT_NAME})

if (WIN32)
    set (APP_PLATFORM_NAME "Win")
    if (FB_OPTIMIZE STREQUAL "size")
        set (FB_OPT_PARAM "/O1 /Ob2 /Os /Oy")
        message(STATUS "Optmizing for size")
    elseif (FB_OPTIMIZE STREQUAL "speed")
        set (FB_OPT_PARAM "/O2 /Ot")
        message(STATUS "Optmizing for speed")
    else()
        set (FB_OPT_PARAM "/Ox")
        message(STATUS "Balanced size/speed optimization")
    endif()

    set(CMAKE_C_FLAGS                            "/GF /DWIN32 /D_WINDLL /DXP_WIN=1 /W3 /wd4996 /nologo /D UNICODE /D _UNICODE /D _WINDOWS /Zm256 /Y- /DBJN_SKINNY_CLIENT=1 /DENABLE_GD /DENABLE_PCAP_READER")
    set(CMAKE_CXX_FLAGS                          "/GF /D__STDC_CONSTANT_MACROS /D__STDC_LIMIT_MACROS /DWIN32 /D_WINDLL /DXP_WIN=1 /W3 /wd4996 /nologo /EHsc /wd4290 /D UNICODE /D _UNICODE /D _WINDOWS /Zm256 /DBJN_SKINNY_CLIENT=1 /DENABLE_GD /DENABLE_PCAP_READER")
    set(CMAKE_C_FLAGS_RELEASE                    "/GL /MT ${FB_OPT_PARAM} /DNDEBUG")
    set(CMAKE_CXX_FLAGS_RELEASE                  "/GL /MT ${FB_OPT_PARAM} /DNDEBUG")
    # x64 does not support edit-and-continue.
    set(CMAKE_C_FLAGS_DEBUG                      "/MTd /Od /DDEBUG /D_DEBUG /Zi /RTC1 /Gm") 
    set(CMAKE_CXX_FLAGS_DEBUG                    "/MTd /Od /DDEBUG /D_DEBUG /Zi /RTC1 /Gm")
    set(CMAKE_C_FLAGS_MINSIZEREL                 "/GL /MT /O1 /Os /DNDEBUG")
    set(CMAKE_CXX_FLAGS_MINSIZEREL               "/GL /MT /O1 /Os /DNDEBUG")
    set(CMAKE_C_FLAGS_RELWITHDEBINFO             "/GL /MT ${FB_OPT_PARAM} /DNDEBUG /Zi")
    set(CMAKE_CXX_FLAGS_RELWITHDEBINFO           "/GL /MT ${FB_OPT_PARAM} /DNDEBUG /Zi")
    set(CMAKE_EXE_LINKER_FLAGS_DEBUG
        "${CMAKE_EXE_LINKER_FLAGS_DEBUG}         ")
    set(CMAKE_EXE_LINKER_FLAGS_RELEASE
        "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /LTCG ")
    set(CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO
        "${CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO} /LTCG")
    set(CMAKE_EXE_LINKER_FLAGS_MINSIZEREL
        "${CMAKE_EXE_LINKER_FLAGS_MINSIZEREL} /LTCG")
    set(CMAKE_SHARED_LINKER_FLAGS_DEBUG
        "${CMAKE_SHARED_LINKER_FLAGS_DEBUG}      /SUBSYSTEM:WINDOWS")
    set(CMAKE_SHARED_LINKER_FLAGS_RELEASE
        "${CMAKE_SHARED_LINKER_FLAGS_RELEASE} /LTCG   /SUBSYSTEM:WINDOWS /OPT:REF /OPT:ICF")
    set(CMAKE_SHARED_LINKER_FLAGS_RELWITHDEBINFO
        "${CMAKE_SHARED_LINKER_FLAGS_RELWITHDEBINFO} /LTCG /SUBSYSTEM:WINDOWS /OPT:REF /OPT:ICF")
    set(CMAKE_SHARED_LINKER_FLAGS_MINSIZEREL
        "${CMAKE_SHARED_LINKER_FLAGS_MINSIZEREL} /LTCG   /SUBSYSTEM:WINDOWS /OPT:REF /OPT:ICF")

    set(LINK_FLAGS "/LIBPATH:\"${ATL_LIBRARY_DIR}\"")

    if (WITH_DYNAMIC_MSVC_RUNTIME)
        message(STATUS "Building with dynamic MSVC runtime")
        foreach(flag_var
                CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE
                CMAKE_CXX_FLAGS_MINSIZEREL CMAKE_CXX_FLAGS_RELWITHDEBINFO
                CMAKE_C_FLAGS CMAKE_C_FLAGS_DEBUG CMAKE_C_FLAGS_RELEASE
                CMAKE_C_FLAGS_MINSIZEREL CMAKE_C_FLAGS_RELWITHDEBINFO
                )
            if(${flag_var} MATCHES "/MT")
                string(REGEX REPLACE "/MT" "/MD" ${flag_var} "${${flag_var}}")
            endif()
        endforeach(flag_var)
    endif()
    if ( NOT WITH_SYSTEM_BOOST )
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /D BOOST_ALL_NO_LIB=1")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /D BOOST_ALL_NO_LIB=1")
    endif ( NOT WITH_SYSTEM_BOOST )

endif()

# We define preprocessor flags here in addition to other flags
# because it is the most convenient place to put them to apply
# to all targets
if(UNIX)
    # XP_UNIX is used by the Gecko SDK
    set(gecko_defs "-DXP_UNIX")

    if(APPLE)
        set (APP_PLATFORM_NAME "Mac")
        file (GLOB PLATFORM RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
            Mac/[^.]*.cpp
            Mac/[^.]*.cc
            Mac/[^.]*.mm
            Mac/[^.]*.h
            Mac/[^.]*.cmake
        )
    endif()

    if(NOT APPLE)
        set (APP_PLATFORM_NAME "X11")
        # On 64 bit, -fPIC is required for compiling all objects which get linked
        # into a .so file.
        # CMake correctly defaults to -fPIC for shared libs, but static libs are
        # built without. Since we are linking static libs later into a shared lib
        # (as opposed to an executable), we need to build static libs with -fPIC
        # too.
        set(fPIC_flag "-fPIC")
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -DBJN_SKINNY_CLIENT=1 -DSSL_USE_OPENSSL -DPOSIX -DLINUX -DHAVE_LIBPULSE -Wno-write-strings -D__STDC_CONSTANT_MACROS -DENABLE_GD -DENABLE_PCAP_READER")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -DBJN_SKINNY_CLIENT=1 -DSSL_USE_OPENSSL -DPOSIX -DLINUX -DHAVE_LIBPULSE -Wno-write-strings -D__STDC_CONSTANT_MACROS -std=c++0x -DENABLE_GD -DENABLE_PCAP_READER")
    endif()

    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${gecko_defs} ${fPIC_flag}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${gecko_defs} ${fPIC_flag}")
    if (NOT WITH_SYSTEM_BOOST)
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DBOOST_ALL_NO_LIB=1 -DUNICODE -D_UNICODE -DBJN_SKINNY_CLIENT=1 -DENABLE_GD -DENABLE_PCAP_READER")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DBOOST_ALL_NO_LIB=1 -DUNICODE -D_UNICODE -DBJN_SKINNY_CLIENT=1 -DENABLE_GD -DENABLE_PCAP_READER")
    endif()

    set(CMAKE_C_FLAGS_RELWITHDEBINFO "${CMAKE_C_FLAGS_RELWITHDEBINFO} -DNDEBUG")
    set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} -DNDEBUG")
endif()

macro (include_platform)

    if (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${APP_PLATFORM_NAME}/projectDef.cmake)
        include(${CMAKE_CURRENT_SOURCE_DIR}/${APP_PLATFORM_NAME}/projectDef.cmake)
    else()
        message ("Could not find a ${APP_PLATFORM_NAME} directory for the current project")
    endif()

endmacro(include_platform)

set(BUILD_TYPE "Release")
if(DEFINED ENV{BUILD_TYPE})
set(BUILD_TYPE $ENV{BUILD_TYPE})
endif()

set(ARCH_TYPE "x86_64")
if(DEFINED ENV{ARCH_TYPE})
set(ARCH_TYPE $ENV{ARCH_TYPE})
endif()

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../curl/include/)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../bjnbuild/${ARCH_TYPE}/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../clientcore/callmanager/callmanagerbase)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../clientcore/callmanager/threadmanager/source/)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../webrtc/webrtc/voice_engine/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../webrtc/webrtc/system_wrappers/interface)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../webrtc/webrtc/video_engine/include/)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../webrtc/webrtc/modules/video_render/include/)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../webrtc/webrtc/)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../webrtc/)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../webrtc/webrtc/modules/interface/)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../pjproject/pjlib/include/)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../pjproject/pjlib-util/include/)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../pjproject/pjnath/include/)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../pjproject/pjmedia/include/)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../pjproject/pjsip/include/)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../clientcore/bjncodec/source/)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../libyuv/include/)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../bwmgr/src/)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../ffmpeg/)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../libvpx/)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../bjnscreencodec/bjnscreendecoder/inc)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../bjnscreencodec/bjnscreenencoder/inc)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../libyuv/include/)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../ext_libs/win32/openssl/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../bwmgr/src)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../bjnscreencodec/bjnscreendecoder/inc)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../bjnscreencodec/bjnscreenencoder/inc)

file (GLOB GENERAL RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    [^.]*.cpp
    [^.]*.cc
    [^.]*.mm
    [^.]*.h
    [^.]*.cmake
)

SOURCE_GROUP(${PROJECT_NAME} FILES ${GENERAL})
set (SOURCES
    ${GENERAL}
    ${PLATFORM}
    )

add_executable(fbrapp ${SOURCES})

set (BJNBUILD_LIBRARY_PATH ${CMAKE_CURRENT_SOURCE_DIR}/..//bjnbuild/${ARCH_TYPE}/lib)
set (X264_LIBRARY_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../x264)
set (JINGLE_LIBRARY_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../webrtc/xcodebuild/${BUILD_TYPE})
set (WEBRTC_LIBRARY_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../webrtc/xcodebuild/${BUILD_TYPE})
set (PJSIP_LIBRARY_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../pjproject/)
set (FIBERCC_LIBRARY_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../clientcore/libfiber/${BUILD_TYPE}/)
set (OPENSSL_LIBRARY_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../openssl/)

target_link_libraries(${PROJECT_NAME} ${FIBERCC_LIBRARY_PATH}libfiber.a)
target_link_libraries(${PROJECT_NAME} ${WEBRTC_LIBRARY_PATH}/libvideo_capture_module.a)
target_link_libraries(${PROJECT_NAME} ${WEBRTC_LIBRARY_PATH}/libcommon_video.a)
target_link_libraries(${PROJECT_NAME} ${WEBRTC_LIBRARY_PATH}/libsystem_wrappers.a)
target_link_libraries(${PROJECT_NAME} ${WEBRTC_LIBRARY_PATH}/libwebrtc_utility.a)
target_link_libraries(${PROJECT_NAME} ${WEBRTC_LIBRARY_PATH}/libaudio_coding_module.a)
target_link_libraries(${PROJECT_NAME} ${WEBRTC_LIBRARY_PATH}/libCNG.a)
target_link_libraries(${PROJECT_NAME} ${WEBRTC_LIBRARY_PATH}/libaudio_processing.a)
target_link_libraries(${PROJECT_NAME} ${WEBRTC_LIBRARY_PATH}/libaudio_processing_sse2.a)
target_link_libraries(${PROJECT_NAME} ${WEBRTC_LIBRARY_PATH}/libG711.a)
target_link_libraries(${PROJECT_NAME} ${WEBRTC_LIBRARY_PATH}/libwebrtc_opus.a)
target_link_libraries(${PROJECT_NAME} ${WEBRTC_LIBRARY_PATH}/libG722.a)
target_link_libraries(${PROJECT_NAME} ${WEBRTC_LIBRARY_PATH}/libiSAC.a)
target_link_libraries(${PROJECT_NAME} ${WEBRTC_LIBRARY_PATH}/libiLBC.a)
target_link_libraries(${PROJECT_NAME} ${WEBRTC_LIBRARY_PATH}/libiSACFix.a)
target_link_libraries(${PROJECT_NAME} ${WEBRTC_LIBRARY_PATH}/libPCM16B.a)
target_link_libraries(${PROJECT_NAME} ${WEBRTC_LIBRARY_PATH}/libNetEq.a)
target_link_libraries(${PROJECT_NAME} ${WEBRTC_LIBRARY_PATH}/libmedia_file.a)
target_link_libraries(${PROJECT_NAME} ${WEBRTC_LIBRARY_PATH}/librtp_rtcp.a)
target_link_libraries(${PROJECT_NAME} ${WEBRTC_LIBRARY_PATH}/libremote_bitrate_estimator.a)
target_link_libraries(${PROJECT_NAME} ${WEBRTC_LIBRARY_PATH}/libpaced_sender.a)
target_link_libraries(${PROJECT_NAME} ${WEBRTC_LIBRARY_PATH}/libNetEq4.a)
target_link_libraries(${PROJECT_NAME} ${WEBRTC_LIBRARY_PATH}/libvoice_engine.a)
target_link_libraries(${PROJECT_NAME} ${WEBRTC_LIBRARY_PATH}/libaudio_conference_mixer.a)
target_link_libraries(${PROJECT_NAME} ${WEBRTC_LIBRARY_PATH}/libaudio_device.a)
target_link_libraries(${PROJECT_NAME} ${WEBRTC_LIBRARY_PATH}/libcommon_audio.a)
target_link_libraries(${PROJECT_NAME} ${WEBRTC_LIBRARY_PATH}/libcommon_audio_sse2.a)
target_link_libraries(${PROJECT_NAME} ${WEBRTC_LIBRARY_PATH}/libaudioproc_debug_proto.a)
target_link_libraries(${PROJECT_NAME} ${WEBRTC_LIBRARY_PATH}/libprotobuf_lite.a)
target_link_libraries(${PROJECT_NAME} ${JINGLE_LIBRARY_PATH}/libjingle.a)
target_link_libraries(${PROJECT_NAME} ${PJSIP_LIBRARY_PATH}/pjlib/lib/libpj-${ARCH_TYPE}-apple-darwin.a)
target_link_libraries(${PROJECT_NAME} ${PJSIP_LIBRARY_PATH}/pjlib-util/lib/libpjlib-util-${ARCH_TYPE}-apple-darwin.a)
target_link_libraries(${PROJECT_NAME} ${PJSIP_LIBRARY_PATH}/pjmedia/lib/libpjmedia-audiodev-${ARCH_TYPE}-apple-darwin.a)
target_link_libraries(${PROJECT_NAME} ${PJSIP_LIBRARY_PATH}/pjmedia/lib/libpjmedia-codec-${ARCH_TYPE}-apple-darwin.a)
target_link_libraries(${PROJECT_NAME} ${PJSIP_LIBRARY_PATH}/pjmedia/lib/libpjmedia-${ARCH_TYPE}-apple-darwin.a)
target_link_libraries(${PROJECT_NAME} ${PJSIP_LIBRARY_PATH}/pjmedia/lib/libpjmedia-videodev-${ARCH_TYPE}-apple-darwin.a)
target_link_libraries(${PROJECT_NAME} ${PJSIP_LIBRARY_PATH}/pjmedia/lib/libpjsdp-${ARCH_TYPE}-apple-darwin.a)
target_link_libraries(${PROJECT_NAME} ${PJSIP_LIBRARY_PATH}/pjnath/lib/libpjnath-${ARCH_TYPE}-apple-darwin.a)
target_link_libraries(${PROJECT_NAME} ${PJSIP_LIBRARY_PATH}/pjsip/lib/libpjsip-${ARCH_TYPE}-apple-darwin.a)
target_link_libraries(${PROJECT_NAME} ${PJSIP_LIBRARY_PATH}/pjsip/lib/libpjsip-simple-${ARCH_TYPE}-apple-darwin.a)
target_link_libraries(${PROJECT_NAME} ${PJSIP_LIBRARY_PATH}/pjsip/lib/libpjsip-ua-${ARCH_TYPE}-apple-darwin.a)
target_link_libraries(${PROJECT_NAME} ${PJSIP_LIBRARY_PATH}/pjsip/lib/libpjsua-${ARCH_TYPE}-apple-darwin.a)
target_link_libraries(${PROJECT_NAME} ${PJSIP_LIBRARY_PATH}/third_party/lib/libg7221codec-${ARCH_TYPE}-apple-darwin.a)
target_link_libraries(${PROJECT_NAME} ${PJSIP_LIBRARY_PATH}/third_party/lib/libgsmcodec-${ARCH_TYPE}-apple-darwin.a)
target_link_libraries(${PROJECT_NAME} ${PJSIP_LIBRARY_PATH}/third_party/lib/libilbccodec-${ARCH_TYPE}-apple-darwin.a)
target_link_libraries(${PROJECT_NAME} ${PJSIP_LIBRARY_PATH}/third_party/lib/libmilenage-${ARCH_TYPE}-apple-darwin.a)
target_link_libraries(${PROJECT_NAME} ${PJSIP_LIBRARY_PATH}/third_party/lib/libportaudio-${ARCH_TYPE}-apple-darwin.a)
target_link_libraries(${PROJECT_NAME} ${PJSIP_LIBRARY_PATH}/third_party/lib/libresample-${ARCH_TYPE}-apple-darwin.a)
target_link_libraries(${PROJECT_NAME} ${PJSIP_LIBRARY_PATH}/third_party/lib/libspeex-${ARCH_TYPE}-apple-darwin.a)
target_link_libraries(${PROJECT_NAME} ${PJSIP_LIBRARY_PATH}/third_party/lib/libsrtp-${ARCH_TYPE}-apple-darwin.a)
target_link_libraries(${PROJECT_NAME} ${BJNBUILD_LIBRARY_PATH}/libyuv.a)
target_link_libraries(${PROJECT_NAME} ${BJNBUILD_LIBRARY_PATH}/libbwmgr.a)
target_link_libraries(${PROJECT_NAME} ${BJNBUILD_LIBRARY_PATH}/libvpx.a)
target_link_libraries(${PROJECT_NAME} ${BJNBUILD_LIBRARY_PATH}/libjpeg.a)
target_link_libraries(${PROJECT_NAME} ${BJNBUILD_LIBRARY_PATH}/libz.a)
target_link_libraries(${PROJECT_NAME} ${BJNBUILD_LIBRARY_PATH}/libbjnscr.a)
if(ENABLE_H264_FFMPEG)
target_link_libraries(${PROJECT_NAME} ${X264_LIBRARY_PATH}/libx264.a)
target_link_libraries(${PROJECT_NAME} ${PLUGIN_INTERNAL_DEPS} ${BJNBUILD_LIBRARY_PATH}/libavcodec.dylib)
target_link_libraries(${PROJECT_NAME} ${PLUGIN_INTERNAL_DEPS} ${BJNBUILD_LIBRARY_PATH}/libavformat.dylib)
target_link_libraries(${PROJECT_NAME} ${PLUGIN_INTERNAL_DEPS} ${BJNBUILD_LIBRARY_PATH}/libavutil.dylib)
endif(ENABLE_H264_FFMPEG)
target_link_libraries(${PROJECT_NAME} ${BJNBUILD_LIBRARY_PATH}/libcurl.a)
target_link_libraries(${PROJECT_NAME} ${BJNBUILD_LIBRARY_PATH}/libssl.a)
target_link_libraries(${PROJECT_NAME} ${BJNBUILD_LIBRARY_PATH}/libcrypto.a)
target_link_libraries(${PROJECT_NAME} ${BJNBUILD_LIBRARY_PATH}/libopus.a)
target_link_libraries(${PROJECT_NAME} /usr/local/lib/libboost_thread-mt.a)
target_link_libraries(${PROJECT_NAME} /usr/local/lib/libboost_filesystem-mt.a)
target_link_libraries(${PROJECT_NAME} /usr/local/lib/libboost_system-mt.a)
#target_link_libraries(${PROJECT_NAME} /Users/uchandak/work/skinny/thirdparty/firebreath/build/boost/libs/thread/Debug/libboost_thread.a)
#target_link_libraries(${PROJECT_NAME} /Users/uchandak/work/skinny/thirdparty/firebreath/build/boost/libs/filesystem/Debug/libboost_filesystem.a)
#target_link_libraries(${PROJECT_NAME} /Users/uchandak/work/skinny/thirdparty/firebreath/build/boost/libs/system/Debug/libboost_system.a)
target_link_libraries(${PROJECT_NAME} c++ resolv)

find_library(AUDIOTOOLBOX_FRAMEWORK AudioToolBox )
find_library(AUDIOUNIT_FRAMEWORK AudioUnit )
find_library(COREAUDIO_FRAMEWORK CoreAudio )
find_library(COREFOUNDATION_FRAMEWORK CoreFoundation )
find_library(FOUNDATION_FRAMEWORK Foundation )
find_library(IOKIT_FRAMEWORK IOKit )
find_library(QTKIT_FRAMEWORK QTKit)
find_library(QUICKTIME_FRAMEWORK QuickTime )
find_library(COREVIDEO_FRAMEWORK CoreVideo )
find_library(OPENGL_FRAMEWORK OpenGL )
find_library(AGL_FRAMEWORK AGL )
find_library(CARBON_FRAMEWORK Carbon )
find_library(COCOA_FRAMEWORK Cocoa )
find_library(APP_FRAMEWORK ApplicationServices )
find_library(CORESERVICES_FRAMEWORK CoreServices )
find_library(SECURITY_FRAMEWORK Security)
find_library(COREWLAN_FRAMEWORK CoreWLAN)
find_library(COREMEDIA_FRAMEWORK CoreMedia )
find_library(SYSTEMCONFIG_FRAMEWORK SystemConfiguration)

set (APP_INTERNAL_DEPS
     ${AUDIOTOOLBOX_FRAMEWORK}
     ${AUDIOUNIT_FRAMEWORK}
     ${COREAUDIO_FRAMEWORK}
     ${COREFOUNDATION_FRAMEWORK}
     ${FOUNDATION_FRAMEWORK}
     ${IOKIT_FRAMEWORK}
     ${QTKIT_FRAMEWORK}
     ${QUITCKTIME_FRAMEWORK}
     ${COREVIDEO_FRAMEWORK}
     ${OPENGL_FRAMEWORK}
     ${AGL_FRAMEWORK}
     ${APP_FRAMEWORK}
     ${CORESERVICES_FRAMEWORK}
     ${CARBON_FRAMEWORK}
     ${COCOA_FRAMEWORK}
     ${SECURITY_FRAMEWORK}
     ${COREWLAN_FRAMEWORK}
     ${COREMEDIA_FRAMEWORK}
     ${SYSTEMCONFIG_FRAMEWORK})

set_target_properties(${PROJECT_NAME}
    PROPERTIES LINK_FLAGS "-weak_framework AVFoundation -weak_framework VideoToolBox")

target_link_libraries(${PROJECT_NAME}
    ${APP_INTERNAL_DEPS}
    )

include_platform()

set_target_properties(${PROJECT_NAME} PROPERTIES FOLDER "Simple Fiber App")
