#ifndef __H264QSUTILS_H__
#define __H264QSUTILS_H__

#include <pjmedia/vid_codec_util.h>
#include "basevideocodec.h"

#if defined(ENABLE_H264_WIN_HWACCL)
#include <atlbase.h>
#include "mfxvideo++.h"

#include <initguid.h>
#include <d3d9.h>
#include <dxva2api.h>
#include <map>
#include <Mmsystem.h> // timeGetTime()

#define CHECK_RESULT(P, X, ERR)    {if ((X) > (P)) {return ERR;}}
#define ALIGN32(X)                 (((mfxU32)((X)+31)) & (~ (mfxU32)31))
#define ALIGN16(value)             (((value + 15) >> 4) << 4)
#define IGNORE_MFX_STS(P, X)       {if ((X) == (P)) {P = MFX_ERR_NONE;}}
#define CHECK_POINTER(P, ERR)      {if (!(P)) {return ERR;}}
#define SAFE_DELETE_ARRAY(P)       {if (P) {delete[] P; P = NULL;}}
#define SLEEP(X)                   { Sleep(X); }

#define DEVICE_MGR_TYPE MFX_HANDLE_DIRECT3D_DEVICE_MANAGER9

#define WIN7_MAJOR_VERSION      6
#define WIN7_MINOR_VERSION      1
#define MAX_NUM_PLANES          3
#define Y_PLANE                 0
#define U_PLANE                 1
#define V_PLANE                 2
#define MAX_RECOVER_ATTEMPT     2
#define SDK_MIN_MAJOR_VERSION   1u //HW H.264 Acceleration will be enabled only for endpoints which has atleast 1.3 Intel Media SDK version.
#define SDK_MIN_MINOR_VERSION   3u
#define MAX_WIDTH               1280
#define MAX_HEIGHT              720
#define MIN_ENCODE_FPS          15

//ERR_NO_BUFFER_AVAILABLE - Custom Error code for GetFreeSurfaceIndex() failure
#define ERR_NO_BUFFER_AVAILABLE -99
//TIME_INTERVAL - The time interval in milli secs which is used in calculating no. of H.264 API failures.
#define TIME_INTERVAL           1000
//MAX_FAILURE_RATE - Maximum failure count allowed in a time interval
#define MAX_FAILURE_RATE        5

int GetFreeSurfaceIndex(mfxFrameSurface1** pSurfacesPool, unsigned short nPoolSize);
void PrintErrString(char * logname, int sts, int line);
bool isHwAcclAvailable();
void postHwCodecDisableEvent(pjmedia_vid_codec* vidCodec, pjmedia_dir dir);
void configureEncoderParameters(mfxVideoParam &mfxEncParams);

enum //Array indexes used for VPP
{
    VPP_IN,
    VPP_OUT
};

struct VppSize
{
    mfxU16 width;
    mfxU16 height;
    VppSize::VppSize(mfxU16 _w, mfxU16 _h):width(_w),height(_h)
    {
    }
    VppSize::VppSize()
    {
        VppSize(0,0);
    }
};

struct VppInputI420
{
    unsigned char* planes[MAX_NUM_PLANES];
    size_t plane_widths[MAX_NUM_PLANES];
    size_t plane_heights[MAX_NUM_PLANES];
    size_t plane_strides[MAX_NUM_PLANES];
    VppInputI420::VppInputI420()
    {
        memset(planes,0,sizeof(planes));
        memset(plane_widths,0,sizeof(plane_widths));
        memset(plane_heights,0,sizeof(plane_heights));
        memset(plane_strides,0,sizeof(plane_strides));
    }
};

/*
FailureRate - Calculates failure rate for the specified time interval
Currently the time interval (TIME_INTERVAL) is set to 1 second, and the number of failures are 
saved for the current time interval. 
If the failure rate exceeds MAX_FAILURE_RATE, IsFailureRateFrequent() returns true.
*/

class FailureRate
{
    DWORD m_prevTime;
    unsigned int  m_numFailures; //Number of failures
    unsigned int m_failureRate; //Number of failures in a time interval

public:
    FailureRate() :m_failureRate(0),
                   m_numFailures(0),
                   m_prevTime(0)
    {
        Reset();
    }

    void Reset()
    {
        m_prevTime = timeGetTime();
        m_numFailures = 0;
    }

