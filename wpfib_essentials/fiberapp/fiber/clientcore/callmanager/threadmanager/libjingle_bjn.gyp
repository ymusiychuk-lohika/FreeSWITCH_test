# Copyright (c) 2011 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

{
  'variables': {
    'no_libjingle_logging%': 0,
    'libjingle_orig': '.',
    #TODO: Move openssl sdk to fiber.
    'openssl_path': '../../../ext_libs/win32/openssl/',
  },
  'target_defaults': {
    'defines': [
      'FEATURE_ENABLE_SSL',
      'FEATURE_ENABLE_VOICEMAIL',  # TODO(ncarter): Do we really need this?
      #'_USE_32BIT_TIME_T',
      'SAFE_TO_DEFINE_TALK_BASE_LOGGING_MACROS',
      'EXPAT_RELATIVE_PATH',
      'JSONCPP_RELATIVE_PATH',
      'SRTP_RELATIVE_PATH',
      'WEBRTC_RELATIVE_PATH',
    ],
    'configurations': {
      'Debug': {
        'defines': [
          # TODO(sergeyu): Fix libjingle to use NDEBUG instead of
          # _DEBUG and remove this define. See below as well.
          #'_DEBUG',
        ],
      }
    },
    'include_dirs': [
      '../../third_party/libyuv/include/',
      '../../third_party/libsrtp/srtp/include/',
      '../../third_party/libsrtp/srtp/crypto/include/',
    ],
    'direct_dependent_settings': {
      'defines': [
        'FEATURE_ENABLE_SSL',
        'FEATURE_ENABLE_VOICEMAIL',
        'EXPAT_RELATIVE_PATH',
        'WEBRTC_RELATIVE_PATH',
      ],
      'conditions': [
        ['OS=="win"', {
          'link_settings': {
            'libraries': [
              '-lsecur32.lib',
              '-lcrypt32.lib',
              '-liphlpapi.lib',
              '<(openssl_path)/lib/libeay32.lib',
              '<(openssl_path)/lib/ssleay32.lib',
              '-lPdh.lib',
            ],
          },
        }],
        ['OS=="win"', {
          'include_dirs': [
            '../third_party/platformsdk_win7/files/Include',
            '<(openssl_path)/include/',
          ],
          'defines': [
              '_CRT_SECURE_NO_WARNINGS',  # Suppres warnings about _vsnprinf
              'SSL_USE_SCHANNEL',
          ],
        }],
        ['OS=="linux"', {
          'include_dirs': [
              '../../../openssl/include/',
          ],
          'defines': [
            'LINUX',
            'SSL_USE_OPENSSL',
            'HAVE_OPENSSL_SSL_H',
            'HAVE_LIBPULSE',
          ],
          'link_settings': {
            'libraries': [
              '../../../bjnbuild/i386/lib/libssl.a',
              '../../../bjnbuild/i386/lib/libcrypto.a',
            ],
          },
        }],
        ['OS=="mac"', {
          'include_dirs': [
              '../../../openssl/include/',
          ],
          'defines': [
            'OSX',
            'SSL_USE_OPENSSL',
            'HAVE_OPENSSL_SSL_H',
          ],
        }],
        ['os_posix == 1', {
          'defines': [
            'POSIX',
          ],
        }],
        ['OS=="openbsd" or OS=="freebsd"', {
          'defines': [
            'BSD',
          ],
        }],
        ['no_libjingle_logging==1', {
          'defines': [
            'NO_LIBJINGLE_LOGGING',
          ],
        }],
      ],
    },
    'all_dependent_settings': {
      'configurations': {
        'Debug': {
          'defines': [
            # TODO(sergeyu): Fix libjingle to use NDEBUG instead of
            # _DEBUG and remove this define. See above as well.
            #'_DEBUG',
          ],
        }
      },
    },
    'conditions': [
      ['inside_chromium_build==1', {
        'defines': [
          'NO_SOUND_SYSTEM',
        ],
        'include_dirs': [
          '<(libjingle_orig)/source',
          '../..',  # the third_party folder for webrtc includes
        ],
        'direct_dependent_settings': {
          'defines': [
            'NO_SOUND_SYSTEM',
          ],
          'include_dirs': [
            '<(libjingle_orig)/source',
          ],
        },
      },{
        'include_dirs': [
          # the third_party folder for webrtc/ includes (non-chromium).
          '../../src',
          '<(libjingle_orig)/source',
        ],
      }],
      ['OS=="win"', {
        'include_dirs': [
          '../third_party/platformsdk_win7/files/Include',
            '<(openssl_path)/include/',
        ],
      }],
      ['OS=="linux"', {
        'include_dirs': [
            '../../../openssl/include/',
        ],
        'defines': [
          'LINUX',
          'HAVE_LIBPULSE',
        ],
        'link_settings': {
          'libraries': [
            '../../../bjnbuild/i386/lib/libssl.a',
            '../../../bjnbuild/i386/lib/libcrypto.a',
          ],
        },
      }],
      ['OS=="mac"', {
        'include_dirs': [
            '../../../openssl/include/',
        ],
        'defines': [
          'OSX',
        ],
      }],
      ['os_posix == 1', {
        'defines': [
          'POSIX',
        ],
      }],
      ['OS=="openbsd" or OS=="freebsd"', {
        'defines': [
          'BSD',
        ],
      }],
    ],
  },
  'targets': [
    {
      'target_name': 'libjingle',
      'type': 'static_library',
      'sources': [
        '<(libjingle_orig)/source/talk/base/asyncfile.cc',
        '<(libjingle_orig)/source/talk/base/asyncfile.h',
        '<(libjingle_orig)/source/talk/base/asyncsocket.cc',
        '<(libjingle_orig)/source/talk/base/asyncsocket.h',
        '<(libjingle_orig)/source/talk/base/common.cc',
        '<(libjingle_orig)/source/talk/base/common.h',
        '<(libjingle_orig)/source/talk/base/cpuid.cc',
        '<(libjingle_orig)/source/talk/base/cpuid.h',
        '<(libjingle_orig)/source/talk/base/cpumonitor.cc',
        '<(libjingle_orig)/source/talk/base/cpumonitor.h',
        '<(libjingle_orig)/source/talk/base/fileutils.cc',
        '<(libjingle_orig)/source/talk/base/fileutils.h',
        '<(libjingle_orig)/source/talk/base/pathutils.cc',
        '<(libjingle_orig)/source/talk/base/pathutils.h',
        '<(libjingle_orig)/source/talk/base/urlencode.cc',
        '<(libjingle_orig)/source/talk/base/urlencode.h',
        '<(libjingle_orig)/source/talk/base/stringutils.cc',
        '<(libjingle_orig)/source/talk/base/stringutils.h',
        '<(libjingle_orig)/source/talk/base/worker.cc',
        '<(libjingle_orig)/source/talk/base/worker.h',
        '<(libjingle_orig)/source/talk/base/ipaddress.cc',
        '<(libjingle_orig)/source/talk/base/ipaddress.h',
        '<(libjingle_orig)/source/talk/base/logging.cc',
        '<(libjingle_orig)/source/talk/base/logging_libjingle.h',
        '<(libjingle_orig)/source/talk/base/messagehandler.cc',
        '<(libjingle_orig)/source/talk/base/messagehandler.h',
        '<(libjingle_orig)/source/talk/base/messagequeue.cc',
        '<(libjingle_orig)/source/talk/base/messagequeue.h',
        '<(libjingle_orig)/source/talk/base/nethelpers.cc',
        '<(libjingle_orig)/source/talk/base/nethelpers.h',
        '<(libjingle_orig)/source/talk/base/physicalsocketserver.cc',
        '<(libjingle_orig)/source/talk/base/physicalsocketserver.h',
        '<(libjingle_orig)/source/talk/base/signalthread.cc',
        '<(libjingle_orig)/source/talk/base/signalthread.h',
        '<(libjingle_orig)/source/talk/base/sigslot.h',
        '<(libjingle_orig)/source/talk/base/sigslotrepeater.h',
        '<(libjingle_orig)/source/talk/base/socketaddress.cc',
        '<(libjingle_orig)/source/talk/base/socketaddress.h',
        '<(libjingle_orig)/source/talk/base/stream.cc',
        '<(libjingle_orig)/source/talk/base/stream.h',
        '<(libjingle_orig)/source/talk/base/stringencode.cc',
        '<(libjingle_orig)/source/talk/base/stringencode.h',
        '<(libjingle_orig)/source/talk/base/systeminfo.cc',
        '<(libjingle_orig)/source/talk/base/systeminfo.h',
        '<(libjingle_orig)/source/talk/base/thread.cc',
        '<(libjingle_orig)/source/talk/base/thread.h',
        '<(libjingle_orig)/source/talk/base/time.cc',
        '<(libjingle_orig)/source/talk/base/time.h',
        '<(libjingle_orig)/source/talk/base/timing.cc',
        '<(libjingle_orig)/source/talk/base/timing.h',
        '<(libjingle_orig)/source/talk/base/window.h',
        '<(libjingle_orig)/source/talk/base/windowpicker.h',
        '<(libjingle_orig)/source/talk/base/windowpickerfactory.h',
        '<(libjingle_orig)/source/talk/session/phone/devicemanager.cc',
        '<(libjingle_orig)/source/talk/session/phone/devicemanager.h',
        '<(libjingle_orig)/source/talk/base/smcmetrics.h',
        '<(libjingle_orig)/source/talk/base/smcmetrics.cc',
      ],
      'conditions': [
        ['inside_chromium_build==1', {
          'sources': [ 
            '<(libjingle_orig)/source/talk/sound/automaticallychosensoundsystem.h', 
            '<(libjingle_orig)/source/talk/sound/platformsoundsystem.cc', 
            '<(libjingle_orig)/source/talk/sound/platformsoundsystem.h', 
            '<(libjingle_orig)/source/talk/sound/platformsoundsystemfactory.cc', 
            '<(libjingle_orig)/source/talk/sound/platformsoundsystemfactory.h', 
            '<(libjingle_orig)/source/talk/sound/sounddevicelocator.h', 
            '<(libjingle_orig)/source/talk/sound/soundinputstreaminterface.h', 
            '<(libjingle_orig)/source/talk/sound/soundoutputstreaminterface.h', 
            '<(libjingle_orig)/source/talk/sound/soundsystemfactory.h', 
            '<(libjingle_orig)/source/talk/sound/soundsysteminterface.cc', 
            '<(libjingle_orig)/source/talk/sound/soundsysteminterface.h', 
            '<(libjingle_orig)/source/talk/sound/soundsystemproxy.cc', 
            '<(libjingle_orig)/source/talk/sound/soundsystemproxy.h',
          ],
          'conditions' : [ 
            ['OS=="linux"', {
              'sources': [
                '<(libjingle_orig)/source/talk/base/latebindingsymboltable.cc',
                '<(libjingle_orig)/source/talk/base/latebindingsymboltable.h',
                '<(libjingle_orig)/source/talk/sound/alsasoundsystem.cc',
                '<(libjingle_orig)/source/talk/sound/alsasoundsystem.h',
                '<(libjingle_orig)/source/talk/sound/alsasymboltable.cc',
                '<(libjingle_orig)/source/talk/sound/alsasymboltable.h',
                '<(libjingle_orig)/source/talk/sound/linuxsoundsystem.cc',
                '<(libjingle_orig)/source/talk/sound/linuxsoundsystem.h',
                '<(libjingle_orig)/source/talk/sound/pulseaudiosoundsystem.cc',
                '<(libjingle_orig)/source/talk/sound/pulseaudiosoundsystem.h',
                '<(libjingle_orig)/source/talk/sound/pulseaudiosymboltable.cc',
                '<(libjingle_orig)/source/talk/sound/pulseaudiosymboltable.h',
              ],
            }],
          ],
        }],
        ['OS=="win"', {
          'sources': [
            '<(libjingle_orig)/source/talk/base/win32socketinit.cc',
            '<(libjingle_orig)/source/talk/base/win32.h',
            '<(libjingle_orig)/source/talk/base/win32.cc',
            '<(libjingle_orig)/source/talk/base/win32regkey.h',
            '<(libjingle_orig)/source/talk/base/win32regkey.cc',
            '<(libjingle_orig)/source/talk/base/win32window.h',
            '<(libjingle_orig)/source/talk/base/win32window.cc',
            '<(libjingle_orig)/source/talk/base/winping.cc',
            '<(libjingle_orig)/source/talk/base/winping.h',
            '<(libjingle_orig)/source/talk/base/win32socketserver.cc',
            '<(libjingle_orig)/source/talk/base/win32socketserver.h',
            '<(libjingle_orig)/source/talk/base/win32windowpicker.cc',
            '<(libjingle_orig)/source/talk/base/win32windowpicker.h',
            '<(libjingle_orig)/source/talk/base/win32processdetails.cc',
            '<(libjingle_orig)/source/talk/base/win32processdetails.h',
            '<(libjingle_orig)/source/talk/session/phone/win32devicemanager.cc',
            '<(libjingle_orig)/source/talk/session/phone/win32devicemanager.h',
            '<(libjingle_orig)/source/talk/session/phone/audiodevlistener.h',
            '<(libjingle_orig)/source/talk/session/phone/audiodevlistener.cc',
            '<(libjingle_orig)/source/talk/session/phone/audioendpointnotification.h',
            '<(libjingle_orig)/source/talk/session/phone/audioendpointnotification.cc',
            '<(libjingle_orig)/source/talk/session/phone/speakervolumenotification.h',
            '<(libjingle_orig)/source/talk/session/phone/speakervolumenotification.cc',
            '<(libjingle_orig)/source/talk/session/phone/micvolumenotification.h',
            '<(libjingle_orig)/source/talk/session/phone/micvolumenotification.cc',
            '<(libjingle_orig)/source/talk/base/win32cpuparams.cc',
            '<(libjingle_orig)/source/talk/base/win32cpuparams.h',
          ],
        }],
        ['OS=="linux"', {
          'sources': [
            '<(libjingle_orig)/source/talk/base/unixfilesystem.cc',
            '<(libjingle_orig)/source/talk/base/unixfilesystem.h',
            '<(libjingle_orig)/source/talk/base/linux.cc',
            '<(libjingle_orig)/source/talk/base/linux.h',
            "<(libjingle_orig)/source/talk/base/linuxwindowpicker.cc",
            "<(libjingle_orig)/source/talk/base/linuxwindowpicker.h",
            '<(libjingle_orig)/source/talk/session/phone/libudevsymboltable.cc',
            '<(libjingle_orig)/source/talk/session/phone/libudevsymboltable.h',
            '<(libjingle_orig)/source/talk/session/phone/linuxdevicemanager.cc',
            '<(libjingle_orig)/source/talk/session/phone/linuxdevicemanager.h',
            '<(libjingle_orig)/source/talk/session/phone/linuxdeviceinfo.cc',
            '<(libjingle_orig)/source/talk/session/phone/linuxdeviceinfo.h',
            '<(libjingle_orig)/source/talk/session/phone/v4llookup.cc',
            '<(libjingle_orig)/source/talk/session/phone/v4llookup.h',
          ],
          'include_dirs': [
            '<(libjingle_orig)/source/talk/third_party/libudev',
          ],
        }],
        ['OS=="mac"', {
          'sources': [
            '<(libjingle_orig)/source/talk/base/macconversion.cc',
            '<(libjingle_orig)/source/talk/base/macconversion.h',
            '<(libjingle_orig)/source/talk/base/macutils.cc',
            '<(libjingle_orig)/source/talk/base/macutils.h',
            '<(libjingle_orig)/source/talk/base/macasyncsocket.cc',
            '<(libjingle_orig)/source/talk/base/macasyncsocket.h',
            "<(libjingle_orig)/source/talk/base/macsocketserver.cc",
            "<(libjingle_orig)/source/talk/base/macsocketserver.h",
            "<(libjingle_orig)/source/talk/base/maccocoasocketserver.mm",
            "<(libjingle_orig)/source/talk/base/maccocoasocketserver.h",
            "<(libjingle_orig)/source/talk/base/macwindowpicker.cc",
            "<(libjingle_orig)/source/talk/base/macwindowpicker.h",
            "<(libjingle_orig)/source/talk/base/macwindowpickerobjc.mm",
            "<(libjingle_orig)/source/talk/base/macwindowpickerobjc.h",
            "<(libjingle_orig)/source/talk/base/scoped_autorelease_pool.mm",
            "<(libjingle_orig)/source/talk/base/scoped_autorelease_pool.h",
            "<(libjingle_orig)/source/talk/session/phone/macdevicemanager.cc",
            "<(libjingle_orig)/source/talk/session/phone/macdevicemanagermm.mm",
            "<(libjingle_orig)/source/talk/base/macsmcmetrics.cc",
            "<(libjingle_orig)/source/talk/base/macsmcmetrics.h",
          ],
        }],
      ],
    },
  ],
}
