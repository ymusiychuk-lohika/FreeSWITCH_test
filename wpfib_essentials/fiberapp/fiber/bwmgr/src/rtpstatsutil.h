/******************************************************************************
 * RTP statistics implementation
 *
 * Copyright (C) Bluejeans Network, Inc  All Rights Reserved
 *
 * Author: Emmanuel Weber
 * Date:   02/28/2010
 *****************************************************************************/


#ifndef _BWMGR_RTP_STATS_UTIL_H_
#define _BWMGR_RTP_STATS_UTIL_H_

#if defined(_WIN32)
// If called before <windows.h>
#ifndef NOMINMAX
#define NOMINMAX
#endif

// If called after <windows.h>
#undef max
#undef min
#endif

#include <stdint.h>
#include <math.h>
#include <algorithm>
#include <cassert>
#include <climits>
#include <cstring>
#include <limits>
#include <vector>

namespace bwmgr {

///////////////////////////////////////////////////////////////////////////////
// class RunningMeanAndVariance
// From B. P. Welford and is presented in Donald Knuth's Art of
// Computer Programming, Vol 2, page 232, 3rd edition.

class RunningMeanAndVariance
{
public:
    RunningMeanAndVariance();

    void Clear()
    {
        m_n = 0;
    }

    void Push(double x);

    uint32_t NumDataValues() const
    {
        return m_n;
    }

    double Mean() const
    {
        return (m_n > 0) ? m_newM : 0.0;
    }

    double Variance() const
    {
        return ( (m_n > 1) ? m_newS/(m_n - 1) : 0.0 );
    }

    double StandardDeviation() const
    {
        return sqrt( Variance() );
    }
private:
    double  m_oldM, m_newM, m_oldS, m_newS;
    uint32_t     m_n;
};

template <typename T, size_t SIZE>
void array_memset(T (&buffer)[SIZE])
{
    memset(buffer, 0, sizeof(buffer));
}

// A windowed version of the above class.
template <unsigned WINDOW_SIZE>
class WindowedRunningMeanAndVariance
{
public:
    WindowedRunningMeanAndVariance()
        : mOldM(0.)
        , mOldS(0.)
        , mNewM(0.)
        , mNewS(0.)
        , mNumEntries(0)
        , mNextEntry(0)
    {
        array_memset(mOldMParts);
        array_memset(mOldSParts);
    }

    void push(float val)
    {
        float poppedVal;

        if (mNumEntries < WINDOW_SIZE)
        {
            mNumEntries++;
        }
        else
        {
            poppedVal = mOldMParts[mNextEntry];
            mNewM -= poppedVal;
            poppedVal = mOldSParts[mNextEntry];
            mNewS -= poppedVal;
        }

        mOldMParts[mNextEntry] = (val - mNewM)/mNumEntries;
        mNewM = mNewM + mOldMParts[mNextEntry];
        mOldSParts[mNextEntry] = (val - mOldM)*(val - mNewM);
        mNewS += mOldSParts[mNextEntry];
        mOldM = mNewM;
        mNextEntry++;

        if (mNextEntry == WINDOW_SIZE)
        {
            mNextEntry = 0;
        }
    }

    float mean() const
    {
        return mNumEntries > 0 ? mNewM : 0.f;
    }

    unsigned numEntries() const { return mNumEntries; }

    bool isFull() const { return mNumEntries >= WINDOW_SIZE; }

    float variance() const
    {
        return mNumEntries > 1 ? mNewS/float(mNumEntries - 1) : 0.f;
    }

    float stddev() const
    {
        float var = variance();
        return var >= 0.f ? sqrt(var) : 0.f;
    }

    void reset()
    {
        array_memset(mOldMParts);
        array_memset(mOldSParts);

        mOldM = mOldS = mNewM = mNewS = 0.f;
        mNumEntries = mNextEntry = 0;
    }

private:
    float mOldMParts[WINDOW_SIZE];
    float mOldSParts[WINDOW_SIZE];

