#ifndef __PJ_CONFIG_SITE__
#define __PJ_CONFIG_SITE__

#ifdef __APPLE__
   #include "TargetConditionals.h"
#endif

#if TARGET_OS_IPHONE
#define PJ_CONFIG_IPHONE  1
#include <pj/config_site_sample.h>
#endif

#if TARGET_IS_ANDROID
#define PJ_CONFIG_ANDROID 1
#include <pj/config_site_sample.h>
#endif

//
// Start of common definitions
//

/**
 * Maximum packet length. We set it more than MTU since a SIP PDU
 * containing presence information can be quite large (>1500).
 * For instance Lync INVITES can be >12000
 */
#define PJSIP_MAX_PKT_LEN 20000

/**
 * Idle timeout interval to be applied to outgoing transports (i.e. client
 * side) with no usage before the transport is destroyed. Value is in
 * seconds.
 *
 * Note that if the value is put lower than 33 seconds, it may cause some
 * pjsip test units to fail. See the comment on the following link:
 * https://trac.pjsip.org/repos/ticket/1465#comment:4
 *
 * Default: 33
 */
#define PJSIP_TRANSPORT_IDLE_TIME    25

/**
 * Maximum message size that can be sent to output device for each call
 * to PJ_LOG(). If the message size is longer than this value, it will be cut.
 * This may affect the stack usage, depending whether PJ_LOG_USE_STACK_BUFFER
 * flag is set.
 *
 * Default: 4000
 *
 * Increased the maximum message size to allow the maximum packet length to be
 * logged together with any addition information.
 */
#define PJ_LOG_MAX_SIZE (PJSIP_MAX_PKT_LEN + 500)

/**
 * Maximum number of candidates for each ICE stream transport component.
 *
 * The default is 8, which isn't enough for some machines.  Alagu has
 * a machine at home with 6 local interfaces, plus we publish three
 * TURN servers, plus one STUN server.  Bump to 16 for now.
 */
#define PJ_ICE_ST_MAX_CAND 16

/**
 * Maximum number of ICE candidates.
 *
 * The default is 16, but this needs to increase as a function of
 * the number of components and the number of candidates each component
 * can have.
 */
#define PJ_ICE_MAX_CAND (PJ_ICE_ST_MAX_CAND * PJ_ICE_MAX_COMP)

/**
 * The duration of the STUN keep-alive period, in seconds.
 */
#define PJ_STUN_KEEP_ALIVE_SEC  5

/**
 * The TURN session timer heart beat interval. When this timer occurs, the 
 * TURN session will scan all the permissions/channel bindings to see which
 * need to be refreshed.
 */
#define PJ_TURN_KEEP_ALIVE_SEC 5

/**
 * Specify whether we prefer to use audio switch board rather than 
 * conference bridge.
 *
 * Audio switch board is a kind of simplified version of conference 
 * bridge, but not really the subset of conference bridge. It has 
 * stricter rules on audio routing among the pjmedia ports and has
 * no audio mixing capability. The power of it is it could work with
 * encoded audio frames where conference brigde couldn't.
 */
#define PJMEDIA_CONF_USE_SWITCH_BOARD 1

/**
 * This macro if set to 1 cancel all In Progress checks for the same component 
 * regardless of its priority. But BJN does not want to do this hence setting to 0.
 *
 */
#define PJ_ICE_CANCEL_ALL  0

/**
 * Maximum life-time of DNS response in the resolver response cache, 
 * in seconds. If the value is zero, then DNS response caching will be 
 * disabled.
 */
#define PJ_DNS_RESOLVER_MAX_TTL		    (24*60*60) //24 hours

//Timeout value to wait before ACK received
#define PJSIP_TD_TIMEOUT    20000

#define PJSIP_T1_TIMEOUT 800

#define PJ_TURN_PERM_TIMEOUT    2*60*60
#define PJ_TURN_CHANNEL_TIMEOUT 2*60*60

#define PJ_MAX_SDP_MEDIA  3

#define PJ_HAS_SSL_SOCK   1
#define PJSIP_HAS_TLS_TRANSPORT  1
#define PJMEDIA_HAS_SRTP 1
#define PJ_HAS_VIDEO      1
#define PJSUA_HAS_VIDEO   1
#define PJMEDIA_HAS_VIDEO 1
#define PJMEDIA_HAS_RED 1
#define PJMEDIA_STREAM_ENABLE_KA (1)

#undef PJMEDIA_HAS_G722_CODEC
#undef PJMEDIA_HAS_G7221_CODEC
#undef PJMEDIA_HAS_SPEEX_CODEC
#undef PJMEDIA_HAS_GSM_CODEC
#undef PJMEDIA_HAS_WEBRTC_AEC
#undef PJMEDIA_AUDIO_DEV_HAS_PORTAUDIO

