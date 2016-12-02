//
//  config_handler.cc
//
//
//  Created by Deepak on 12/12/12.
//  Copyright (c) 2012 Bluejeans. All rights reserved.
//

#ifndef __CONFIG_HANDLER_H
#define __CONFIG_HANDLER_H

#include <iostream>
#include <cstdio>
#include <vector>
#include <map>
#include <stdint.h>
#include <pjlib-util/xml.h>
#include <pj/file_io.h>

class Config_Parser_Handler
{
public:
    Config_Parser_Handler(const std::string &config_file);
    ~Config_Parser_Handler();

    void parseConfigXML(char *buffer, pj_size_t len);
    void parseConfigXML();

    int Read(void* buffer, size_t buffer_len, size_t* read, int* error);

    std::vector<std::string> vendor() { return vendor_; }
    std::vector<std::string> android_model() { return android_model_; }
    std::vector<int> family() { return family_; }
    std::vector<int> model() { return model_; }
    std::vector<int> stepping() { return stepping_; }
    std::vector<int> vcores() { return vcores_; }
    std::vector<int> frequency() { return frequency_; }
    std::vector<int> encwidth() { return enc_width_; }
    std::vector<int> encheight() { return enc_height_; }
    std::vector<int> vp8_encwidth() { return vp8_enc_width_; }
    std::vector<int> vp8_encheight() { return vp8_enc_height_; }
    std::vector<int> encfps() { return enc_fps_; }
    std::vector<int> decwidth() { return dec_width_; }
    std::vector<int> decheight() { return dec_height_; }
    std::vector<int> vp8_decwidth() { return vp8_dec_width_; }
    std::vector<int> vp8_decheight() { return vp8_dec_height_; }
    std::vector<int> decfps() { return dec_fps_; }
    std::vector<bool> dualstream() { return dual_stream_; }
    std::vector<std::string> machineModel() { return machine_model_; }

    int duration() { return duration_; }
    int bandwidth() { return bandwidth_; }

    bool forceTurn() { return force_turn_; }
    std::string turnProtocol() { return turn_protocol_; }

    std::string secureProp() const { return secure_call_; }
    bool doCallSecure() const { return do_secure_call_; }
    int getRefreshRate() const { return refresh_rate_; }

    bool useD3D9Renderer() const { return use_D3D9_renderer_; }
    
    bool readPcap() const { return read_pcap_; }
    bool writePcap() const { return write_pcap_; }

    int  bitrate(int med_idx) {return bitrate_[med_idx]; }
    int  bitrateBurst(int med_idx) {return bitrate_burst_[med_idx]; }

    int minQPVP8() const { return minQPVP8_; }

    int maxQPVP8() const { return maxQPVP8_; }

    int minQPVP8Content() const { return minQPVP8Content_; }

    int maxQPVP8Content() const { return maxQPVP8Content_; }

    int minQPVP8RDC() const { return minQPVP8RDC_; }

    int maxQPVP8RDC() const { return maxQPVP8RDC_; }

    int contentHighResBitrate() const { return contentHighResBitrate_; }

    int videoFreeRangeQPBitrate() const { return videoFreeRangeQPBitrate_; }

    bool sipUaThread() const { return sipua_thread_; }

    int maxLowRTT() const { return maxLowRTT_; }

    float maxLoss() const { return maxLoss_; }

    int maxAllowedRTXDelay() const { return maxAllowedRTXDelay_; }

    float minLossToEnableRtx() const { return minLossToEnableRtx_; }

    int pjsipLogLevel() const { return pjsipLogLevel_; }

    int minIntervalBetweenEnablement() const { return minIntervalBetweenEnablement_; }

    typedef std::map<std::string, int>                  DevicePropertiesMap;
    typedef std::map<std::string, DevicePropertiesMap>  DeviceConfigMap;

    const DeviceConfigMap & getDeviceConfig() const { return device_config_; }

    void ChoppyAudioParameters(uint32_t& rx_loss,
                                 uint32_t& rx_tau,
                                 uint32_t& tx_loss,
                                 uint32_t& tx_tau) const {
        rx_loss = audio_gd_rx_loss_thresh_;
        rx_tau = audio_gd_rx_tau_;
        tx_loss = audio_gd_tx_loss_thresh_;
        tx_tau = audio_gd_tx_tau_;
    }