    float mOldM;
    float mOldS;

    float mNewM;
    float mNewS;

    /* calculating std dev */
    unsigned mNumEntries;
    unsigned mNextEntry;
};

//
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// class MaxValueEstimator

class MaxValueEstimator
{
public:
    MaxValueEstimator(uint32_t windowSize);
    ~MaxValueEstimator();
    void reset(void);

    void push(double x);

    double getMax(void) const;
    
private:
    double*  mWindow;
    uint32_t mWindowSize;
    uint32_t mNumEntries;
    uint32_t mNextEntry;
    double   mMaxValue;
};

//
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// class WindowedCorrelationCoeffT

template <class T>
class WindowedCorrelationCoeffT
{
public:
	WindowedCorrelationCoeffT(size_t windowSize)
        : mWindowSize(windowSize)
    {
        mXs 		= new T[windowSize];
        mYs 		= new T[windowSize];
        mXYs		= new T[windowSize];
        mXSquares 	= new T[windowSize];
        mYSquares 	= new T[windowSize];

        reset();
    }

    ~WindowedCorrelationCoeffT()
    {
        delete[] mXs;
        delete[] mYs;
        delete[] mXYs;
        delete[] mXSquares;
        delete[] mYSquares;
    }

    void reset(void)
    {
        mNumEntries = 0;
        mNextEntry  = 0;

        mSumOfXs      = (T)0;
        mSumOfYs      = (T)0;
        mSumOfXYs      = (T)0;
        mSumOfXSquares      = (T)0;
        mSumOfYSquares      = (T)0;

        mSlope = 0.0;
    }

    void push(T x, T y)
    {
        T poppedVal;

        if (mNumEntries < mWindowSize)
        {
            mNumEntries++;
        }
        else
        {
            poppedVal = mXs[mNextEntry];
            mSumOfXs -= poppedVal;

            poppedVal = mYs[mNextEntry];
            mSumOfYs -= poppedVal;

            poppedVal = mXYs[mNextEntry];
            mSumOfXYs -= poppedVal;

            poppedVal = mXSquares[mNextEntry];
            mSumOfXSquares -= poppedVal;

            poppedVal = mYSquares[mNextEntry];
            mSumOfYSquares -= poppedVal;
        }

        mXs[mNextEntry] = x;
        mYs[mNextEntry] = y;
        mXYs[mNextEntry] = (x * y);
        mXSquares[mNextEntry] = (x * x);
        mYSquares[mNextEntry] = (y * y);

        mSumOfXs += mXs[mNextEntry];
        mSumOfYs += mYs[mNextEntry];
        mSumOfXYs += mXYs[mNextEntry];
        mSumOfXSquares += mXSquares[mNextEntry];
        mSumOfYSquares += mYSquares[mNextEntry];

        mNextEntry++;
        if(mNextEntry == mWindowSize)
        {
            mNextEntry = 0;
        }
    }

    double getCorrelationCoeff(void) const
    {
    	if (mNumEntries == 0)
    	{
    		return 0.0;
    	}
    	else if (mNumEntries == 1)
    	{
    		return 1.0;
    	}
    	else {
        	double retval = 0.0;

			// r = (n(sum of xy) - (sum of x)(sum of y)) / sqrt((n(sum of x^2) - (sum of x)^2)(n(sum of y^2) - (sum of y)^2))

			T dempart1 = ((mNumEntries * mSumOfXSquares) - (mSumOfXs * mSumOfXs));
			T dempart2 = ((mNumEntries * mSumOfYSquares) - (mSumOfYs * mSumOfYs));
			T denominator = dempart1 * dempart2;
			if (denominator != (T)0)
			{
				 retval = (double)((mNumEntries * mSumOfXYs) - (mSumOfXs * mSumOfYs)) / (double)sqrt(denominator);
			}

			return retval;
		}
    }

