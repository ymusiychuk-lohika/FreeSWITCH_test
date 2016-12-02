/* Copyright (C) 2007 Jean-Marc Valin
      
   File: speex_resampler.h 
   Resampling code
      
   The design goals of this code are:
      - Very fast algorithm
      - Low memory requirement
      - Good *perceptual* quality (and not best SNR)

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions are
   met:

   1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

   2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.

   3. The name of the author may not be used to endorse or promote products
   derived from this software without specific prior written permission.

   THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
   IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
   OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
   DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
   INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
   (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
   SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
   HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
   STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
   ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
   POSSIBILITY OF SUCH DAMAGE.
*/
#ifndef SPEEX_RESAMPLER_H
#define SPEEX_RESAMPLER_H

namespace SpeexResampler {

#define FLOATING_POINT
#ifndef M_PI
#define M_PI 3.14159263
#endif

enum {
   RESAMPLER_ERR_SUCCESS         =  0,
   RESAMPLER_ERR_ALLOC_FAILED    = -1,
   RESAMPLER_ERR_BAD_STATE       = -2,
   RESAMPLER_ERR_INVALID_ARG     = -3,
   RESAMPLER_ERR_PTR_OVERLAP     = -4,
   RESAMPLER_ERR_MAX_ERROR       = -5,
} ;

struct FuncDef {
   double *table;
   int oversample;
};

class SpeexResampler;

typedef int (SpeexResampler::*resampler_basic_func)(unsigned int , const float *, unsigned int *, float *, unsigned int *);

class SpeexResampler  
{
protected:
   unsigned int mInRate;
   unsigned int mOutRate;
   unsigned int mNumRate;
   unsigned int mDenRate;
   
   int    mQuality;
   float  mDownsampleBw;
   float  mUpsampleBw;
   unsigned int mNbChannels;
   unsigned int mFiltLen;
   unsigned int mMemAllocSize;
   unsigned int mBufferSize;
   int mIntAdvance;
   int mFracAdvance;
   float mCutoff;
   unsigned int mOversample;
   int   mInitialised;
   int   mStarted;
   
   /* These are per-channel */
   int  *mLastSample;
   unsigned int *mSampFracNum;
   unsigned int *mMagicSamples;
   
   float *mMem;
   float *mSincTable;
   unsigned int mSincTableLength;
   resampler_basic_func mResamplerPtr;
         
   int    mInStride;
   int    mOutStride;

    double compute_func(float x, struct FuncDef *func);
    float sinc(float cutoff, float x, int N, struct FuncDef *window_func);
    void update_filter();
    void cubic_coef(float frac, float interp[4]);
    int resampler_basic_direct_single(unsigned int channel_index,const float *in, unsigned int *in_len, float *out, unsigned int *out_len);
    int resampler_basic_direct_double(unsigned int channel_index,const float *in, unsigned int *in_len, float *out, unsigned int *out_len);
    int resampler_basic_interpolate_single(unsigned int channel_index,const float *in, unsigned int *in_len, float *out, unsigned int *out_len);
    int resampler_basic_interpolate_double(unsigned int channel_index, const float *in, unsigned int *in_len, float *out, unsigned int *out_len);
    int speex_resampler_magic(unsigned int channel_index, float **out, unsigned int out_len);
    int speex_resampler_process_native(unsigned int channel_index,unsigned int *in_len, float *out, unsigned int *out_len);

public:
    SpeexResampler(unsigned int nb_channels, unsigned int in_rate, unsigned int out_rate, int quality, int *err):
        mDownsampleBw(-1.0f),
        mUpsampleBw(-1.0f),
        mInitialised(0),
        mLastSample(0),
        mSampFracNum(0),
        mMagicSamples(0),
        mMem(0),
        mSincTable(0)
    {
        speex_resampler_init_frac(nb_channels, in_rate, out_rate, in_rate, out_rate, quality, err);
    }