#define PJMEDIA_HAS_G722_CODEC      0
#define PJMEDIA_HAS_G7221_CODEC     0
#define PJMEDIA_HAS_SPEEX_CODEC     0
#define PJMEDIA_HAS_GSM_CODEC       0

#define PJMEDIA_HAS_LIBSWSCALE          0
#define PJMEDIA_HAS_LIBAVUTIL           0
#define PJMEDIA_VIDEO_DEV_HAS_FFMPEG    0
#define PJMEDIA_VIDEO_DEV_HAS_CBAR_SRC  0
#define PJMEDIA_AUDIO_DEV_HAS_PORTAUDIO 0

#define PJMEDIA_HAS_PASSTHROUGH_CODEC_AMR  0
#define PJMEDIA_HAS_PASSTHROUGH_CODEC_G729 0
#define PJMEDIA_HAS_PASSTHROUGH_CODEC_ILBC 0
#define PJMEDIA_HAS_PASSTHROUGH_CODEC_PCMU 0
#define PJMEDIA_HAS_PASSTHROUGH_CODEC_PCMA 0
#define PJMEDIA_HAS_PASSTHROUGH_CODEC_OPUS 1

#define PJ_LOG_MAX_LEVEL                  6

/* Disable local host resulition like myhostname.local. On some lan it takes 
 * 4-5 secs for resolution of address and when some port like mdns 5353 is blocked. 
 */
#define PJ_GETHOSTIP_DISABLE_LOCAL_RESOLUTION 1

#define PJSUA_ACQUIRE_CALL_TIMEOUT (40 * 1000);

#define PJMEDIA_RTCP_NORMALIZE_FACTOR	0

//
// Start of specific target definitions
//



// Start of IOS target definitions
#if TARGET_OS_IPHONE

#ifndef __IPHONE_OS_VERSION_MIN_REQUIRED
#define __IPHONE_OS_VERSION_MIN_REQUIRED 40000
#endif

#undef PJMEDIA_VIDEO_DEV_HAS_IOS
#define PJMEDIA_VIDEO_DEV_HAS_IOS 0

#endif // TARGET_OS_IPHONE

// Start of MAC_OS target definitions
#if TARGET_OS_MAC && !TARGET_OS_IPHONE

#define PJMEDIA_HAS_WEBRTC_AEC            0
#define PJMEDIA_HAS_SPEEX_AEC             0
#define PJMEDIA_HAS_LEGACY_SOUND_API      0
#define PJMEDIA_AUDIO_DEV_HAS_COREAUDIO   0
#define BJN_WEBRTC_VOE                    1
#define PJMEDIA_HAS_PASSTHROUGH_CODECS    1

#endif // TARGET_OS_MAC && !TARGET_OS_IPHONE

//
// Add target definitions for Android, WIN32 and WIN64 here
//
#if WIN32 || WIN64
#define PJMEDIA_AUDIO_DEV_HAS_WMME         0
#define PJMEDIA_HAS_WEBRTC_AEC             0
#define PJMEDIA_HAS_SPEEX_AEC              0
#define PJMEDIA_VIDEO_DEV_HAS_DSHOW        0
#define BJN_WEBRTC_VOE                     1
#define PJMEDIA_HAS_PASSTHROUGH_CODECS     1
#endif

#if TARGET_IS_ANDROID
#define PJMEDIA_AUDIO_DEV_HAS_COREAUDIO   0
#define PJMEDIA_HAS_SPEEX_AEC             0
#define PJMEDIA_HAS_WEBRTC_AEC            0
#undef  PJMEDIA_VIDEO_DEV_HAS_CBAR_SRC
#define PJMEDIA_VIDEO_DEV_HAS_CBAR_SRC 0
#undef  PJMEDIA_HAS_FFMPEG_VID_CODEC
#define PJMEDIA_HAS_FFMPEG_VID_CODEC 0
#undef  PJ_LOG_MAX_LEVEL
#define PJ_LOG_MAX_LEVEL 4
#define PJ_ENABLE_EXTRA_CHECK 0
#define PJ_OS_HAS_CHECK_STACK 0
#define BJN_WEBRTC_VOE                1
#define PJMEDIA_HAS_PASSTHROUGH_CODECS    1
#endif

#if PJ_LINUX
#define PJMEDIA_AUDIO_DEV_HAS_COREAUDIO   0
#define BJN_WEBRTC_VOE                    1
#define PJMEDIA_HAS_PASSTHROUGH_CODECS    1
#endif

#undef PJ_OS_HAS_CHECK_STACK
#define PJ_OS_HAS_CHECK_STACK 0

#endif // __PJ_CONFIG_SITE__