    double getSlope(void) const
    {
    	double slope = 0.0;

        // (NΣXY - (ΣX)(ΣY)) / (NΣX2 - (ΣX)2)
        T denominator = ((mNumEntries * mSumOfXSquares) - (mSumOfXs * mSumOfXs));
        if (denominator != (T)0)
        {
        	slope = (double)((mNumEntries * mSumOfXYs) - (mSumOfXs * mSumOfYs)) / (double)(denominator);
        }

        return slope;
    }

    double getAverageX(void) const
    {
    	return (double)(mSumOfXs/(T)mNumEntries);
    }

    double getAverageY(void) const
    {
    	return (double)(mSumOfYs/(T)mNumEntries);
    }

    unsigned getNumEntries(void) const
    {
        return mNumEntries;
    }

    bool isFull(void) const
    {
        return (mNumEntries >= mWindowSize);
    }

private:
    T*  mXs;
    T*  mYs;
    T*  mXYs;
    T*  mXSquares;
    T*  mYSquares;

    T   mSumOfXs;
    T   mSumOfYs;
    T   mSumOfXYs;
    T   mSumOfXSquares;
    T   mSumOfYSquares;

    double	mSlope;

    size_t mWindowSize;
    size_t mNumEntries;
    size_t mNextEntry;
};


//
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// template WindowedAverageT

template <class T>
class WindowedAverageT
{
public:
    WindowedAverageT(uint32_t windowSize)
        : mWindowSize(windowSize)
    {
        mWindow = new T[windowSize];
        reset();
    }

    ~WindowedAverageT()
    {
        if(mWindow)
            delete[] mWindow;
    }

    void reset(void)
    {
        mNumEntries = 0;
        mNextEntry  = 0;

        mMinValue = std::numeric_limits<T>::max();
        mMaxValue = std::numeric_limits<T>::min();
    }

    void push(T x)
    {
        mWindow[mNextEntry++] = x;
        if(mNextEntry == mWindowSize)
        {
            mNextEntry = 0;
        }
        mNumEntries++;
    }

    T getAverage(void) const
    {
        T total = (T)0;
        uint32_t maxIndex = std::min<uint32_t>(mNumEntries,mWindowSize);
        if(maxIndex > 0)
        {
            for(uint32_t i=0;i<maxIndex;i++)
            {
                total += mWindow[i];
            }
            total/=maxIndex;
        }
        return total;
    }

    void getMinMax(T& minval, T& maxval)
    {
        uint32_t maxIndex = std::min<uint32_t>(mNumEntries,mWindowSize);
        if(maxIndex > 0)
        {
            // Just recalculate it.
            mMinValue = std::numeric_limits<T>::max();
            mMaxValue = std::numeric_limits<T>::min();
            for (uint32_t i = 0; i < maxIndex; ++i)
            {
                if (mWindow[i] < mMinValue)
                {
                    mMinValue = mWindow[i];
                }
                if (mWindow[i] > mMaxValue)
                {
                    mMaxValue = mWindow[i];
                }
            }
        }

        minval = mMinValue;
        maxval = mMaxValue;
    }

    bool isFull(void) const
    {
    	return (mNumEntries >= mWindowSize);
    }

    bool isEmpty() const
    {
        return mNumEntries == 0;
    }

private:
    T*  mWindow;
    T   mMinValue;
    T   mMaxValue;
    uint32_t mWindowSize;
    uint32_t mNumEntries;
    uint32_t mNextEntry;
};

//
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// class WindowedAverage

typedef  WindowedAverageT<double> WindowedAverage;

//
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// class PacketSizeEstimator

class PacketSizeEstimator
{
public:
    PacketSizeEstimator();
    void reset(void);

    void handleNewPacket(unsigned char* pkt, int pktlen);

    double getEstimatedPacketSize(bool highBound) const;
    double getEstimatedPayloadSize(bool highBound) const;

    uint64_t getCumulativePayloadSize(void) const
    {
        return mCumulativePayloadSize;
    }