    SpeexResampler(unsigned int nb_channels, unsigned int in_rate, unsigned int out_rate, int quality, int *err, float bw):
        mDownsampleBw(-1.0f),
        mUpsampleBw(-1.0f),
        mInitialised(0),
        mLastSample(0),
        mSampFracNum(0),
        mMagicSamples(0),
        mMem(0),
        mSincTable(0)
    {
        speex_resampler_set_downsample_bw(bw);
        speex_resampler_set_upsample_bw(bw);
        speex_resampler_init_frac(nb_channels, in_rate, out_rate, in_rate, out_rate, quality, err);
    }
       /** Create a new resampler with fractional input/output rates. The sampling 
    * rate ratio is an arbitrary rational number with both the numerator and 
    * denominator being 32-bit integers.
    * @param nb_channels Number of channels to be processed
    * @param ratio_num Numerator of the sampling rate ratio
    * @param ratio_den Denominator of the sampling rate ratio
    * @param in_rate Input sampling rate rounded to the nearest integer (in Hz).
    * @param out_rate Output sampling rate rounded to the nearest integer (in Hz).
    * @param quality Resampling quality between 0 and 10, where 0 has poor quality
    * and 10 has very high quality.
    * @return Newly created resampler state
    * @retval NULL Error: not enough memory
    */
    void speex_resampler_init_frac(unsigned int nb_channels, 
                                               unsigned int ratio_num, 
                                               unsigned int ratio_den, 
                                               unsigned int in_rate, 
                                               unsigned int out_rate, 
                                               int quality,
                                               int *err);

    ~SpeexResampler();  // destructor 
    int InitAsNeeded(int num_chan,int in_rate,int out_rate,int quality,int *err);
    int spResampler(unsigned int channel_index, const short *in, short inLen, short *out, short outLen, bool stereo);
    void rStereoToMono(short *in_stereo, short inLen, short *out);
    /** Resample a float array. The input and output buffers must *not* overlap.
    * @param channel_index Index of the channel to process for the multi-channel 
    * base (0 otherwise)
    * @param in Input buffer
    * @param in_len Number of input samples in the input buffer. Returns the 
    * number of samples processed
    * @param out Output buffer
    * @param out_len Size of the output buffer. Returns the number of samples written
    */
    int speex_resampler_process_float(unsigned int channel_index, 
                                   const float *in, 
                                   unsigned int *in_len, 
                                   float *out, 
                                   unsigned int *out_len);

    /** Resample an int array. The input and output buffers must *not* overlap.
    * @param channel_index Index of the channel to process for the multi-channel 
    * base (0 otherwise)
    * @param in Input buffer
    * @param in_len Number of input samples in the input buffer. Returns the number
    * of samples processed
    * @param out Output buffer
    * @param out_len Size of the output buffer. Returns the number of samples written
    */
#if 0
     int speexResamplerDown(unsigned int channel_index, 
                                 const short *inBuff, 
                                 short inLen, 
                                 short *outBuff, 
                                 short outLen);

     int speexResamplerUp16(unsigned int channel_index, 
                                 const short  *in, 
                                 short  inLen, 
                                 short *out, 
                                 short outLen);
#endif

     int speex_resampler_process_int(unsigned int channel_index, 
                                 const short *in, 
                                 unsigned int *in_len, 
                                 short *out, 
                                 unsigned int *out_len);

    /** Resample an interleaved float array. The input and output buffers must *not* overlap.
    * @param in Input buffer
    * @param in_len Number of input samples in the input buffer. Returns the number
    * of samples processed. This is all per-channel.
    * @param out Output buffer
    * @param out_len Size of the output buffer. Returns the number of samples written.
    * This is all per-channel.
    */
    int speex_resampler_process_interleaved_float(const float *in, 
                                               unsigned int *in_len, 
                                               float *out, 
                                               unsigned int *out_len);

    /** Set (change) the input/output sampling rates (integer value).
    * @param in_rate Input sampling rate (integer number of Hz).
    * @param out_rate Output sampling rate (integer number of Hz).
    */
    int speex_resampler_set_rate(unsigned int in_rate, 
                              unsigned int out_rate);

    /** Get the current input/output sampling rates (integer value).
    * @param in_rate Input sampling rate (integer number of Hz) copied.
    * @param out_rate Output sampling rate (integer number of Hz) copied.
    */
    void speex_resampler_get_rate(unsigned int *in_rate, 
                              unsigned int *out_rate);

