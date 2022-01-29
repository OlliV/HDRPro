#pragma once

#include "fx/fxobjects.h"
#include "delay.h"
#include "dcblock.h"
#include "paramids.h"

namespace MyVst {
using namespace Steinberg::Vst;

#define HDR_DELAY_MS    5
#define HDR_DET_ATTACK  0.0
#define HDR_DET_RELEASE HDR_DELAY_MS

template <typename SampleType>
class HDR {
public:
    /*
     * Normalized parameters tuned directly by the user.
     */
    SampleType gain;
    SampleType pitch;
    SampleType boostGain;
    SampleType boostFc;
    SampleType blend;

    void updateParams(void);
    void reset(float sampleRate);
    void process(SampleType* buf, int nrSamples);
private:
    struct {
        SampleType gain;
    } cooked;

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
}

template <typename SampleType>
void Demon<SampleType>::reset(float sampleRate)
{
    struct AudioDetectorParameters det_hi_params;
    struct AudioDetectorParameters det_lo_params;

    det_hi_params.attackTime_mSec = HDR_DET_ATTACK;
    det_hi_params.release_in_ms = HDR_DET_RELEASE;
    det_hi_params.detectMode = TLD_AUDIO_DETECT_MODE_PEAK;
    det_hi_params.detect_dB = true;
    det_hi_params.clampToUnityMax = false;
    det_hi.setParameters(det_hi_params);
    det_hi.reset(sampleRate);

    det_lo_params.attackTime_mSec = HDR_DET_ATTACK;
    det_lo_params.release_in_ms = HDR_DET_RELEASE;
    det_lo_params.detectMode = TLD_AUDIO_DETECT_MODE_RMS;
    det_lo_params.detect_dB = true;
    det_lo_params.clampToUnityMax = false;
    det_lo.setParameters(det_lo_params);
    det_lo.reset(sampleRate);

    delay_hi.set(sampleRate, HDR_DELAY_MS);
    delay_hi.reset();
    delay_lo.set(sampleRate, HDR_DELAY_MS);
    delay_lo.reset();
    dcblock.reset(sampleRate);
}

template <typename SampleType>
SampleType Demon<SampleType>::process(SampleType xn_hi, SampleType xn_lo)
{
    SampleType yn_hi;
    SampleType yn_lo;
    SampleType hi_level;
    SampleType lo_level;

    yn_hi = delay_hi.process(xn_hi);
    yn_lo = delay_lo.process(xn_lo) * proc.gain;

    hi_level = det_hi.process(xn_hi);
    lo_level = det_lo.process(xn_lo);

    if (proc.lo_sel) {
        if (lo_level < lo_min) {
            proc.sel_lo = false;
            proc.offset_hi = yn_lo - yn_hi;
        }
    } else {
        if (hi_level > hi_max) {
            proc.sel_lo = true;
            proc.offset_lo = yn_hi - yn_lo;
        }
    }

    return dcblock.process((proc.sel_lo) ? yn_lo + proc.offset_lo : yn_hi + proc.offset_hi);
}

}
