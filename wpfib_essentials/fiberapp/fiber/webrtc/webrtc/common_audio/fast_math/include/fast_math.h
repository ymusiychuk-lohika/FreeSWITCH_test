/*=====================================================================*
 *                   Copyright (C) 2011 Paul Mineiro                   *
 * All rights reserved.                                                *
 *                                                                     *
 * Redistribution and use in source and binary forms, with             *
 * or without modification, are permitted provided that the            *
 * following conditions are met:                                       *
 *                                                                     *
 *     * Redistributions of source code must retain the                *
 *     above copyright notice, this list of conditions and             *
 *     the following disclaimer.                                       *
 *                                                                     *
 *     * Redistributions in binary form must reproduce the             *
 *     above copyright notice, this list of conditions and             *
 *     the following disclaimer in the documentation and/or            *
 *     other materials provided with the distribution.                 *
 *                                                                     *
 *     * Neither the name of Paul Mineiro nor the names                *
 *     of other contributors may be used to endorse or promote         *
 *     products derived from this software without specific            *
 *     prior written permission.                                       *
 *                                                                     *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND              *
 * CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,         *
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES               *
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE             *
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER               *
 * OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,                 *
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES            *
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE           *
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR                *
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF          *
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT           *
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY              *
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE             *
 * POSSIBILITY OF SUCH DAMAGE.                                         *
 *                                                                     *
 * Contact: Paul Mineiro <paul@mineiro.com>                            *
 *=====================================================================*/
#ifndef __FAST_MATH_HDR__
#define __FAST_MATH_HDR__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

// Remapping of commoon functions into fast math equivalents
#define powf(a, b) fasterpow(a, b)
#define log10f(a) .301030f*fasterlog2(a)
#define log10(a) log10f(a)
#define log(a) fasterlog(a)
#define exp(a) fasterexp(a)
#define sqrt(a) sqrtf(a)

// next remapping (used in audio_dsp.cpp) will cause audioDropping in Sky3992
// #define expf(a) fasterexp(a)
// so use the standard math instead:
float expf(float a);

// TODO: INCOMPLETE math.h functions not implemented by fast math library 
// prototypes included here to link against fast_math.cc's include of cmath
float fmaxf(float a, float b);
double fabs(double x);
double tanh(double x);
float sqrtf(float x);
double pow(double a, double b);
double floor(double x);
float cosf(float a);
float sinf(float a);
    
// TODO (brant) These opts are unused.
#ifdef __SSE2__
#include <emmintrin.h>

#ifdef __cplusplus
namespace {
#endif // __cplusplus

typedef __m128 v4sf;
typedef __m128i v4si;

#define v4si_to_v4sf _mm_cvtepi32_ps
#define v4sf_to_v4si _mm_cvttps_epi32

#define v4sfl(x) ((const v4sf) { (x), (x), (x), (x) })
#define v2dil(x) ((const v4si) { (x), (x) })
#define v4sil(x) v2dil((((unsigned long long) (x)) << 32) | (x))

typedef union { v4sf f; float array[4]; } v4sfindexer;
#define v4sf_index(_findx, _findi)      \
  ({                                    \
     v4sfindexer _findvx = { _findx } ; \
     _findvx.array[_findi];             \
  })
typedef union { v4si i; int array[4]; } v4siindexer;
#define v4si_index(_iindx, _iindi)      \
  ({                                    \
     v4siindexer _iindvx = { _iindx } ; \
     _iindvx.array[_iindi];             \
  })

typedef union { v4sf f; v4si i; } v4sfv4sipun;
#define v4sf_fabs(x)                    \
  ({                                    \
     v4sfv4sipun vx = { x };            \
     vx.i &= v4sil (0x7FFFFFFF);        \
     vx.f;                              \
  })

#ifdef __cplusplus
} // end namespace
#endif // __cplusplus

#endif // __SSE2__

float
fastpow2 (float p);

float
fastexp (float p);

float
fasterpow2 (float p);

float
fasterexp (float p);

#ifdef __SSE2__

v4sf
vfastpow2 (const v4sf p);

v4sf
vfastexp (const v4sf p);

v4sf
vfasterpow2 (const v4sf p);

v4sf
vfasterexp (const v4sf p);

#endif //__SSE2__

float 
fastlog2 (float x);

float
fastlog (float x);

float 
fasterlog2 (float x);

float
fasterlog (float x);

#ifdef __SSE2__

v4sf
vfastlog2 (v4sf x);

v4sf
vfastlog (v4sf x);

v4sf 
vfasterlog2 (v4sf x);

v4sf
vfasterlog (v4sf x);

#endif // __SSE2__