    /** Set (change) the input/output sampling rates and resampling ratio 
    * (fractional values in Hz supported).
    * @param ratio_num Numerator of the sampling rate ratio
    * @param ratio_den Denominator of the sampling rate ratio
    * @param in_rate Input sampling rate rounded to the nearest integer (in Hz).
    * @param out_rate Output sampling rate rounded to the nearest integer (in Hz).
    */
    int speex_resampler_set_rate_frac(unsigned int ratio_num, 
                                   unsigned int ratio_den, 
                                   unsigned int in_rate, 
                                   unsigned int out_rate);

    /** Get the current resampling ratio. This will be reduced to the least
    * common denominator.
    * @param ratio_num Numerator of the sampling rate ratio copied
    * @param ratio_den Denominator of the sampling rate ratio copied
    */
    void speex_resampler_get_ratio(unsigned int *ratio_num, unsigned int *ratio_den);

    /** Set (change) the conversion quality.
    * @param quality Resampling quality between 0 and 10, where 0 has poor 
    * quality and 10 has very high quality.
    */
    int speex_resampler_set_quality(int quality);

    /** Get the conversion quality.
    * @param quality Resampling quality between 0 and 10, where 0 has poor 
    * quality and 10 has very high quality.
    */
    void speex_resampler_get_quality(int *quality);

    /** Set (change) the bandwidth of the filter 
    * @param downsample_bw Downsampling bandwidth as percentage of Nyquist 
    * frequency. This will be strictly bounded between 0 and 1.  A higher
    * value is generally desirable, but in certain cases (e.g. PSTN 
    * JIRA:DNM-5883) we set this value lower to reduce distortion and noise.
    */
    int speex_resampler_set_downsample_bw(float downsample_bw);

    /** Set (change) the bandwidth of the filter
    * @param upsample_bw Downsampling bandwidth as percentage of Nyquist 
    * frequency. This will be strictly bounded between 0 and 1.  A higher
    * value is generally desirable, but in certain cases (e.g. PSTN 
    * JIRA:DNM-5883) we set this value lower to reduce distortion and noise.
    */
    int speex_resampler_set_upsample_bw(float upsample_bw);

    /** Get the conversion quality.
    * @param downsample_bw Downsampling bandwidth as percentage of Nyquist 
    * frequency. This will be strictly bounded between 0 and 1.  A higher
    * value is generally desirable, but in certain cases (e.g. PSTN 
    * JIRA:DNM-5883) we set this value lower to reduce distortion and noise.
    */
    int speex_resampler_get_downsample_bw(float& downsample_bw);

    /** Get the bandwidth of the filter 
    * @param upsample_bw upsampling bandwidth as percentage of Nyquist 
    * frequency. This will be strictly bounded between 0 and 1.  A higher
    * value is generally desirable, but in certain cases (e.g. PSTN 
    * JIRA:DNM-5883) we set this value lower to reduce distortion and noise.
    */
    int speex_resampler_get_upsample_bw(float& upsample_bw);


    /** Set (change) the input stride.
    * @param stride Input stride
    */
    void speex_resampler_set_input_stride(unsigned int stride);

    /** Get the input stride.
    * @param stride Input stride copied
    */
    void speex_resampler_get_input_stride(unsigned int *stride);

    /** Set (change) the output stride.
    * @param stride Output stride
    */
    void speex_resampler_set_output_stride(unsigned int stride);

    /** Get the output stride.
    * @param stride Output stride
    */
    void speex_resampler_get_output_stride(unsigned int *stride);

    /** Get the latency in input samples introduced by the resampler.
    */
    int speex_resampler_get_input_latency();

    /** Get the latency in output samples introduced by the resampler.
    */
    int speex_resampler_get_output_latency();

    /** Make sure that the first samples to go out of the resamplers don't have 
    * leading zeros. This is only useful before starting to use a newly created 
    * resampler. It is recommended to use that when resampling an audio file, as
    * it will generate a file with the same length. For real-time processing,
    * it is probably easier not to use this call (so that the output duration
    * is the same for the first frame).
    */
    int speex_resampler_skip_zeros();

    /** Reset a resampler so a new (unrelated) stream can be processed.
    */
    int speex_resampler_reset_mem();

    const char * speex_resampler_strerror(int err);
};

} // namespace SpeexResampler 
#endif
