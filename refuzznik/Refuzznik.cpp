/*
Copyright (c) 2007 Johan Sarge

Permission is hereby granted, free of charge, to any person obtaining a copy of this
software and associated documentation files (the "Software"), to deal in the Software
without restriction, including without limitation the rights to use, copy, modify, merge,
publish, distribute, sublicense, and/or sell copies of the Software, and to permit
persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or
substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE
FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
DEALINGS IN THE SOFTWARE.
*/

#include "Refuzznik.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <new>
#include <stdexcept>
#ifndef RFZ_NO_GUI
#include "RefuzznikEditor.hpp"
#endif

// Parameter transform macros.
#define BOOL_T(v) ((v) > 0.5f)
#define IN_GAIN_T(v) (std::pow(2.0f, 8.0f*(v)))
#define SLEW_LIM_T(v) (std::pow(2.0f, 9.0f*((v) - 1.0f)))
#define F_MIN_T(v) (std::pow(2.0f, 9.0f*(v) - 1.0f))
#define F_DIFF_T(v) (copysignf(std::pow(2.0f, 14.0f*std::abs((v) - 0.5f)) - 1.0f, (v) - 0.5f))
#define OUT_GAIN_T(v) (std::pow(2.0f, 8.0f*((v) - 1.0f)))

// Private static data.
const char *const Refuzznik::paramNames[kNumParams] = {
	"ZeroX", "InGain", "SlewLim", "FuncMix", "AmpMin", "FreqMin", "FreqDif", "OutGain"
};

const char *const Refuzznik::paramLabels[kNumParams] = {
	"", "dB", "%", "%", "%", "cycles", "cycles", "dB"
};

const float Refuzznik::initParamValues[kNumParams] = {
	1.0f, 0.0f, 1.0f, 0.5f, 0.0f, 2.0f / 9.0f, 0.5f, 1.0f
};

const Refuzznik::funcfcp Refuzznik::paramDisplayers[kNumParams] = {
	&Refuzznik::yesNoDisplayer,
	&Refuzznik::inGainDisplayer,
	&Refuzznik::slewLimDisplayer,
	&Refuzznik::pcntDisplayer,
	&Refuzznik::pcntDisplayer,
	&Refuzznik::freqMinDisplayer,
	&Refuzznik::freqDifDisplayer,
	&Refuzznik::outGainDisplayer
};

const Refuzznik::methodf Refuzznik::paramSetters[kNumParams] = {
	&Refuzznik::zeroXSetter,
	&Refuzznik::inGainSetter,
	&Refuzznik::slewLimSetter,
	&Refuzznik::funcMixSetter,
	&Refuzznik::ampMinSetter,
	&Refuzznik::freqMinSetter,
	&Refuzznik::freqDifSetter,
	&Refuzznik::outGainSetter
};

const Refuzznik::method4fpi Refuzznik::procHandlers[5] = {
	&Refuzznik::proc1In1Out,
	&Refuzznik::proc1In1OutB,
	&Refuzznik::proc2In2Out,
	&Refuzznik::proc2In2OutB,
	
	&Refuzznik::procDoNothing // Fallback handler for error states.
};

const Refuzznik::method4fpi Refuzznik::procRHandlers[5] = {
	&Refuzznik::procR1In1Out,
	&Refuzznik::procR1In1OutB,
	&Refuzznik::procR2In2Out,
	&Refuzznik::procR2In2OutB,
	
	&Refuzznik::procDoNothing // Fallback handler for error states.
};


// Displayers.
void Refuzznik::yesNoDisplayer(float value, char *text) {
	std::strcpy(text, BOOL_T(value) ? "Yes" : "No");
}

void Refuzznik::pcntDisplayer(float value, char *text) {
	std::sprintf(text, "%1.3f", 100.0f*value);
}

void Refuzznik::dBDisplayer(float value, char *text) {
	std::sprintf(text, "%1.2f", 20.0f*std::log10(value));
}

void Refuzznik::inGainDisplayer(float value, char *text) {
	dBDisplayer(IN_GAIN_T(value), text);
}

void Refuzznik::slewLimDisplayer(float value, char *text) {
	float value2 = SLEW_LIM_T(value);
	if (value2 < 0.999f)
		std::sprintf(text, "%1.2f", 100.0f*value2);
	else
		std::strcpy(text, "Off");
}

void Refuzznik::freqMinDisplayer(float value, char *text) {
	std::sprintf(text, "%1.2f", F_MIN_T(value));
}

void Refuzznik::freqDifDisplayer(float value, char *text) {
	std::sprintf(text, "%1.2f", F_DIFF_T(value));
}