    uint64_t getCumulativePacketSize(void) const
    {
        return mCumulativePacketSize;
    }
    
private:
    RunningMeanAndVariance mStatsPayload;
    RunningMeanAndVariance mStatsFull;
    uint64_t               mCumulativePayloadSize;
    uint64_t               mCumulativePacketSize;
};

//
///////////////////////////////////////////////////////////////////////////////

// Just the classic recursive average/low pass filter.
class Filter
{
public:
    Filter(float alpha)
        : m_Alpha(alpha)
        , m_Value(0)
    {
        assert(alpha > 0 && alpha <= 1.);
    }

    void SetAlpha(float alpha)
    {
        assert(alpha > 0 && alpha <= 1.);
        m_Alpha = alpha;
    }

    int32_t GetValue() const { return (int32_t)(m_Value + 0.5f); }
    void    SetValue(int32_t newVal) { m_Value = (float)newVal; }

    int32_t Update(int32_t newVal)
    {
        m_Value += m_Alpha * (newVal - m_Value);
        return GetValue();
    }

private:
    float m_Alpha;
    float m_Value;
};

// A recursive average/filter with attack and release alphas
class TwoAlphaSmoother
{
public:
    TwoAlphaSmoother(float atk, float rel)
        : mAttack(atk)
        , mRelease(rel)
        , mValue(0.f)
    {
        assert(atk > 0 && atk <= 1.f);
        assert(rel > 0 && rel <= 1.f);
    }

    void SetAlpha(float atk, float rel)
    {
        assert(atk > 0 && atk <= 1.f);
        assert(rel > 0 && rel <= 1.f);
    }

    float GetValue() const  { return mValue; }
    void SetValue(float newVal) { mValue = newVal; }


    float Update(float newVal)
    {
        float alpha = newVal > mValue ? mAttack : mRelease;
        mValue += alpha * (newVal - mValue);
        return mValue;
    }
private:
    float mAttack;
    float mRelease;
    float mValue;
};

// special case of a simple linear regression, in which the x variable is nothing more than
// a sampling interval (1, 2, 3, 4, ...).  Should be fairly efficient if the window is short
class WindowedSingleVarRegression
{
public:
    WindowedSingleVarRegression(int windowLen);
    ~WindowedSingleVarRegression();

    void update(int y);

    int mean() const;

    bool full() const
    {
        return static_cast<int>(mY.size()) == mWindowLen;
    }

    void reset();

    float slope() const;

private:
    std::vector<int> mY; // treated as a circular buffer

    // we're using constant values for the x variables.  These are precalculated
    // denominators, but also inverted to avoid divides at runtime
    std::vector<float> m1_SlopeDenominators;
    std::vector<int> mSX;

    int mWindowLen;
    unsigned mHeadPos;
    int mSY; // Sum of the Y's
    int mSXY; // Sum of the X*Y's
};

template <int MINVAL, int MAXVAL, int STEPSIZE>
class BasicHistogram
{
public:
    enum { NUMBINS = ((MAXVAL-MINVAL)/STEPSIZE)+2 };

    // A basic histogram that includes bins from min/max, with two extra bins for
    // those outside the range.
    // numBins is desired bin spacing ((max - min)/desiredSpacing) + 2.
    BasicHistogram()
        : mCount(0)
        , mMaxSeen(INT_MIN)
        , mMinSeen(INT_MAX)
    {
        assert(MINVAL <= MAXVAL);
        assert(NUMBINS >= 2);
        reset();
    }

    void insert(int value)
    {
        mCount++;
        if (value < mMinSeen)
            mMinSeen = value;
        if (value > mMaxSeen)
            mMaxSeen = value;
        unsigned idx = getIndex(value);
        mBins[idx]++;
    }

