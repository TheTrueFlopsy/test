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

#ifndef REFUZZNIK_HPP
#define REFUZZNIK_HPP

#include "wpstdinclude.h"

#include <windows.h>
#include "audioeffectx.h"
#include "refuzznikparams.h"

#define RFZ_MAJOR 0
#define RFZ_MINOR 5
#define RFZ_UPDATE 3
#define RFZ_VENDOR_VERSION ((long) (RFZ_MAJOR*10000 + RFZ_MINOR*100 + RFZ_UPDATE))
#define RFZ_STRINGIFY(arg) #arg
#define RFZ_VERSION_STRING(ma, mi, u) RFZ_STRINGIFY(ma) "." RFZ_STRINGIFY(mi) "." RFZ_STRINGIFY(u)

class Refuzznik : public AudioEffectX {
public: // public typedefs
	typedef void (*funcfcp)(float, char *);
	
private: // private typedefs
	typedef void (Refuzznik::*methodf)(float);
	typedef void (Refuzznik::*method4fpi)(float *, float *, float *, float *, int);
	
	struct RefuzznikData {
		// ---<<< State fields (what is the case now) >>>---
		// Plugin configuration.
		bool operational, bypassedFlag;
		int input0Connected, input1Connected, output0Connected, output1Connected;
		
		// ---<<< Update fields (what the processing thread should change) >>>---
		bool reinitFlag, resetFlag, setProcessHandlersFlag;
		float newParamValues[kNumParams];
		
		void clearUpdateFields();
	};
	
private: // private static data members
	static const char *const paramNames[kNumParams];
	static const char *const paramLabels[kNumParams];
	
	static const float initParamValues[kNumParams];
	
	// Displayers and setters.
	static const funcfcp paramDisplayers[kNumParams];
	static const methodf paramSetters[kNumParams];
	
	// Processing handlers.
	static const method4fpi procHandlers[5], procRHandlers[5];
	
public: // public static methods
	// Displayers.
	static funcfcp getParamDisplayer(int index) {return paramDisplayers[index];}
	
	static void yesNoDisplayer(float value, char *text);
	static void pcntDisplayer(float value, char *text);
	static void dBDisplayer(float value, char *text);
	
	static void inGainDisplayer(float value, char *text);
	static void slewLimDisplayer(float value, char *text);
	static void freqMinDisplayer(float value, char *text);
	static void freqDifDisplayer(float value, char *text);
	static void outGainDisplayer(float value, char *text);
	
private: // private data members
	CRITICAL_SECTION myCriticalSection;
	
	// ---<<< Shared data                     >>>---
	// ---<<< ALL ACCESS MUST BE SYNCHRONIZED >>>---
	
	// NOTE: Mutable fields inherited from AudioEffectX also count as shared data.
	
	// Plugin info. (Not touched by the processing thread.)
	char programName[32];
	float paramValues[kNumParams];
	
	// Data accessed by both control and processing threads.
	RefuzznikData sharedData;
	
	// ---<<< Private data of processing object           >>>---
	// ---<<< ALL ACCESS MUST HAPPEN ON PROCESSING THREAD >>>---
	RefuzznikData processingData;
	
	// Setter and processing handler state.
	method4fpi procHandler, procRHandler;
	int in0Index, in1Index, out0Index, out1Index;
	
	// DSP variables.
	bool zeroX;
	float inGain, slewLim, funcMix, ampMin, freqMin, freqDif, outGain;
	float ampDif, periodsDifLog;
	
	float limitedIn0, limitedIn1;
	
public: // public methods
	// Constructor.
	Refuzznik(audioMasterCallback audioMaster);
	
	// Destructor.
	virtual ~Refuzznik();
	
	// Plug info.
	virtual bool getEffectName(char* name);
	virtual bool getVendorString(char* text);
	virtual bool getProductString(char* text);
	virtual long getVendorVersion();
	virtual VstPlugCategory getPlugCategory();
	virtual long canDo(char *text);
	
	// Programs.
	virtual void setProgram(long program);
	virtual void setProgramName(char *name);
	virtual void getProgramName(char *name);
	virtual bool getProgramNameIndexed(long category, long index, char *text);
	
	// Parameters.
	virtual void setParameter(long index, float value);
	virtual float getParameter(long index);
	virtual void getParameterLabel(long index, char *label);
	virtual void getParameterDisplay(long index, char *text);
	virtual void getParameterName(long index, char *text);
	
	// Processing.
	virtual bool setBypass(bool onOff);
	virtual void resume();
	virtual void process(float **inputs, float **outputs, long sampleFrames);
	virtual void processReplacing(float **inputs, float **outputs, long sampleFrames);
	
	// Custom.
	bool isOperational();
	
private: // private methods
	void setOperational(bool flag);
	
	void doThreadSynchronizedDataExchange();
	
	bool reinitialize();
	
	// Setters.
	int doParameterUpdates();
	
	void zeroXSetter(float value);
	void inGainSetter(float value);
	void slewLimSetter(float value);
	void funcMixSetter(float value);
	void ampMinSetter(float value);
	void freqMinSetter(float value);
	void freqDifSetter(float value);
	void outGainSetter(float value);
	
	// Processing handlers.
	void setProcHandlers();
	
	void procDoNothing(float *in0, float *in1, float *out0, float *out1, int sampleFrames) {}
	
	void proc1In1Out(float *in0, float *in1, float *out0, float *out1, int sampleFrames);
	void proc1In1OutB(float *in0, float *in1, float *out0, float *out1, int sampleFrames);
	void proc2In2Out(float *in0, float *in1, float *out0, float *out1, int sampleFrames);
	void proc2In2OutB(float *in0, float *in1, float *out0, float *out1, int sampleFrames);
	
	void procR1In1Out(float *in0, float *in1, float *out0, float *out1, int sampleFrames);
	void procR1In1OutB(float *in0, float *in1, float *out0, float *out1, int sampleFrames);
	void procR2In2Out(float *in0, float *in1, float *out0, float *out1, int sampleFrames);
	void procR2In2OutB(float *in0, float *in1, float *out0, float *out1, int sampleFrames);
	
	// DSP routines
	float refuzzZeroX(float in, float &limitedIn);
	float refuzzNoZeroX(float in, float &limitedIn);
	
	void updateSlewLimiter(float in, float &limitedIn);
	
	inline float aMap(float ain);
	inline float fMap(float ain);
};

#endif