void Refuzznik::outGainDisplayer(float value, char *text) {
	dBDisplayer(OUT_GAIN_T(value), text);
}


// Constructor.
Refuzznik::Refuzznik(audioMasterCallback audioMaster) :
AudioEffectX(audioMaster, 1, kNumParams) {
	
	// Initialize critical section to synch control threads and process thread.
#ifdef RFZ_OLD_WINDOWS
	InitializeCriticalSection(&myCriticalSection);
#else
	if (!InitializeCriticalSectionAndSpinCount(&myCriticalSection, 0x80000400)) {
		sharedData.operational = false;
		throw std::runtime_error("Refuzznik::Refuzznik - Critical section initialization failed.");
	}
#endif
	
	EnterCriticalSection(&myCriticalSection);
	
	setNumInputs(2); // Stereo in.
	setNumOutputs(2); // Stereo out.
	setUniqueID(CCONST('R','f','z','n')); // Identify.
	canProcessReplacing(); // Supports both accumulating and replacing output.
	std::strcpy(programName, "Default");	// Default program name.
	
	// Set initial plugin configuration.
	sharedData.operational = true;
	sharedData.bypassedFlag = false;
	sharedData.input0Connected = sharedData.output0Connected = 1;
	sharedData.input1Connected = sharedData.output1Connected = 0;
	
	// Set initial parameter values.
	std::memcpy(paramValues, initParamValues, kNumParams * sizeof (float));
	
	// Tell processing thread to initialize its private data.
	sharedData.reinitFlag = true; // This is SUPER IMPORTANT!
	sharedData.resetFlag = false;
	sharedData.setProcessHandlersFlag = false;
	std::memcpy(
		sharedData.newParamValues, paramValues, kNumParams * sizeof (float));
	
	// Create GUI editor (if GUI build).
	editor = NULL;
	
#ifndef RFZ_NO_GUI
	try {
		editor = new RefuzznikEditor(this);
	}
	catch (std::bad_alloc e) { // Editor allocation failed.
		sharedData.operational = false;
	}
#endif
	
	// Done.
	LeaveCriticalSection(&myCriticalSection);
}

Refuzznik::~Refuzznik() {
#ifndef RFZ_NO_GUI
	EnterCriticalSection(&myCriticalSection);
	
	delete editor;
	editor = NULL;
	
	LeaveCriticalSection(&myCriticalSection);
#endif
	
	DeleteCriticalSection(&myCriticalSection);
}


// Inherited virtual public methods.
bool Refuzznik::getEffectName(char *text) {
	std::strcpy(text, "Refuzznik");
	return true;
}

bool Refuzznik::getProductString(char *text) {
	std::strcpy(text, "Refuzznik - Sinusoidal distortion unit");
	return true;
}

bool Refuzznik::getVendorString(char *text) {
	std::strcpy(text, "Transvaal Audio");
	return true;
}

long Refuzznik::getVendorVersion() {return RFZ_VENDOR_VERSION;}

VstPlugCategory Refuzznik::getPlugCategory() {return kPlugCategEffect;}

long Refuzznik::canDo(char *text) {
	if (!std::strcmp(text, "sendVstEvents") ||
	    !std::strcmp(text, "sendVstMidiEvent") ||
	    !std::strcmp(text, "sendVstTimeInfo"))
		return -1l; // No event send.
	if (!std::strcmp(text, "receiveVstEvents") ||
	    !std::strcmp(text, "receiveVstMidiEvent") ||
	    !std::strcmp(text, "receiveVstTimeInfo"))
		return -1l; // No event receive.
	else if (!std::strcmp(text, "offline") || !std::strcmp(text, "noRealTime"))
		return -1l; // Realtime interface only.
	else if (!std::strcmp(text, "1in1out") || !std::strcmp(text, "2in2out"))
		return 1l; // Supported IO configurations.
	else if (!std::strcmp(text, "1in2out") || !std::strcmp(text, "2in1out") ||
	         !std::strcmp(text, "2in4out") || !std::strcmp(text, "4in2out") ||
		       !std::strcmp(text, "4in4out") || !std::strcmp(text, "4in8out") ||
	         !std::strcmp(text, "8in4out") || !std::strcmp(text, "8in8out"))
		return -1l; // Unsupported IO configurations.
	else if (!std::strcmp(text, "bypass"))
		return 1l; // Soft/listening bypass supported.
	return 0l; // Dunno.
}

void Refuzznik::setProgram(long program) {} // No program changes allowed.

