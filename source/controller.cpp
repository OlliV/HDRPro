//------------------------------------------------------------------------
// Copyright(c) 2021-2022 Olli Vanhoja.
//------------------------------------------------------------------------

#include "pluginterfaces/base/ibstream.h"
#include "pluginterfaces/base/ustring.h"
#include "pluginterfaces/vst/ivstmidicontrollers.h"
#include "base/source/fstreamer.h"
#include "controller.h"
#include "cids.h"
#include "paramids.h"
#include "mdaParameter.h"
#include "vstgui/plugin-bindings/vst3editor.h"

using namespace Steinberg;
using namespace Steinberg::Vst;

namespace MyVst {

class GainParameter : public Parameter
{
public:
    GainParameter (const char *name, int32 flags, int32 id, float min, float max);

    void toString (ParamValue normValue, String128 string) const SMTG_OVERRIDE;
    bool fromString (const TChar* string, ParamValue& normValue) const SMTG_OVERRIDE;

private:
    float gain_min;
    float gain_max;
};

GainParameter::GainParameter (const char *name, int32 flags, int32 id, float min_db, float max_db)
{
    Steinberg::UString (info.title, USTRINGSIZE (info.title)).assign (USTRING (name));
    Steinberg::UString (info.units, USTRINGSIZE (info.units)).assign (USTRING ("dB"));

    gain_min = min_db;
    gain_max = max_db;

    info.flags = flags;
    info.id = id;
    info.stepCount = 0;
    info.defaultNormalizedValue = db2norm(0.0f, min_db, max_db);
    info.unitId = kRootUnitId;

    setNormalized(info.defaultNormalizedValue);
}

void GainParameter::toString (ParamValue normValue, String128 string) const
{
    char text[32];
    float db = norm2db(normValue, gain_min, gain_max);

    snprintf(text, sizeof(text), "%.2f", db);
    text[sizeof(text) - 1] = '\0';

    Steinberg::UString (string, 128).fromAscii(text);
}

bool GainParameter::fromString (const TChar* string, ParamValue& normValue) const
{
    String wrapper ((TChar*)string); // don't know buffer size here!
    double tmp = 0.0;

    if (wrapper.scanFloat(tmp)) {
        normValue = db2norm((float)tmp, gain_min, gain_max);

        return true;
    }
    return false;
}

tresult PLUGIN_API HDRProController::initialize (FUnknown* context)
{
	//---do not forget to call parent ------
	tresult result = EditControllerEx1::initialize(context);
	if (result != kResultOk) {
		return result;
	}

    //--- Create Units-------------
    UnitInfo unitInfo;
    Unit* unit;
    Parameter *param;

    // HDR
    unitInfo.id = HDRProHDRUnitId;
    unitInfo.parentUnitId = kRootUnitId; // attached to the root unit
    Steinberg::UString(unitInfo.name, USTRINGSIZE(unitInfo.name)).assign(USTRING("HDR"));
    unitInfo.programListId = kNoProgramListId;
    addUnit(new Unit(unitInfo));

    const int32 stepCountToggle = 1;
    ParamValue defaultVal = 0;
    int32 flags = ParameterInfo::kCanAutomate | ParameterInfo::kIsBypass;
    param = parameters.addParameter(STR16("Bypass"), nullptr, stepCountToggle, defaultVal, flags, kBypassId);
    param->setUnitID(HDRProRootUnitId);

    // Gain
    param = new GainParameter("Gain", ParameterInfo::kNoFlags, kHDRGainId, HDR_GAIN_MIN, HDR_GAIN_MAX);
    param->setNormalized(HDR_GAIN_DEFAULT_N);
    param->setPrecision(2);
    param->setUnitID(HDRProHDRUnitId);
    parameters.addParameter(param);

    // Hi_max
    param = new GainParameter("Hi max", ParameterInfo::kNoFlags, kHDRHiMaxId, HDR_24B_FS_MIN, HDR_24B_FS_MAX);
    param->setNormalized(HDR_HI_MAX_DEFAULT_N);
    param->setPrecision(2);
    param->setUnitID(HDRProHDRUnitId);
    parameters.addParameter(param);

    // Lo_min
    param = new GainParameter("Lo min", ParameterInfo::kNoFlags, kHDRLoMinId, HDR_24B_FS_MIN, HDR_24B_FS_MAX);
    param->setNormalized(HDR_LO_MIN_DEFAULT_N);
    param->setPrecision(2);
    param->setUnitID(HDRProHDRUnitId);
    parameters.addParameter(param);

    // Sel_lo
    parameters.addParameter(STR16("SelLo"), nullptr, 0, 0, ParameterInfo::kIsReadOnly, kHDRSelLoId);

	return result;
}

tresult PLUGIN_API HDRProController::terminate ()
{
	// Here the Plug-in will be de-instanciated, last possibility to remove some memory!

	//---do not forget to call parent ------
	return EditControllerEx1::terminate();
}

tresult PLUGIN_API HDRProController::setComponentState (IBStream* state)
{
    if (!state)
        return kResultFalse;

    IBStreamer streamer(state, kLittleEndian);

    int32 savedBypass;
    if (!streamer.readInt32(savedBypass)) {
        return kResultFalse;
    }

    setParamNormalized(kBypassId, savedBypass ? 1 : 0);

   float gain;
   if (!streamer.readFloat(gain)) {
       return kResultFalse;
   }

   setParamNormalized(kHDRGainId, gain);

	return kResultOk;
}

tresult PLUGIN_API HDRProController::setState (IBStream* state)
{
	// Here you get the state of the controller

	return kResultTrue;
}

tresult PLUGIN_API HDRProController::getState (IBStream* state)
{
	// Here you are asked to deliver the state of the controller (if needed)
	// Note: the real state of your plug-in is saved in the processor

	return kResultTrue;
}

IPlugView* PLUGIN_API HDRProController::createView (FIDString name)
{
	// Here the Host wants to open your editor (if you have one)
	if (FIDStringsEqual (name, Vst::ViewType::kEditor))
	{
		// create your editor here and return a IPlugView ptr of it
		auto* view = new VSTGUI::VST3Editor (this, "view", "editor.uidesc");
		return view;
	}
	return nullptr;
}

tresult PLUGIN_API HDRProController::setParamNormalized (Vst::ParamID tag, Vst::ParamValue value)
{
	// called by host to update your parameters
	tresult result = EditControllerEx1::setParamNormalized (tag, value);
	return result;
}

tresult PLUGIN_API HDRProController::getParamStringByValue (Vst::ParamID tag, Vst::ParamValue valueNormalized, Vst::String128 string)
{
	// called by host to get a string for given normalized value of a specific parameter
	// (without having to set the value!)
	return EditControllerEx1::getParamStringByValue (tag, valueNormalized, string);
}

tresult PLUGIN_API HDRProController::getParamValueByString (Vst::ParamID tag, Vst::TChar* string, Vst::ParamValue& valueNormalized)
{
	// called by host to get a normalized value from a string representation of a specific parameter
	// (without having to set the value!)
	return EditControllerEx1::getParamValueByString (tag, string, valueNormalized);
}

}
