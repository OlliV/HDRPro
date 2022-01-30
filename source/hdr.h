#pragma once

#include "fx/fxobjects.h"
#include "delay.h"
#include "dcblock.h"
#include "paramids.h"

namespace MyVst {
using namespace Steinberg::Vst;

#define HDR_DELAY_MS    15.0
#define HDR_DET_ATTACK  0.1
#define HDR_DET_RELEASE HDR_DELAY_MS

template <typename SampleType>
class HDR {
public:
    /*
     * Normalized parameters tuned directly by the user.
     */
    SampleType gain;
    SampleType hi_max;
    SampleType lo_min;

    void updateParams(void);
    void reset(float sampleRate);
    SampleType process(SampleType xn_hi, SampleType xn_lo);


    bool get_sel_lo(void)
    {
        return proc.sel_lo;
    }
    unsigned get_nlookahead(void)
    {
        return cooked.nlookahead;
    }

private:
    struct {
        SampleType gain;
        SampleType hi_max;
        SampleType lo_min;
        unsigned nlookahead;
    } cooked;

    struct {
        bool sel_lo;
        SampleType offset_hi = 0.0;
        SampleType offset_lo = 0.0;
    } proc;

    AudioDetector det_hi;
    AudioDetector det_lo;
    Delay<SampleType, 4096> delay_hi;
    Delay<SampleType, 4096> delay_lo;
    DCBlock<SampleType> dcblock;
};

template <typename SampleType>
void HDR<SampleType>::updateParams(void)
{
    cooked.gain = normdb2factor(gain, HDR_GAIN_MIN, HDR_GAIN_MAX);
    cooked.hi_max = PLAIN(hi_max, HDR_24B_FS_MIN, HDR_GAIN_MAX);
    cooked.lo_min = PLAIN(lo_min, HDR_24B_FS_MIN, HDR_GAIN_MAX);
}

template <typename SampleType>
void HDR<SampleType>::reset(float sampleRate)
{
    struct AudioDetectorParameters det_hi_params;
    struct AudioDetectorParameters det_lo_params;

    det_hi_params.attackTime_mSec = HDR_DET_ATTACK;
    det_hi_params.releaseTime_mSec = HDR_DET_RELEASE;
    det_hi_params.detectMode = TLD_AUDIO_DETECT_MODE_PEAK;
    det_hi_params.detect_dB = true;
    det_hi_params.clampToUnityMax = true;
    det_hi.setParameters(det_hi_params);
    det_hi.reset(sampleRate);

    det_lo_params.attackTime_mSec = HDR_DET_ATTACK;
    det_lo_params.releaseTime_mSec = HDR_DET_RELEASE;
    det_lo_params.detectMode = TLD_AUDIO_DETECT_MODE_RMS;
    det_lo_params.detect_dB = true;
    det_lo_params.clampToUnityMax = true;
    det_lo.setParameters(det_lo_params);
    det_lo.reset(sampleRate);

    delay_hi.set(sampleRate, HDR_DELAY_MS);
    delay_hi.reset();
    delay_lo.set(sampleRate, HDR_DELAY_MS);
    delay_lo.reset();
    dcblock.reset(sampleRate);
    cooked.nlookahead = (unsigned)round(HDR_DELAY_MS * (sampleRate / 1000.0));

    proc.sel_lo = false;
    proc.offset_hi = 0.0;
    proc.offset_lo = 0.0;
}

template <typename SampleType>
SampleType HDR<SampleType>::process(SampleType xn_hi, SampleType xn_lo)
{
    SampleType yn_hi;
    SampleType yn_lo;
    SampleType hi_level;
    SampleType lo_level;

    yn_hi = delay_hi.process(xn_hi);
    yn_lo = delay_lo.process(xn_lo) * cooked.gain;

    hi_level = det_hi.processAudioSample(xn_hi);
    lo_level = det_lo.processAudioSample(xn_lo);
    //printf("hi_level: %f %f\n", (double)xn_hi, (double)hi_level);

    if (proc.sel_lo) {
        if (lo_level < cooked.lo_min) {
            proc.sel_lo = false;
            proc.offset_hi = yn_lo - yn_hi;
        }
    } else {
        if (hi_level > cooked.hi_max) {
            proc.sel_lo = true;
            proc.offset_lo = yn_hi - yn_lo;
        }
    }

    // The dcblock cuts too much of the low frequencies and removes the DC
    // offset too aggressively.
    //return dcblock.process((proc.sel_lo) ? yn_lo + proc.offset_lo : yn_hi + proc.offset_hi);
    return (proc.sel_lo) ? yn_lo + proc.offset_lo : yn_hi + proc.offset_hi;
}

}