void Refuzznik::setProgramName(char *name) { // SYNCHRONIZED
	EnterCriticalSection(&myCriticalSection);
	
	std::strcpy(programName, name);
	
	LeaveCriticalSection(&myCriticalSection);
}

void Refuzznik::getProgramName(char *name) { // SYNCHRONIZED
	EnterCriticalSection(&myCriticalSection);
	
	std::strcpy(name, programName);
	
	LeaveCriticalSection(&myCriticalSection);
}

bool Refuzznik::getProgramNameIndexed(long category, long index, char *text) { // SYNCHRONIZED
	bool success = false;
	
	EnterCriticalSection(&myCriticalSection);
	
	if ((category == 0 || category == -1) && index == 0) {
		std::strcpy(text, programName);
		success = true;
	}
	
	LeaveCriticalSection(&myCriticalSection);
	
	return success;
}

void Refuzznik::setParameter(long index, float value) { // SYNCHRONIZED
	EnterCriticalSection(&myCriticalSection);
	
	paramValues[index] = value;
	sharedData.newParamValues[index] = value;
	
#ifndef RFZ_NO_GUI
	if (editor != NULL)
		((AEffGUIEditor *) editor)->setParameter(index, value);
#endif
	
	LeaveCriticalSection(&myCriticalSection);
}

float Refuzznik::getParameter(long index) { // SYNCHRONIZED
	float value = NAN;
	
	EnterCriticalSection(&myCriticalSection);
	
	value = paramValues[index];
	
	LeaveCriticalSection(&myCriticalSection);
	
	return value;
}

void Refuzznik::getParameterName(long index, char *label) {
	std::strcpy(label, paramNames[index]);
}

void Refuzznik::getParameterDisplay(long index, char *text) {
	float value = getParameter(index);
	
	paramDisplayers[index](value, text);
}

void Refuzznik::getParameterLabel(long index, char *label) {
	std::strcpy(label, paramLabels[index]);
}

bool Refuzznik::setBypass(bool onOff) { // SYNCHRONIZED
	EnterCriticalSection(&myCriticalSection);
	
	sharedData.bypassedFlag = onOff;
	sharedData.setProcessHandlersFlag = true;
	
	LeaveCriticalSection(&myCriticalSection);
	
	return true;
}

void Refuzznik::resume() { // SYNCHRONIZED
	int nInputs = 0, nOutputs = 0;
	
	EnterCriticalSection(&myCriticalSection);
	
	sharedData.resetFlag = true;
	sharedData.setProcessHandlersFlag = true;
	
	nInputs += (sharedData.input0Connected = isInputConnected(0l));
	nInputs += (sharedData.input1Connected = isInputConnected(1l));
	nOutputs += (sharedData.output0Connected = isOutputConnected(0l));
	nOutputs += (sharedData.output1Connected = isOutputConnected(1l));
	
	if (nInputs == 0 | nOutputs == 0 | nInputs != nOutputs)
		sharedData.operational = false;
	
	LeaveCriticalSection(&myCriticalSection);
	
	if (nInputs != nOutputs)
		throw std::runtime_error("Refuzznik::resume - Input and output channel counts inequal.");
	else if (nInputs == 0)
		throw std::runtime_error("Refuzznik::resume - No inputs connected.");
	else if (nOutputs == 0)
		throw std::runtime_error("Refuzznik::resume - No outputs connected.");
}

void Refuzznik::process(float **inputs, float **outputs, long sampleFrames) {
	doThreadSynchronizedDataExchange();
	
	(this->*procHandler)(
		inputs[in0Index], inputs[in1Index], outputs[out0Index], outputs[out1Index], sampleFrames);
}

void Refuzznik::processReplacing(float **inputs, float **outputs, long sampleFrames) {
	doThreadSynchronizedDataExchange();
	
	(this->*procRHandler)(
		inputs[in0Index], inputs[in1Index], outputs[out0Index], outputs[out1Index], sampleFrames);
}


// Non-inherited non-virtual public methods.
bool Refuzznik::isOperational() { // SYNCHRONIZED
	bool flag = false;
	
	EnterCriticalSection(&myCriticalSection);
	
	flag = sharedData.operational;
	
	LeaveCriticalSection(&myCriticalSection);
	
	return flag;
}


// ---<<< PRIVATE METHODS BEGIN HERE >>>---
void Refuzznik::setOperational(bool flag) { // SYNCHRONIZED
	EnterCriticalSection(&myCriticalSection);
	
	sharedData.operational = flag;
	
	LeaveCriticalSection(&myCriticalSection);
}