    // returns what could best be described as a lazy median
    int median() const
    {
        int numElems = mCount;
        if (numElems == 0)
            return (MAXVAL-MINVAL)/2 + MINVAL;
        int sum = 0;

        const int halfWay = numElems/2;
        unsigned idx = getIndex(mMinSeen);
        for (; idx < NUMBINS; ++idx)
        {
            sum += mBins[idx];
            if (sum >= halfWay)
                break;
        }

        return MINVAL + static_cast<int>(STEPSIZE*(idx-1)) + STEPSIZE/2;
    }

    // num total elements
    unsigned count() const { return mCount; }

    // num elements in binOf(val)
    unsigned count(int val) const { return mBins[getIndex(val)]; }

    // Density of values from [min, binOf(val)].
    float incDensity(int val) const
    {
        int numElems = mCount;
        if (numElems == 0)
            return 0.f;
        unsigned endIdx = getIndex(val);
        int sum = 0;
        for (unsigned i = getIndex(mMinSeen); i <= endIdx; ++i)
        {
            sum += mBins[i];
        }
        return (float)sum / numElems;
    }

    // Density of values from [binOf(val), max].
    float decDensity(int val) const
    {
        int numElems = mCount;
        if (numElems == 0)
            return 0.f;
        unsigned startIdx = getIndex(val);
        int sum = 0;
        for (unsigned i = startIdx; i < NUMBINS; ++i)
        {
            sum += mBins[i];
        }
        return (float)sum / numElems;
    }

    // Density within val's bin
    float valDensity(int val) const
    {
        int numElems = count();
        if (numElems == 0)
            return 0.f;
        unsigned idx = getIndex(val);
        return mBins[idx] / (float)numElems;
    }

    int minSeen() const { return mMinSeen; }
    int maxSeen() const { return mMaxSeen; }

    void reset()
    {
        // set all bins to zero
        memset(mBins, 0, sizeof(mBins));
        mCount = 0;
        mMinSeen = INT_MAX;
        mMaxSeen = INT_MIN;
    }

private:
    unsigned getIndex(int val) const
    {
        if (val < MINVAL)
            return 0;
        if (val >= MAXVAL)
            return NUMBINS - 1;
        int relVal = val - MINVAL;
        relVal /= STEPSIZE;
        return relVal + 1;
    }

    int mBins[NUMBINS]; // Histogram bins.  The first/last elems contain vals beyond min/max range
    unsigned mCount; // num elems stored
    int mMaxSeen;    // min element seen
    int mMinSeen;    // max element seen
};

// This wraps the BasicHistogram to essentially overlap them, issuing resets
// on an interval
template <int MINVAL, int MAXVAL, int STEPSIZE>
class OverlappedHistogram
{
public:
    // swapInterval is the interval we'll swap (effective history len is 2*interval)
    OverlappedHistogram(unsigned swapInterval)
        : mActive(&mFirst)
        , mSwapInterval(swapInterval)
        , mSwapCount(0)
    {
    }

    void insert(int value)
    {
        if (mSwapCount == mSwapInterval)
        {
            mActive->reset();
            mActive = mActive == &mFirst ? &mSecond : &mFirst;
            mSwapCount = 0;
        }

        mFirst.insert(value);
        mSecond.insert(value);
        ++mSwapCount;
    }

    int median() const { return mActive->median(); }
    unsigned count() const { return mActive->count(); }
    float incDensity(int val) const { return mActive->incDensity(val); }
    float decDensity(int val) const { return mActive->decDensity(val); }
    float valDensity(int val) const { return mActive->valDensity(val); }

    int minSeen() const { return mActive->minSeen(); }
    int maxSeen() const { return mActive->maxSeen(); }

    void reset()
    {
        mFirst.reset();
        mSecond.reset();
        mSwapCount = 0;
    }

private:
    typedef BasicHistogram<MINVAL, MAXVAL, STEPSIZE> HistogramType;
    HistogramType mFirst;
    HistogramType mSecond;

    HistogramType *mActive; // will toggle between mFirst and mSecond
    const unsigned mSwapInterval;
    unsigned mSwapCount;
};

}

#endif // _RTP_STATS_H_
