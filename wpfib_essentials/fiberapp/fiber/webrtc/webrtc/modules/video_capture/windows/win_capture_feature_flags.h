
#ifndef __WIN_CAPTURER_FEATURE_FLAGS_H_
#define __WIN_CAPTURER_FEATURE_FLAGS_H_

namespace BJN {

    // Feature type description, used by windows app capture process
    typedef enum _FeatureType {
        CaptureApp2UsingLatest       = 1,    // AppCapture2 with hooks and magnification
        CaptureScreenUsingDDBasic    = 2,    // primary & secondary (partial) monitor using dd
        CaptureScreenUsingDDComplete = 3,    // all monitor support & smart presenter widget filtering
        CaptureAppUsingDDComplete    = 4     // Capture application using dd
    }FeatureType;

    // Exit code for app capture process
    typedef enum _AppCaptureExitCode {
        ExitSuccess              = 0,  // App capture process exit successfully
        ExitFailureGeneric       = -1, // App capture process exit with failure
        ExitFailureDDScreenShare = -2  // Problem with current DD screen capture mechanism request to fallback to existing mechanism
    };

    class AppCaptureProcessParameters {
    public:
        typedef struct _ProcessData{
            FeatureType requestedFeatureType;
            FeatureType deliveredFeatureType;
            bool bInfoBarEnabled;
        } ProcessData;

        // Create new app capture params
        AppCaptureProcessParameters(bool bCreate);
        // Open existing app capture params
        AppCaptureProcessParameters(LPCTSTR lpSharedDataId);

        ~AppCaptureProcessParameters();

        bool IsValid() const { return _isValid; };
        bool SetRequestedFeatureType(FeatureType requestedFeatureType);
        bool GetRequestedFeatureType(FeatureType& requestedFeatureType) const;
        bool SetDeliveredFeatureType(FeatureType deliveredFeatureType);
        bool GetDeliveredFeatureType(FeatureType& deliveredFeatureType) const;
        bool SetInfoBarEnabled(bool enabled);
        bool GetInfoBarEnabled(bool& enabled) const;

        bool GetSharedDataId(std::wstring& sharedDataId) const;
    private:
        bool          _isValid;
        ProcessData*  _pProcessData;
        std::wstring  _sharedDataId;
        HANDLE        _sharedMemory;
    };

    bool GetGUID(std::wstring& sGuid);

} // namespace BJN

#endif //__WIN_CAPTURER_FEATURE_FLAGS_H_