void Refuzznik::RefuzznikData::clearUpdateFields() {
	reinitFlag = false;
	resetFlag = false;
	setProcessHandlersFlag = false;
	
	std::fill(newParamValues, newParamValues + kNumParams, NAN);
}

void Refuzznik::doThreadSynchronizedDataExchange() {
	
	EnterCriticalSection(&myCriticalSection);
	
	// Copy shared structure to thread-private structure.
	processingData = sharedData;
	sharedData.clearUpdateFields();
	
	LeaveCriticalSection(&myCriticalSection);
	
	// Update plugin configuration.
	if (processingData.reinitFlag) {
		if (!reinitialize())
			setOperational(false);
	}
	
	if (processingData.setProcessHandlersFlag)
		setProcHandlers();
	
	if (!processingData.operational) // SYSTEM ATE SHIT.
		return;
	
	if (processingData.resetFlag)
		limitedIn0 = limitedIn1 = 0.0f;
	
	// Perform parameter updates.
	doParameterUpdates();
}

bool Refuzznik::reinitialize() {
	// Initialize DSP variables.
	zeroX = true;
	inGain = 1.0f;
	slewLim = 1000000.0f;
	funcMix = 0.5f;
	ampMin = 0.0f;
	freqMin = (float) M_PI;
	freqDif = 0.0f;
	outGain = 1.0f;
	
	ampDif = 1.0f;
	periodsDifLog = 1.0f;
	
	limitedIn0 = limitedIn1 = 0.0f;
	
	// Set initial processing handlers.
	setProcHandlers();
	
	return processingData.operational;
}


// Setters.
int Refuzznik::doParameterUpdates() {
	int nUpdated = 0;
	
	for (int index = 0; index < kNumParams; index++) {
		if (!std::isnan(processingData.newParamValues[index])) { // Parameter updated.
			(this->*paramSetters[index])(processingData.newParamValues[index]);
			
			nUpdated++;
		}
	}
	
	return nUpdated;
}

void Refuzznik::zeroXSetter(float value) {zeroX = BOOL_T(value);}

void Refuzznik::funcMixSetter(float value) {funcMix = value;}

void Refuzznik::inGainSetter(float value) {inGain = IN_GAIN_T(value);}

void Refuzznik::slewLimSetter(float value) {
	slewLim = SLEW_LIM_T(value);
	if (slewLim >= 0.999f)
		slewLim = 1000000.0f;
}

void Refuzznik::ampMinSetter(float value) {
	ampMin = value;
	ampDif = 1.0f - ampMin;
}

void Refuzznik::freqMinSetter(float value) {
	freqMin = F_MIN_T(value) * ((float) M_PI);
}

void Refuzznik::freqDifSetter(float value) {
	float periods = F_DIFF_T(value);
	
	periodsDifLog = std::log(std::abs(periods) + 2.0f)/((float) M_LN2);
	freqDif = periods * ((float) M_PI);
}

void Refuzznik::outGainSetter(float value) {outGain = OUT_GAIN_T(value);}


// Processing handlers.
void Refuzznik::setProcHandlers() {
	if (!processingData.operational) { // Unrecoverable error.
		procHandler = procHandlers[5]; // Use do-nothing handlers.
		procRHandler = procRHandlers[5];
		return;
	}
	
	int handlerIndex = 0;
	
	switch (processingData.input1Connected << 3 | processingData.input0Connected << 2 |
	        processingData.output1Connected << 1 | processingData.output0Connected) {
		case 5: // !in1 in0 !out1 out0
		in1Index = 0; in0Index = 0; out1Index = 0; out0Index = 0;
		break;
		
		case 6: // !in1 in0 out1 !out0
		in1Index = 0; in0Index = 0; out1Index = 1; out0Index = 1;
		break;
		
		case 9: // in1 !in0 !out1 out0
		in1Index = 1; in0Index = 1; out1Index = 0; out0Index = 0;
		break;
		
		case 10: // in1 !in0 out1 !out0
		in1Index = 1; in0Index = 1; out1Index = 1; out0Index = 1;
		break;
		
		case 15: // in1 in0 out1 out0
		in1Index = 1; in0Index = 0; out1Index = 1; out0Index = 0;
		handlerIndex += 2; // Stereo mode.
		break;
	}
	
	if (processingData.bypassedFlag)
		handlerIndex += 1; // Bypass mode.
	
	procHandler = procHandlers[handlerIndex];
	procRHandler = procRHandlers[handlerIndex];
}

