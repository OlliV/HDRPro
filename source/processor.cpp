//------------------------------------------------------------------------
// Copyright(c) 2021-2022 Olli Vanhoja.
//------------------------------------------------------------------------

#include "processor.h"
#include "cids.h"
#include "paramids.h"

#include "base/source/fstreamer.h"
#include "public.sdk/source/vst/vstaudioprocessoralgo.h"
#include "pluginterfaces/vst/ivstparameterchanges.h"

namespace MyVst {
using namespace Steinberg;
using namespace Steinberg::Vst;

HDRProProcessor::HDRProProcessor ()
{
    //--- set the wanted controller for our processor
    setControllerClass (kHDRProControllerUID);
}

HDRProProcessor::~HDRProProcessor ()
{}

tresult PLUGIN_API HDRProProcessor::initialize (FUnknown* context)
{
    //---always initialize the parent-------
    tresult result = AudioEffect::initialize (context);
    if (result != kResultOk) {
        return result;
    }

    hdr32.gain = HDR_GAIN_DEFAULT_N;
    hdr32.updateParams();
    hdr64.gain = HDR_GAIN_DEFAULT_N;
    hdr64.updateParams();

    addAudioInput(STR16("Stereo In"), Steinberg::Vst::SpeakerArr::kStereo);
    addAudioOutput(STR16("Stereo Out"), Steinberg::Vst::SpeakerArr::kStereo);
    addEventInput(STR16("Event In"), 1);

    return kResultOk;
}

tresult PLUGIN_API HDRProProcessor::terminate ()
{
    return AudioEffect::terminate();
}

tresult PLUGIN_API HDRProProcessor::setActive (TBool state)
{
	//--- called when the Plug-in is enable/disable (On/Off) -----
    if (state) {
        float sampleRate = (float)this->processSetup.sampleRate;

        hdr32.reset(sampleRate);
        hdr32.updateParams();
        hdr64.reset(sampleRate);
        hdr64.updateParams();
    }

	return AudioEffect::setActive(state);
}

void HDRProProcessor::handleParamChanges(IParameterChanges* paramChanges)
{
	int32 numParamsChanged = paramChanges->getParameterCount();

    for (int32 i = 0; i < numParamsChanged; i++) {
        IParamValueQueue* paramQueue = paramChanges->getParameterData(i);

        if (paramQueue) {
            ParamValue value;
            int32 sampleOffset;
            int32 numPoints = paramQueue->getPointCount();

            switch (paramQueue->getParameterId()) {
                case kBypassId:
                    if (paramQueue->getPoint(numPoints - 1, sampleOffset, value) == kResultTrue) {
                        bBypass = value > 0.5f;
                    }
                    break;

                case kHDRGainId:
                    if (paramQueue->getPoint(numPoints - 1, sampleOffset, value) == kResultTrue) {
                        hdr32.gain = value;
                        hdr64.gain = value;
                    }
                    break;
                case kHDRHiMaxId:
                    if (paramQueue->getPoint(numPoints - 1, sampleOffset, value) == kResultTrue) {
                        hdr32.hi_max = value;
                        hdr64.hi_max = value;
                    }
                    break;
                case kHDRLoMinId:
                    if (paramQueue->getPoint(numPoints - 1, sampleOffset, value) == kResultTrue) {
                        hdr32.lo_min = value;
                        hdr64.lo_min = value;
                    }
                    break;
            }
        }

        if (numParamsChanged) {
            hdr32.updateParams();
            hdr64.updateParams();
        }
    }
}

tresult PLUGIN_API HDRProProcessor::process (Vst::ProcessData& data)
{
	/*
     * Read inputs parameter changes.
     */
    IParameterChanges* paramChanges = data.inputParameterChanges;
    if (paramChanges) {
        handleParamChanges(paramChanges);
    }

    /*
     * Processing
     */

    if (data.numInputs != 1 || data.numOutputs != 1) {
        return kResultOk;
    }

    void** in = getChannelBuffersPointer(processSetup, data.inputs[0]);
    void** out = getChannelBuffersPointer(processSetup, data.outputs[0]);
    const int nrSamples = data.numSamples;
    const size_t sampleFramesSize = getSampleFramesSizeInBytes(processSetup, nrSamples);

    if (data.inputs[0].silenceFlags) {
        data.outputs[0].silenceFlags = data.inputs[0].silenceFlags;

        memset(out[0], 0, sampleFramesSize);
        memset(out[1], 0, sampleFramesSize);

        return kResultOk;
    }

    // Normally the output is not silenced
    data.outputs[0].silenceFlags = 0;

    if (bBypass) {
        memmove(out[0], in[0], sampleFramesSize);
        memmove(out[1], in[1], sampleFramesSize);

        for (int i = 0; i < nrSamples; i++) {
            if (data.symbolicSampleSize == kSample32) {
                hdr32.process(0.0, 0.0);
            } else {
                hdr64.process(0.0, 0.0);
            }
        }

        return kResultOk;
    }

    // TODO Remove
        memmove(in[1], in[0], sampleFramesSize);
    if (data.symbolicSampleSize == kSample32) {
        Sample32 *hi = ((Sample32**)in)[0];
        Sample32 *lo = ((Sample32**)in)[1];
        Sample32 *l_out = ((Sample32 **)out)[0];

        for (int i = 0; i < nrSamples; i++) {
            l_out[i] = hdr32.process(hi[i], lo[i]);
        }
    } else {
        Sample64 *hi = ((Sample64**)in)[0];
        Sample64 *lo = ((Sample64**)in)[1];
        Sample64 *l_out = ((Sample64**)out)[0];

        for (int i = 0; i < nrSamples; i++) {
            l_out[i] = hdr64.process(hi[i], lo[i]);
        }
    }

    // Copy L -> R
    memcpy(out[1], out[0], sampleFramesSize);

    IParameterChanges* outParamChanges = data.outputParameterChanges;
    if (outParamChanges) {
        const float sel_lo = ((data.symbolicSampleSize == kSample32) ? hdr32.get_sel_lo() : hdr64.get_sel_lo()) ? 1.0 : 0.0;
        int32 index = 0;
        IParamValueQueue* paramQueue;

        paramQueue = outParamChanges->addParameterData(kHDRSelLoId, index);
        if (paramQueue) {
            paramQueue->addPoint(0, sel_lo, index);
        }
    }

	return kResultOk;
}

tresult PLUGIN_API HDRProProcessor::setupProcessing (Vst::ProcessSetup& newSetup)
{
	//--- called before any processing ----
	return AudioEffect::setupProcessing(newSetup);
}

tresult PLUGIN_API HDRProProcessor::canProcessSampleSize (int32 symbolicSampleSize)
{
	if (symbolicSampleSize == Vst::kSample32 || symbolicSampleSize == Vst::kSample64)
		return kResultTrue;

	return kResultFalse;
}

tresult PLUGIN_API HDRProProcessor::setBusArrangements (Steinberg::Vst::SpeakerArrangement* inputs, int32 numIns,
                                                        Steinberg::Vst::SpeakerArrangement* outputs, int32 numOuts)
{
    if (numIns != 1 || numOuts != 1) {
        return kResultFalse;
    }

    auto* bus = FCast<Steinberg::Vst::AudioBus>(audioInputs.at(0));
    if (!bus) {
        return kResultFalse;
    }

    if (Steinberg::Vst::SpeakerArr::getChannelCount(inputs[0]) == 2 &&
        Steinberg::Vst::SpeakerArr::getChannelCount(outputs[0]) == 2) {
        getAudioInput(0)->setArrangement(inputs[0]);
        getAudioInput(0)->setName(STR16("Stereo In"));
        getAudioOutput(0)->setArrangement(outputs[0]);
        getAudioOutput(0)->setName(STR16("Stereo Out"));
        return kResultTrue;
    }

    if (bus->getArrangement() != Steinberg::Vst::SpeakerArr::kStereo) {
        getAudioInput(0)->setArrangement(Steinberg::Vst::SpeakerArr::kStereo);
        getAudioInput(0)->setName(STR16("Stereo In"));
        getAudioOutput(0)->setArrangement(Steinberg::Vst::SpeakerArr::kStereo);
        getAudioOutput(0)->setName(STR16("Stereo Out"));
        return kResultFalse;
    }

    return kResultFalse;
}

tresult PLUGIN_API HDRProProcessor::setState (IBStream* state)
{
    if (!state) {
        kResultFalse;
    }

	IBStreamer streamer(state, kLittleEndian);

    int32 savedBypass = 0;
    if (!streamer.readInt32(savedBypass)) {
            return kResultFalse;
    }

    bBypass = savedBypass > 0;

    float gain;
    float hi_max;
    float lo_min;
    if (!streamer.readFloat(gain) ||
        !streamer.readFloat(hi_max) ||
        !streamer.readFloat(lo_min)) {
        return kResultFalse;
    }

    hdr32.gain = gain;
    hdr32.hi_max = hi_max;
    hdr32.lo_min = lo_min;
    hdr32.updateParams();
    hdr64.gain = gain;
    hdr64.hi_max = hi_max;
    hdr64.lo_min = lo_min;
    hdr64.updateParams();

	return kResultOk;
}

tresult PLUGIN_API HDRProProcessor::getState (IBStream* state)
{
	IBStreamer streamer(state, kLittleEndian);

    streamer.writeInt32(bBypass ? 1 : 0);
    streamer.writeFloat(hdr32.gain);
    streamer.writeFloat(hdr32.hi_max);
    streamer.writeFloat(hdr32.lo_min);

	return kResultOk;
}

}