    struct VideoDeviceProperties
    {
        VideoDeviceProperties()
        : width(0)
        ,height(0)
        ,fps(0)
        {
            
        }
        
        int width;
        int height;
        int fps;
    };
    
    typedef std::vector<VideoDeviceProperties> VideoDevicePropertiesList;
    typedef std::map<std::string, VideoDevicePropertiesList> VideoDevicePropertyMap;
    typedef std::map<std::string, bool> RestartCaptureMap;

    const VideoDevicePropertyMap& getVideoDevicePropertiesMap() const { return video_device_properties_; }
    const RestartCaptureMap& getRestartCaptureMap() const { return restart_capture_map_; }
    
    int chromaScaleFactor() const { return chromaScaleFactor_; }

private:
    pj_pool_t                            *pool;
    pj_oshandle_t                        *fp;
    std::string                          configFile;
    std::vector<std::string>             vendor_;
    std::vector<std::string>             android_model_;
    std::vector<int>                     family_;
    std::vector<int>                     model_;
    std::vector<int>                     stepping_;
    std::vector<int>                     vcores_;
    std::vector<int>                     frequency_;
    std::vector<int>                     enc_width_;
    std::vector<int>                     enc_height_;
    std::vector<int>                     vp8_enc_width_;
    std::vector<int>                     vp8_enc_height_;
    std::vector<int>                     enc_fps_;
    std::vector<int>                     dec_width_;
    std::vector<int>                     dec_height_;
    std::vector<int>                     vp8_dec_width_;
    std::vector<int>                     vp8_dec_height_;
    std::vector<int>                     dec_fps_;
    std::vector<bool>                    dual_stream_;
    std::vector<std::string>             machine_model_;
    DeviceConfigMap                      device_config_;
    VideoDevicePropertyMap               video_device_properties_;
    RestartCaptureMap                    restart_capture_map_;

    int                     bandwidth_;
    int                     duration_;

    // TURN Server Config
    bool                    force_turn_;
    std::string             turn_protocol_;

    // Secure Call Config
    std::string             secure_call_;
    bool                    do_secure_call_;
    // refresh frame rate
    int                     refresh_rate_;

    // D3D9 Rendering On/Off
    bool                    use_D3D9_renderer_;

    // pcap writing On/Off
    bool                    write_pcap_;

    // Feeds in media from a pcap -- On/Off
    bool                    read_pcap_;

    //Turn server settings
    int                     bitrate_[MAX_MED_IDX];
    int                     bitrate_burst_[MAX_MED_IDX];

    // VP8 Encoder minQP for main video
    int                     minQPVP8_;

    // VP8 Encoder maxQP for main video
    int                     maxQPVP8_;

    // VP8 Encoder minQP for content
    int                     minQPVP8Content_;

    // VP8 Encoder maxQP for content
    int                     maxQPVP8Content_;

    // VP8 Encoder minQP for RDC(remote desktop control)
    int                     minQPVP8RDC_;

    // VP8 Encoder maxQP for RDC
    int                     maxQPVP8RDC_;

    // VP8 Encoder minimum bandwidth for content with 1080p resolution
    int                     contentHighResBitrate_;

    // VP8 Encoder minimum bitrate for video with free range qp
    int                     videoFreeRangeQPBitrate_;

    // sip ua thread enable/disable
    bool                    sipua_thread_;

    //max low RTT
    int                     maxLowRTT_;
    float                   maxLoss_;

    //max RTT for RTX
    int                     maxAllowedRTXDelay_;

    //min loss to enable RTX
    float                   minLossToEnableRtx_;

    //interval between RTX inbound enablement
    int                     minIntervalBetweenEnablement_;

    //maximum allowed log level in pjsip
    int                     pjsipLogLevel_;

    // tuning parameters for FBR-537
    uint32_t audio_gd_rx_loss_thresh_;
    uint32_t audio_gd_rx_tau_;
    uint32_t audio_gd_tx_loss_thresh_;
    uint32_t audio_gd_tx_tau_;

    // color vividness chroma scale factor
    int                     chromaScaleFactor_;
};

#endif  //__CONFIG_HANDLER_H