    // IsFailureRateFrequent needs to be called whenever the failure happens
    bool IsFailureRateFrequent()
    {
        bool _isFailFrequent = false;
        DWORD _currTime = timeGetTime();
        DWORD _timeDiff = _currTime - m_prevTime;
        ++m_numFailures;
        if (_timeDiff > TIME_INTERVAL)
        {
            DWORD _intervalsElapsed = _timeDiff / TIME_INTERVAL;
            m_failureRate = m_numFailures / _intervalsElapsed; // Number of failures per time interval
            _isFailFrequent = (m_failureRate >= MAX_FAILURE_RATE) ? true : false;
            Reset();
        }
        return _isFailFrequent;
    }

    unsigned int GetFailureRate()
    {
        return m_failureRate;
    }
};

class VideoMemAllocator : public mfxFrameAllocator
{
public:
    VideoMemAllocator();
    ~VideoMemAllocator();
    mfxStatus Initialize(MFXVideoSession* pSession, HWND hWnd = NULL, bool bCreateSharedHandles = false);
    mfxStatus AllocFrames(mfxFrameAllocRequest *request, mfxFrameAllocResponse *response);
    mfxStatus LockFrame(mfxMemId mid, mfxFrameData *ptr);
    mfxStatus UnlockFrame(mfxMemId mid, mfxFrameData *ptr);
    mfxStatus GetFrameHDL(mfxMemId mid, mfxHDL *handle);
    mfxStatus FreeFrames(mfxFrameAllocResponse *response);

private:
    CComPtr<IDirect3DDeviceManager9>           m_pDeviceManager9;
    CComPtr<IDirect3DDevice9Ex>                m_pDevice9Ex ;
    CComPtr<IDirect3D9Ex>                      m_pD3D9;
    HANDLE                                     m_pDeviceHandle;
    CComPtr<IDirectXVideoAccelerationService>  m_pDXVAServiceDec;
    CComPtr<IDirectXVideoAccelerationService>  m_pDXVAServiceVPP;
    bool                                       m_bCreateSharedHandles;
    bool                                       m_bIsHWdeviceCreated;
    std::map<mfxMemId*, mfxHDL>                m_allocResponses;
    std::map<mfxHDL, mfxFrameAllocResponse>    m_allocDecodeResponses;
    std::map<mfxHDL, int>                      m_allocDecodeRefCount;
    mfxU32                                     m_adapterNum;
    char                                       m_LogName[32];

    static mfxStatus MFX_CDECL  Alloc_(mfxHDL pthis, mfxFrameAllocRequest *request, mfxFrameAllocResponse *response);
    static mfxStatus MFX_CDECL  Lock_(mfxHDL pthis, mfxMemId mid, mfxFrameData *ptr);
    static mfxStatus MFX_CDECL  Unlock_(mfxHDL pthis, mfxMemId mid, mfxFrameData *ptr);
    static mfxStatus MFX_CDECL  GetHDL_(mfxHDL pthis, mfxMemId mid, mfxHDL *handle);
    static mfxStatus MFX_CDECL  Free_(mfxHDL pthis, mfxFrameAllocResponse *response);

    mfxU32 GetIntelDeviceAdapterNum(mfxSession session);
    mfxStatus InitHWDevice(HWND window, bool bCreateSharedHandles);
    void CleanupHWDevice();
};

class VideoProcessor
{
    MFXVideoVPP*                m_pMfxVPP; // VPP instance
    mfxVideoParam               m_VPPParams; // parameters for VPP (Scaling and YV12->NV12 conversion)
    mfxExtVPPDoNotUse           m_extDoNotUse;
    mfxExtBuffer*               m_VppExtBuffers[1];
    mfxFrameAllocResponse       m_mfxResponseVPPIn;
    mfxFrameSurface1**          m_pmfxSurfacesVPPIn;
    mfxU16                      m_nSurfNumVPP[2];
    VideoMemAllocator*          m_pVideoAllocator;
    bool                        m_bIsFramesAllocated;
    VppSize                     m_sizes[2];
    char                        m_LogName[32];

    mfxStatus AllocSurfaces();
    void FreeSurfaces();

public:
    VideoProcessor();
    mfxStatus Initialize(mfxSession,VideoMemAllocator*);
    void Close();
    void SetResolutions(VppSize*, VppSize*);
    void GetVPPOutInfo(mfxFrameInfo&);
    mfxU16 GetNumSurfaces(unsigned int);
    mfxStatus ProcessFrame(VppInputI420* ,mfxFrameSurface1*);
    ~VideoProcessor();
};
#endif
#endif //__H264QSUTILS_H__

