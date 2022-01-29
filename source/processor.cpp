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

    demon32[0].gain = DEMON0_GAIN_DEFAULT_N;
    demon32[0].pitch = DEMON0_PITCH_DEFAULT_N;
    demon32[0].boostGain = DEMON0_BOOST_GAIN_DEFAULT_N;
    demon32[0].boostFc = DEMON0_BOOST_FC_DEFAULT_N;
    demon32[0].blend = DEMON0_BLEND_DEFAULT_N;
    demon32[0].updateParams();
    demon32[1].gain = DEMON1_GAIN_DEFAULT_N;
    demon32[1].pitch = DEMON1_PITCH_DEFAULT_N;
    demon32[1].boostGain = DEMON1_BOOST_GAIN_DEFAULT_N;
    demon32[1].boostFc = DEMON1_BOOST_FC_DEFAULT_N;
    demon32[1].blend = DEMON1_BLEND_DEFAULT_N;
    demon32[1].updateParams();
    demon32[2].gain = DEMON2_GAIN_DEFAULT_N;
    demon32[2].pitch = DEMON2_PITCH_DEFAULT_N;
    demon32[2].boostGain = DEMON2_BOOST_GAIN_DEFAULT_N;
    demon32[2].boostFc = DEMON2_BOOST_FC_DEFAULT_N;
    demon32[2].blend = DEMON2_BLEND_DEFAULT_N;
    demon32[2].updateParams();
    demon64[0].gain = DEMON0_GAIN_DEFAULT_N;
    demon64[0].pitch = DEMON0_PITCH_DEFAULT_N;
    demon64[0].boostGain = DEMON0_BOOST_GAIN_DEFAULT_N;
    demon64[0].boostFc = DEMON0_BOOST_FC_DEFAULT_N;
    demon64[0].blend = DEMON0_BLEND_DEFAULT_N;
    demon64[0].updateParams();
    demon64[1].gain = DEMON1_GAIN_DEFAULT_N;
    demon64[1].pitch = DEMON1_PITCH_DEFAULT_N;
    demon64[1].boostGain = DEMON1_BOOST_GAIN_DEFAULT_N;
    demon64[1].boostFc = DEMON1_BOOST_FC_DEFAULT_N;
    demon64[1].blend = DEMON1_BLEND_DEFAULT_N;
    demon64[1].updateParams();
    demon64[2].gain = DEMON2_GAIN_DEFAULT_N;
    demon64[2].pitch = DEMON2_PITCH_DEFAULT_N;
    demon64[2].boostGain = DEMON2_BOOST_GAIN_DEFAULT_N;
    demon64[2].boostFc = DEMON2_BOOST_FC_DEFAULT_N;
    demon64[2].blend = DEMON2_BLEND_DEFAULT_N;
    demon64[2].updateParams();

    addAudioInput(STR16("Mono In"), Steinberg::Vst::SpeakerArr::kMono);
    addAudioOutput(STR16("Mono Out"), Steinberg::Vst::SpeakerArr::kMono);
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

        for (int i = 3; i < 3; i++) {
            demon32[i].reset(sampleRate);
            demon32[i].updateParams();
            demon64[i].reset(sampleRate);
            demon64[i].updateParams();
        }
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

                case kDemon0GainId:
                    if (paramQueue->getPoint(numPoints - 1, sampleOffset, value) == kResultTrue) {
                        demon32[0].gain = value;
                        demon64[0].gain = value;
                    }
                    break;
                case kDemon1GainId:
                    if (paramQueue->getPoint(numPoints - 1, sampleOffset, value) == kResultTrue) {
                        demon32[1].gain = value;
                        demon64[1].gain = value;
                    }
                    break;
                case kDemon2GainId:
                    if (paramQueue->getPoint(numPoints - 1, sampleOffset, value) == kResultTrue) {
                        demon32[2].gain = value;
                        demon64[2].gain = value;
                    }
                    break;

                case kDemon0PitchId:
                    if (paramQueue->getPoint(numPoints - 1, sampleOffset, value) == kResultTrue) {
                        demon32[0].pitch = value;
                        demon64[0].pitch = value;
                    }
                    break;
                case kDemon1PitchId:
                    if (paramQueue->getPoint(numPoints - 1, sampleOffset, value) == kResultTrue) {
                        demon32[1].pitch = value;
                        demon64[1].pitch = value;
                    }
                    break;
                case kDemon2PitchId:
                    if (paramQueue->getPoint(numPoints - 1, sampleOffset, value) == kResultTrue) {
                        demon32[2].pitch = value;
                        demon64[2].pitch = value;
                    }
                    break;

                case kDemon0BoostGainId:
                    if (paramQueue->getPoint(numPoints - 1, sampleOffset, value) == kResultTrue) {
                        demon32[0].boostGain = value;
                        demon64[0].boostGain = value;
                    }
                    break;
                case kDemon1BoostGainId:
                    if (paramQueue->getPoint(numPoints - 1, sampleOffset, value) == kResultTrue) {
                        demon32[1].boostGain = value;
                        demon64[1].boostGain = value;
                    }
                    break;
                case kDemon2BoostGainId:
                    if (paramQueue->getPoint(numPoints - 1, sampleOffset, value) == kResultTrue) {
                        demon32[2].boostGain = value;
                        demon64[2].boostGain = value;
                    }
                    break;

                case kDemon0BoostFcId:
                    if (paramQueue->getPoint(numPoints - 1, sampleOffset, value) == kResultTrue) {
                        demon32[0].boostFc = value;
                        demon64[0].boostFc = value;
                    }
                    break;
                case kDemon1BoostFcId:
                    if (paramQueue->getPoint(numPoints - 1, sampleOffset, value) == kResultTrue) {
                        demon32[1].boostFc = value;
                        demon64[1].boostFc = value;
                    }
                    break;
                case kDemon2BoostFcId:
                    if (paramQueue->getPoint(numPoints - 1, sampleOffset, value) == kResultTrue) {
                        demon32[2].boostFc = value;
                        demon64[2].boostFc = value;
                    }
                    break;

                case kDemon0BlendId:
                    if (paramQueue->getPoint(numPoints - 1, sampleOffset, value) == kResultTrue) {
                        demon32[0].blend = value;
                        demon64[0].blend = value;
                    }
                    break;
                case kDemon1BlendId:
                    if (paramQueue->getPoint(numPoints - 1, sampleOffset, value) == kResultTrue) {
                        demon32[1].blend = value;
                        demon64[1].blend = value;
                    }
                    break;
                case kDemon2BlendId:
                    if (paramQueue->getPoint(numPoints - 1, sampleOffset, value) == kResultTrue) {
                        demon32[2].blend = value;
                        demon64[2].blend = value;
                    }
                    break;
            }
        }

        if (numParamsChanged) {
            for (int i = 0; i < 3; i++) {
                demon32[i].updateParams();
                demon64[i].updateParams();
            }
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

    if (data.numInputs != 2 || data.numOutputs != 1) {
        return kResultOk;
    }

    void* in = getChannelBuffersPointer(processSetup, data.inputs[0]);
    void* out = getChannelBuffersPointer(processSetup, data.outputs[0])[0];
    const int nrSamples = data.numSamples;
    const size_t sampleFramesSize = getSampleFramesSizeInBytes(processSetup, nrSamples);

    if (data.inputs[0].silenceFlags) {
        data.outputs[0].silenceFlags = data.inputs[0].silenceFlags;

        if (in != out) {
            memset(out, 0, sampleFramesSize);
        }
    } else {
        // Normally the output is not silenced
        data.outputs[0].silenceFlags = 0;

        if (!bBypass) {
            memset(out, 0, sampleFramesSize);
        } else if (in != out) {
            memmove(out, in[0], sampleFramesSize);
        }

        if (data.symbolicSampleSize == kSample32) {
            Sample32 *hi = ((Sample32**)in)[0];
            Sample32 *lo = ((Sample32**)in)[1];

            for (size_t i = 0; i < nrSamples; i++) {
                ((Sample32*)out)[i] = hdr32.process(hi[i], lo[i]);
            }
        } else {
            Sample64 *hi = ((Sample64**)in)[0];
            Sample64 *lo = ((Sample64**)in)[1];

            for (size_t i = 0; i < nrSamples; i++) {
                ((Sample64*)out)[i] = hdr64.process(hi[i], lo[i]);
            }
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
    if (numIns != 1 || numOuts != 1 ||
        Steinberg::Vst::SpeakerArr::getChannelCount(inputs[0]) != 1 ||
        Steinberg::Vst::SpeakerArr::getChannelCount(outputs[0]) != 1) {
        return kResultFalse;
    }

    auto* bus = FCast<Steinberg::Vst::AudioBus>(audioInputs.at(0));
    if (!bus) {
        return kResultFalse;
    }

    if (bus->getArrangement() != Steinberg::Vst::SpeakerArr::kMono) {
        getAudioInput(0)->setArrangement(inputs[0]);
        getAudioInput(0)->setName(STR16("Mono In"));
        getAudioOutput(0)->setArrangement(outputs[0]);
        getAudioOutput(0)->setName(STR16("Mono Out"));
    }

    return kResultOk;
}

tresult PLUGIN_API HDRProProcessor::setState (IBStream* state)
{
    // called when we load a preset or project, the model has to be reloaded
    if (!state) {
        kResultFalse;
    }

	IBStreamer streamer(state, kLittleEndian);

    int32 savedBypass = 0;

    if (!streamer.readInt32(savedBypass)) {
            return kResultFalse;
    }

    bBypass = savedBypass > 0;

    for (int i = 0; i < 3; i++) {
        float gain;
        float pitch;
        float boostGain;
        float boostFc;
        float blend;

        if (!streamer.readFloat(gain) ||
            !streamer.readFloat(pitch) ||
            !streamer.readFloat(boostGain) ||
            !streamer.readFloat(boostFc) ||
            !streamer.readFloat(blend)) {
            return kResultFalse;
        }

        demon32[i].gain = gain;
        demon32[i].pitch = pitch;
        demon32[i].boostGain = boostGain;
        demon32[i].boostFc = boostFc;
        demon32[i].blend = blend;
        demon32[i].updateParams();

        demon64[i].gain = gain;
        demon64[i].pitch = pitch;
        demon64[i].boostGain = boostGain;
        demon64[i].boostFc = boostFc;
        demon64[i].blend = blend;
        demon64[i].updateParams();
    }

	return kResultOk;
}

tresult PLUGIN_API HDRProProcessor::getState (IBStream* state)
{
	// here we need to save the model
	IBStreamer streamer (state, kLittleEndian);

    /*
     * YOLO, we just write the state in the right order here
     * and hope it goes well.
     */
    streamer.writeInt32(bBypass ? 1 : 0);

    for (int i = 0; i < 3; i++) {
        streamer.writeFloat(demon32[i].gain);
        streamer.writeFloat(demon32[i].pitch);
        streamer.writeFloat(demon32[i].boostGain);
        streamer.writeFloat(demon32[i].boostFc);
        streamer.writeFloat(demon32[i].blend);
    }

	return kResultOk;
}

}