void Refuzznik::proc1In1Out(float *in0, float *in1, float *out0, float *out1, int sampleFrames) {
	if (zeroX)
		while (--sampleFrames >= 0)
			*out0++ += refuzzZeroX(*in0++, limitedIn0);
	else
		while (--sampleFrames >= 0)
			*out0++ += refuzzNoZeroX(*in0++, limitedIn0);
}

void Refuzznik::proc1In1OutB(float *in0, float *in1, float *out0, float *out1, int sampleFrames) {
	while (--sampleFrames >= 0) {
		updateSlewLimiter(*in0, limitedIn0);
		*out0++ += *in0++;
	}
}

void Refuzznik::proc2In2Out(float *in0, float *in1, float *out0, float *out1, int sampleFrames) {
	if (zeroX)
		while (--sampleFrames >= 0) {
			*out0++ += refuzzZeroX(*in0++, limitedIn0);
			*out1++ += refuzzZeroX(*in1++, limitedIn1);
		}
	else
		while (--sampleFrames >= 0) {
			*out0++ += refuzzNoZeroX(*in0++, limitedIn0);
			*out1++ += refuzzNoZeroX(*in1++, limitedIn1);
		}
}

void Refuzznik::proc2In2OutB(float *in0, float *in1, float *out0, float *out1, int sampleFrames) {
	while (--sampleFrames >= 0) {
		updateSlewLimiter(*in0, limitedIn0);
		updateSlewLimiter(*in1, limitedIn1);
		*out0++ += *in0++;
		*out1++ += *in1++;
	}
}

void Refuzznik::procR1In1Out(float *in0, float *in1, float *out0, float *out1, int sampleFrames) {
	if (zeroX)
		while (--sampleFrames >= 0)
			*out0++ = refuzzZeroX(*in0++, limitedIn0);
	else
		while (--sampleFrames >= 0)
			*out0++ = refuzzNoZeroX(*in0++, limitedIn0);
}

void Refuzznik::procR1In1OutB(float *in0, float *in1, float *out0, float *out1, int sampleFrames) {
	while (--sampleFrames >= 0) {
		updateSlewLimiter(*in0, limitedIn0);
		*out0++ = *in0++;
	}
}

void Refuzznik::procR2In2Out(float *in0, float *in1, float *out0, float *out1, int sampleFrames) {
	if (zeroX)
		while (--sampleFrames >= 0) {
			*out0++ = refuzzZeroX(*in0++, limitedIn0);
			*out1++ = refuzzZeroX(*in1++, limitedIn1);
		}
	else
		while (--sampleFrames >= 0) {
			*out0++ = refuzzNoZeroX(*in0++, limitedIn0);
			*out1++ = refuzzNoZeroX(*in1++, limitedIn1);
		}
}

void Refuzznik::procR2In2OutB(float *in0, float *in1, float *out0, float *out1, int sampleFrames) {
	while (--sampleFrames >= 0) {
		updateSlewLimiter(*in0, limitedIn0);
		updateSlewLimiter(*in1, limitedIn1);
		*out0++ = *in0++;
		*out1++ = *in1++;
	}
}


// DSP routines.
float Refuzznik::refuzzZeroX(float in, float &limitedIn) {
	in *= inGain;
	
	float diff = in - limitedIn;
	limitedIn += copysignf(std::abs(diff) <? slewLim, diff);
	
	float a = aMap(in);
	
	float
		shelfTerm = copysignf(a, in),
		sinTerm = (ampMin + ampDif * a) * std::sin(fMap(limitedIn) * limitedIn);
	
	return outGain * (shelfTerm + funcMix * (sinTerm - shelfTerm));
}

float Refuzznik::refuzzNoZeroX(float in, float &limitedIn) {
	in *= inGain;
	
	float diff = in - limitedIn;
	limitedIn += copysignf(std::abs(diff) <? slewLim, diff);
	
	float a = aMap(in);
	
	float
		sinTerm = (ampMin + ampDif * a) * 0.5f * (1.0f - std::cos(fMap(limitedIn) * limitedIn));
	
	return outGain * copysignf(a + funcMix * (sinTerm - a), in);
}

void Refuzznik::updateSlewLimiter(float in, float &limitedIn) {
	in *= inGain;
	
	float diff = in - limitedIn;
	limitedIn += copysignf(std::abs(diff) <? slewLim, diff);
}

inline float Refuzznik::aMap(float in) {
	float ain = std::abs(in);
	float f1 = 4.0f * ain * ain;
	return f1 / (f1 + 1.0f);
}

inline float Refuzznik::fMap(float in) {
	return freqMin + freqDif * std::pow(std::abs(in), periodsDifLog);
}
