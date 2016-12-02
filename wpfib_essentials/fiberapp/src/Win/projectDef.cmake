#/**********************************************************\ 
# Auto-generated Windows project definition file for the
# bjnplugin project
#\**********************************************************/

# Windows template platform definition CMake file
# Included from ../CMakeLists.txt

# remember that the current source dir is the project root; this file is in Win/
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DSSL_USE_SCHANNEL -DFEATURE_ENABLE_SSL=1 -DFEATURE_ENABLE_VOICEMAIL=1")

set(ENABLE_H264_FFMPEG 0)
if(DEFINED ENV{EnableH264Ffmpeg})
set(ENABLE_H264_FFMPEG $ENV{EnableH264Ffmpeg})
endif()

file (GLOB PLATFORM RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    Win/[^.]*.cpp
    Win/[^.]*.cc
    Win/[^.]*.rc
    Win/[^.]*.h
    Win/[^.]*.cmake
    Win/LeakFinder/*.cpp
    Win/LeakFinder/*.h
    )

file (GLOB LogitechSRK_Headers RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
	../LogitechSRK/LogiHID/common/hidcommon.h
	../LogitechSRK/LogiHID/common/LogiHID.h
 )
# use this to add preprocessor definitions
add_definitions(
    /D "_ATL_STATIC_REGISTRY"
)

if ("$ENV{ENABLE_PER_MACHINE_INSTALL}" STREQUAL "1")
    add_definitions(/DENABLE_PER_MACHINE_INSTALL)
endif()

SOURCE_GROUP(Win FILES ${PLATFORM})

SOURCE_GROUP(Win/LogitechSRK FILES ${LogitechSRK_Headers})
set (SOURCES
    ${SOURCES}
    ${PLATFORM}
    ${LogitechSRK_Headers}
    )

add_windows_plugin(${PROJECT_NAME} SOURCES)

# This is an example of how to add a build step to sign the plugin DLL before
# the WiX installer builds.  The first filename (certificate.pfx) should be
# the path to your pfx file.  If it requires a passphrase, the passphrase
# should be located inside the second file. If you don't need a passphrase
# then set the second filename to "".  If you don't want signtool to timestamp
# your DLL then make the last parameter "".
#
# Note that this will not attempt to sign if the certificate isn't there --
# that's so that you can have development machines without the cert and it'll
# still work. Your cert should only be on the build machine and shouldn't be in
# source control!
# -- uncomment lines below this to enable signing --
#firebreath_sign_plugin(${PROJECT_NAME}
#    "${CMAKE_CURRENT_SOURCE_DIR}/sign/certificate.pfx"
#    "${CMAKE_CURRENT_SOURCE_DIR}/sign/passphrase.txt"
#    "http://timestamp.verisign.com/scripts/timestamp.dll")

# add library dependencies here; leave ${PLUGIN_INTERNAL_DEPS} there unless you know what you're doing!

firebreath_sign_plugin(${PROJECT_NAME}
    "$ENV{SKINNY_CERTIFICATE}"
    "$ENV{SKINNY_PWD_FILE}"
    "$ENV{SHA1_TIMESTAMP_URL}"
    )

target_link_libraries(${PROJECT_NAME}
    ${PLUGIN_INTERNAL_DEPS}
    )

if("$ENV{WIN_ARCHTYPE}" STREQUAL "x64")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DWIN64")
    set (PLATFORM_BUILD_DIR x64/$ENV{MSBUILD_TYPE})
    set (PLATFORM_IPP_DIR intel64)
    set (PLATFORM_YUV_LIB_NAME libyuv_x64.lib)
    set (OPUS_LIB_DIR ${PLATFORM_BUILD_DIR})
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DWIN32")
    set (PLATFORM_BUILD_DIR $ENV{MSBUILD_TYPE})
    set (PLATFORM_IPP_DIR ia32)
    set (PLATFORM_YUV_LIB_NAME libyuv.lib)
    set (OPUS_LIB_DIR Win32/$ENV{MSBUILD_TYPE})
endif()

set (PLATFORM_JPEG_LIB_NAME jpeg-static.lib)
set (JINGLE_LIBRARY_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../thirdparty/fiber/webrtc/build/$ENV{MSBUILD_TYPE}/lib/)
set (WEBRTC_LIBRARY_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../thirdparty/fiber/webrtc/build/$ENV{MSBUILD_TYPE}/lib/)
set (OPUS_LIBRARY_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../thirdparty/fiber/opus/win32/VS2010/${OPUS_LIB_DIR}/)
set (PJSIP_LIBRARY_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../thirdparty/fiber/pjproject/)
set (SSL_LIBRARY_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../thirdparty/fiber/ext_libs/$ENV{MSVS_BUILD_PLATFORM}/openssl/lib/VC/static/)
set (EXT_LIBRARY_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../thirdparty/fiber/ext_libs/win32/)
set (EXT_MINGW_LIBRARY_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../thirdparty/fiber/ext_libs/win32/mingw/)
set (LIBYUV_LIBRARY_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../thirdparty/fiber/libyuv/build/$ENV{MSBUILD_TYPE}/lib/)
set (TURBOJPEG_LIBRARY_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../thirdparty/fiber/libyuv/third_party/libjpeg_turbo/$ENV{WEBRTCBUILDTYPE}/)
set (LIBVPX_LIBRARY_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../thirdparty/fiber/libvpx/out/$ENV{MSVS_BUILD_PLATFORM}/)
set (LIBMFX_LIBRARY_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../thirdparty/fiber/libmfx/lib/$ENV{MSVS_BUILD_PLATFORM}/)
set (FIBERCC_LIBRARY_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../thirdparty/fiber/clientcore/libfiber/$ENV{FBBUILDTYPE}/)
set (LIBCURL_LIBRARY_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../thirdparty/fiber/curl/lib/$ENV{MSBUILD_TYPE}/)
set (BWMGR_LIBRARY_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../thirdparty/fiber/bwmgr/${PLATFORM_BUILD_DIR}/)

set (USE_IPP_LIBRARIES 0)
if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set (USE_IPP_LIBRARIES 1)
	set (IPP_LIBRARY_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../thirdparty/fiber/ipp/win/lib/${PLATFORM_IPP_DIR}/)
    set (IPP_INCLUDE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../thirdparty/fiber/ipp/win/include/)
    include_directories(${IPP_INCLUDE_PATH})
endif(CMAKE_SYSTEM_NAME STREQUAL "Windows")

set (LIBCURL_INCLUDE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../thirdparty/fiber/curl/include/)
include_directories(${LIBCURL_INCLUDE_PATH})

set (LIBOPENSSL_INCLUDE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../thirdparty/fiber/ext_libs/win32/openssl/include/)
include_directories(${LIBOPENSSL_INCLUDE_PATH})

set (LIBMFX_INCLUDE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../thirdparty/fiber/libmfx/include/)
include_directories(${LIBMFX_INCLUDE_PATH})

set (LOGITECH_SRK_INCLUDE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../LogitechSRK)
set (LOGITECH_SRK_LIBRARY_PATH ${LOGITECH_SRK_INCLUDE_PATH}/lib/)
include_directories(${LOGITECH_SRK_INCLUDE_PATH}/LogiHID/common)
target_link_libraries(${PROJECT_NAME} ${OPUS_LIBRARY_PATH}celt.lib)
target_link_libraries(${PROJECT_NAME} ${OPUS_LIBRARY_PATH}opus.lib)
target_link_libraries(${PROJECT_NAME} ${OPUS_LIBRARY_PATH}silk_common.lib)
target_link_libraries(${PROJECT_NAME} ${OPUS_LIBRARY_PATH}silk_fixed.lib)
target_link_libraries(${PROJECT_NAME} ${OPUS_LIBRARY_PATH}silk_float.lib)

target_link_libraries(${PROJECT_NAME} ${WEBRTC_LIBRARY_PATH}directshow_baseclasses.lib)
target_link_libraries(${PROJECT_NAME} ${WEBRTC_LIBRARY_PATH}video_capture_module.lib)
target_link_libraries(${PROJECT_NAME} ${WEBRTC_LIBRARY_PATH}common_video.lib)
target_link_libraries(${PROJECT_NAME} ${WEBRTC_LIBRARY_PATH}system_wrappers.lib)
target_link_libraries(${PROJECT_NAME} ${WEBRTC_LIBRARY_PATH}webrtc_utility.lib)
target_link_libraries(${PROJECT_NAME} ${WEBRTC_LIBRARY_PATH}audio_coding_module.lib)
target_link_libraries(${PROJECT_NAME} ${WEBRTC_LIBRARY_PATH}CNG.lib)
target_link_libraries(${PROJECT_NAME} ${WEBRTC_LIBRARY_PATH}audio_processing.lib)
target_link_libraries(${PROJECT_NAME} ${WEBRTC_LIBRARY_PATH}audio_processing_sse2.lib)
target_link_libraries(${PROJECT_NAME} ${WEBRTC_LIBRARY_PATH}G711.lib)
target_link_libraries(${PROJECT_NAME} ${WEBRTC_LIBRARY_PATH}G722.lib)
target_link_libraries(${PROJECT_NAME} ${WEBRTC_LIBRARY_PATH}iLBC.lib)
target_link_libraries(${PROJECT_NAME} ${WEBRTC_LIBRARY_PATH}iSAC.lib)
target_link_libraries(${PROJECT_NAME} ${WEBRTC_LIBRARY_PATH}iSACFix.lib)
target_link_libraries(${PROJECT_NAME} ${WEBRTC_LIBRARY_PATH}webrtc_opus.lib)
target_link_libraries(${PROJECT_NAME} ${WEBRTC_LIBRARY_PATH}PCM16B.lib)
target_link_libraries(${PROJECT_NAME} ${WEBRTC_LIBRARY_PATH}NetEq.lib)
target_link_libraries(${PROJECT_NAME} ${WEBRTC_LIBRARY_PATH}media_file.lib)
target_link_libraries(${PROJECT_NAME} ${WEBRTC_LIBRARY_PATH}rtp_rtcp.lib)
target_link_libraries(${PROJECT_NAME} ${WEBRTC_LIBRARY_PATH}remote_bitrate_estimator.lib)
target_link_libraries(${PROJECT_NAME} ${WEBRTC_LIBRARY_PATH}/paced_sender.lib)
target_link_libraries(${PROJECT_NAME} ${WEBRTC_LIBRARY_PATH}/NetEq4.lib)
target_link_libraries(${PROJECT_NAME} ${WEBRTC_LIBRARY_PATH}voice_engine.lib)
target_link_libraries(${PROJECT_NAME} ${WEBRTC_LIBRARY_PATH}audio_conference_mixer.lib)
target_link_libraries(${PROJECT_NAME} ${WEBRTC_LIBRARY_PATH}audio_device.lib)
target_link_libraries(${PROJECT_NAME} ${WEBRTC_LIBRARY_PATH}common_audio.lib)
target_link_libraries(${PROJECT_NAME} ${WEBRTC_LIBRARY_PATH}common_audio_sse2.lib)
target_link_libraries(${PROJECT_NAME} ${WEBRTC_LIBRARY_PATH}audioproc_debug_proto.lib)
target_link_libraries(${PROJECT_NAME} ${WEBRTC_LIBRARY_PATH}protobuf_lite.lib)
target_link_libraries(${PROJECT_NAME} ${LOGITECH_SRK_INCLUDE_PATH}/lib/$ENV{MSVS_BUILD_PLATFORM}/hid.lib)
target_link_libraries(${PROJECT_NAME} ${LOGITECH_SRK_INCLUDE_PATH}/$ENV{MSBUILD_TYPE}/LogitechSRK.lib)

#if(USE_IPP_LIBRARIES EQUALS 1)
	target_link_libraries(${PROJECT_NAME} ${IPP_LIBRARY_PATH}ipps_l.lib)
	target_link_libraries(${PROJECT_NAME} ${IPP_LIBRARY_PATH}ippcore_l.lib)
#endif(USE_IPP_LIBRARIES EQUALS 1)

target_link_libraries(${PROJECT_NAME} ${JINGLE_LIBRARY_PATH}libjingle.lib)
target_link_libraries(${PROJECT_NAME} ${TURBOJPEG_LIBRARY_PATH}${PLATFORM_JPEG_LIB_NAME})
target_link_libraries(${PROJECT_NAME} ${LIBYUV_LIBRARY_PATH}${PLATFORM_YUV_LIB_NAME})
target_link_libraries(${PROJECT_NAME} ${LIBVPX_LIBRARY_PATH}Release/vpxmt.lib)
target_link_libraries(${PROJECT_NAME} ${LIBMFX_LIBRARY_PATH}/libmfx.lib)
if($ENV{MSBUILD_TYPE} STREQUAL "Debug")
    target_link_libraries(${PROJECT_NAME} ${EXT_LIBRARY_PATH}libzlibd.lib)
else()
    target_link_libraries(${PROJECT_NAME} ${EXT_LIBRARY_PATH}libzlib.lib)
endif()
target_link_libraries(${PROJECT_NAME} ${EXT_LIBRARY_PATH}libbjnscr.lib)
target_link_libraries(${PROJECT_NAME} ${EXT_LIBRARY_PATH}/libcurl/$ENV{MSBUILD_TYPE}/libcurl.lib)
target_link_libraries(${PROJECT_NAME} ${BWMGR_LIBRARY_PATH}bwmgr.lib)
target_link_libraries(${PROJECT_NAME} ${FIBERCC_LIBRARY_PATH}fiber.lib)
target_link_libraries(${PROJECT_NAME} powrprof.lib)
target_link_libraries(${PROJECT_NAME} strmiids.lib)
target_link_libraries(${PROJECT_NAME} winmm.lib)
target_link_libraries(${PROJECT_NAME} advapi32.lib)
target_link_libraries(${PROJECT_NAME} crypt32.lib)
target_link_libraries(${PROJECT_NAME} Pdh.lib)
target_link_libraries(${PROJECT_NAME} secur32.lib)
target_link_libraries(${PROJECT_NAME} shell32.lib)
target_link_libraries(${PROJECT_NAME} shlwapi.lib)
target_link_libraries(${PROJECT_NAME} user32.lib)
target_link_libraries(${PROJECT_NAME} winhttp.lib)
target_link_libraries(${PROJECT_NAME} wininet.lib)
target_link_libraries(${PROJECT_NAME} ws2_32.lib)
target_link_libraries(${PROJECT_NAME} dmoguids.lib)
target_link_libraries(${PROJECT_NAME} wmcodecdspuuid.lib)
target_link_libraries(${PROJECT_NAME} amstrmid.lib)
target_link_libraries(${PROJECT_NAME} msdmo.lib)
target_link_libraries(${PROJECT_NAME} Strmiids.lib)
target_link_libraries(${PROJECT_NAME} DelayImp.lib)
target_link_libraries(${PROJECT_NAME} setupapi.lib)

if(ENABLE_H264_FFMPEG)
target_link_libraries(${PROJECT_NAME} ${EXT_MINGW_LIBRARY_PATH}libgcc.lib)
target_link_libraries(${PROJECT_NAME} ${EXT_MINGW_LIBRARY_PATH}libmingwex.lib)
target_link_libraries(${PROJECT_NAME} ${EXT_LIBRARY_PATH}libx264.lib)
target_link_libraries(${PROJECT_NAME} ${EXT_LIBRARY_PATH}avformat.lib)
target_link_libraries(${PROJECT_NAME} ${EXT_LIBRARY_PATH}avutil.lib)
target_link_libraries(${PROJECT_NAME} ${EXT_LIBRARY_PATH}avcodec.lib)
endif(ENABLE_H264_FFMPEG)


if($ENV{MSBUILD_TYPE} STREQUAL "Release")
    target_link_libraries(${PROJECT_NAME} ${PJSIP_LIBRARY_PATH}/third_party/lib/libsrtp-i386-$ENV{MSVS_BUILD_PLATFORM}-vc-Release.lib)
    target_link_libraries(${PROJECT_NAME} ${PJSIP_LIBRARY_PATH}/lib/libpjproject-i386-$ENV{MSVS_BUILD_PLATFORM}-vc-Release.lib)
    target_link_libraries(${PROJECT_NAME} ${SSL_LIBRARY_PATH}libeay32MT.lib)
    target_link_libraries(${PROJECT_NAME} ${SSL_LIBRARY_PATH}ssleay32MT.lib)
    target_link_libraries(${PROJECT_NAME} LIBCMT.lib)
elseif($ENV{MSBUILD_TYPE} STREQUAL "Debug")
    target_link_libraries(${PROJECT_NAME} ${PJSIP_LIBRARY_PATH}/third_party/lib/libsrtp-i386-$ENV{MSVS_BUILD_PLATFORM}-vc-Debug.lib)
    target_link_libraries(${PROJECT_NAME} ${PJSIP_LIBRARY_PATH}/lib/libpjproject-i386-$ENV{MSVS_BUILD_PLATFORM}-vc-Debug.lib)
    target_link_libraries(${PROJECT_NAME} ${SSL_LIBRARY_PATH}libeay32MTd.lib)
    target_link_libraries(${PROJECT_NAME} ${SSL_LIBRARY_PATH}ssleay32MTd.lib)	
endif()

# delay load screen magnifier and dwmapi as they aren't available on XP
target_link_libraries(${PROJECT_NAME} Magnification.lib)
target_link_libraries(${PROJECT_NAME} Dwmapi.lib)
target_link_libraries(${PROJECT_NAME} dxva2.lib)
target_link_libraries(${PROJECT_NAME} d3d9.lib)
target_link_libraries(${PROJECT_NAME} iphlpapi.lib)
set_target_properties (${PROJECT_NAME} PROPERTIES
    LINK_FLAGS "${LINK_FLAGS} /delayload:Magnification.dll /delayload:Dwmapi.dll /delayload:dxva2.dll /delayload:d3d9.dll /delayload:iphlpapi.dll"
    )

set(WIX_HEAT_FLAGS
    -gg                 # Generate GUIDs
    -srd                # Suppress Root Dir
    -cg PluginDLLGroup  # Set the Component group name
    -dr INSTALLDIR      # Set the directory ID to put the files in
    )
if(ENABLE_H264_FFMPEG)
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
         ${EXT_LIBRARY_PATH}/avcodec-54.dll
         ${FB_BIN_DIR}/${PLUGIN_NAME}/$ENV{FBBUILDTYPE}/)
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
          ${EXT_LIBRARY_PATH}/avformat-54.dll
          ${FB_BIN_DIR}/${PLUGIN_NAME}/$ENV{FBBUILDTYPE}/)
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
         ${EXT_LIBRARY_PATH}/avutil-51.dll
         ${FB_BIN_DIR}/${PLUGIN_NAME}/$ENV{FBBUILDTYPE}/)
else()
    if($ENV{MSBUILD_TYPE} STREQUAL "Release")
        # enable optimization when FFMPEG is disabled
        message(STATUS "FFMPEG is disabled, so enable REFRENCES optimization")
        set_target_properties (${PROJECT_NAME} PROPERTIES
            LINK_FLAGS_RELWITHDEBINFO "${LINK_FLAGS} /OPT:REF"
        )
        set_target_properties (${PROJECT_NAME} PROPERTIES
            LINK_FLAGS_RELEASE "${LINK_FLAGS} /OPT:REF"
        )
        set_target_properties (${PROJECT_NAME} PROPERTIES
            LINK_FLAGS_MINSIZEREL "${LINK_FLAGS} /OPT:REF"
        )
    endif()
endif(ENABLE_H264_FFMPEG)

# Removed ${FB_BIN_DIR}/${PLUGIN_NAME}/${CMAKE_CFG_INTDIR}/${FBSTRING_PluginFileName}.dll
# from the 5th parameter to add_wix_installer as we don't use heat to extract the self
# registration entries from the plugin, they are in bjnplugininstaller.wxs.in. We do this
# as heat doesn't work with 64bit binaries so we can now build for 32 and 64 bit

add_wix_installer( ${PLUGIN_NAME}_${FBSTRING_PLUGIN_VERSION}
    ${CMAKE_CURRENT_SOURCE_DIR}/Win/WiX/bjnpluginInstaller.wxs
    PluginDLLGroup
    ${FB_BIN_DIR}/${PLUGIN_NAME}/${CMAKE_CFG_INTDIR}/
    ""
    ${PROJECT_NAME}
    )

if (EXISTS $ENV{SKINNY_CERTIFICATE})
    # Initialise all possible MSI file variables
    set (MSI_SHA2_SHA1 "${FB_BIN_DIR}/${PLUGIN_NAME}/${CMAKE_CFG_INTDIR}/${PLUGIN_NAME}_${FBSTRING_PLUGIN_VERSION}.msi")
    set (MSI_SHA2_SHA256 "${FB_BIN_DIR}/${PLUGIN_NAME}/${CMAKE_CFG_INTDIR}/${PLUGIN_NAME}_${FBSTRING_PLUGIN_VERSION}_$ENV{SHA2_SHA256_CERT_FILENAME_APPEND}.msi")

    # Generate post build step to copy the MSI files for signing with different certificates
    copy_msi_files("${PROJECT_NAME}_${FBSTRING_PLUGIN_VERSION}_WiXInstall"
        "${MSI_SHA2_SHA1}"
        "${MSI_SHA2_SHA256}"
        )

    # Sign MSI file with new certificate, this will sign with SHA1 hash and can be used on all OS
    firebreath_sign_file("${PROJECT_NAME}_${FBSTRING_PLUGIN_VERSION}_WiXInstall"
        "${MSI_SHA2_SHA1}"
        "$ENV{SKINNY_CERTIFICATE}"
        "$ENV{SKINNY_PWD_FILE}"
        "$ENV{SHA1_TIMESTAMP_URL}"
        )

    # Sign MSI file with new certificate, this will sign with SHA256 hash and can be used on Windows 7 and above only
    firebreath_sign_file256("${PROJECT_NAME}_${FBSTRING_PLUGIN_VERSION}_WiXInstall"
        "${MSI_SHA2_SHA256}"
        "$ENV{SKINNY_CERTIFICATE}"
        "$ENV{SKINNY_PWD_FILE}"
        "$ENV{SHA256_TIMESTAMP_URL}"
        )

endif()