// fasterfc: not actually faster than erfcf(3) on newer machines!
// ... although vectorized version is interesting
//     and fastererfc is very fast

float
fasterfc (float x);

float
fastererfc (float x);

// fasterf: not actually faster than erff(3) on newer machines! 
// ... although vectorized version is interesting
//     and fastererf is very fast
float
fasterf (float x);

float
fastererf (float x);

float
fastinverseerf (float x);

float
fasterinverseerf (float x);

#ifdef __SSE2__

v4sf
vfasterfc (v4sf x);

v4sf
vfastererfc (const v4sf x);

v4sf
vfasterf (v4sf x);

v4sf
vfastererf (const v4sf x);

v4sf
vfastinverseerf (v4sf x);

v4sf
vfasterinverseerf (v4sf x);

#endif //__SSE2__

/* gamma/digamma functions only work for positive inputs */
float
fastlgamma (float x);

float
fasterlgamma (float x);

float
fastdigamma (float x);

float
fasterdigamma (float x);

#ifdef __SSE2__

v4sf
vfastlgamma (v4sf x);

v4sf
vfasterlgamma (v4sf x);

v4sf
vfastdigamma (v4sf x);

v4sf
vfasterdigamma (v4sf x);

#endif //__SSE2__

float
fastsinh (float p);

float
fastersinh (float p);

float
fastcosh (float p);

float
fastercosh (float p);

float
fasttanh (float p);

float
fastertanh (float p);

#ifdef __SSE2__

v4sf
vfastsinh (const v4sf p);

v4sf
vfastersinh (const v4sf p);

v4sf
vfastcosh (const v4sf p);

v4sf
vfastercosh (const v4sf p);

v4sf
vfasttanh (const v4sf p);

v4sf
vfastertanh (const v4sf p);

#endif //__SSE2__

// these functions compute the upper branch aka W_0
float
fastlambertw (float x);

float
fasterlambertw (float x);

float
fastlambertwexpx (float x);

float
fasterlambertwexpx (float x);

#ifdef __SSE2__

v4sf
vfastlambertw (v4sf x);

v4sf
vfasterlambertw (v4sf x);

v4sf
vfastlambertwexpx (v4sf x);

v4sf
vfasterlambertwexpx (v4sf x);

#endif // __SSE2__

float
fastpow (float x,
         float p);

float
fasterpow (float x,
           float p);

#ifdef __SSE2__

v4sf
vfastpow (const v4sf x,
          const v4sf p);

v4sf
vfasterpow (const v4sf x,
            const v4sf p);

#endif //__SSE2__

float
fastsigmoid (float x);

float
fastersigmoid (float x);

#ifdef __SSE2__

v4sf
vfastsigmoid (const v4sf x);

v4sf
vfastersigmoid (const v4sf x);

#endif //__SSE2__

// http://www.devmaster.net/forums/showthread.php?t=5784
// fast sine variants are for x \in [ -\pi, pi ]
// fast cosine variants are for x \in [ -\pi, pi ]
// fast tangent variants are for x \in [ -\pi / 2, pi / 2 ]
// "full" versions of functions handle the entire range of inputs
// although the range reduction technique used here will be hopelessly
// inaccurate for |x| >> 1000
//
// WARNING: fastsinfull, fastcosfull, and fasttanfull can be slower than
// libc calls on older machines (!) and on newer machines are only 
// slighly faster.  however:
//   * vectorized versions are competitive
//   * faster full versions are competitive

float
fastsin (float x);

float
fastersin (float x);

float
fastsinfull (float x);

float
fastersinfull (float x);

float
fastcos (float x);

float
fastercos (float x);

float
fastcosfull (float x);

float
fastercosfull (float x);

float
fasttan (float x);

float
fastertan (float x);

float
fasttanfull (float x);

float
fastertanfull (float x);

// TODO (brant) Building this code in release mode on Redhat causes a compiler
// segfault.  Debug works fine. Dunno. Weirdness.
/*
#ifdef __SSE2__

v4sf
vfastsin (const v4sf x);

v4sf
vfastersin (const v4sf x);

v4sf
vfastsinfull (const v4sf x);

v4sf
vfastersinfull (const v4sf x);

v4sf
vfastcos (const v4sf x);

v4sf
vfastercos (v4sf x);

v4sf
vfastcosfull (const v4sf x);

v4sf
vfastercosfull (const v4sf x);

v4sf
vfasttan (const v4sf x);

v4sf
vfastertan (const v4sf x);

v4sf
vfasttanfull (const v4sf x);

v4sf
vfastertanfull (const v4sf x);

#endif // SSE
*/

#ifdef __cplusplus
}
#endif

#endif //EOF: __FAST_MATH_HDR__
