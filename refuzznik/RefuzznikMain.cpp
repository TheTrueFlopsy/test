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

#include "wpstdinclude.h"

#include <new>
#include <stdexcept>
#include <windows.h>
#include "Refuzznik.hpp"

void *hInstance;
extern "C" BOOL WINAPI DllMain(HINSTANCE hInst, DWORD dwReason, LPVOID lpvReserved) {
	hInstance = hInst;
	return 1;
}

extern "C" AEffect *main_gcc(audioMasterCallback audioMaster) {
	// Check VST version.
	if (!audioMaster(NULL, audioMasterVersion, 0, 0, NULL, 0))
		return NULL;  // Old version.
	
	// Create the plug-in.
	Refuzznik *plug = NULL;
	
	try {
		plug = new Refuzznik(audioMaster);
	}
	catch (std::bad_alloc e) { // Plugin allocation failed.
		plug = NULL;
	}
	catch (std::runtime_error e) { // Plugin initialization failed.
		plug = NULL;
	}
	
	// Check whether the plug-in object was successfully created.
	if (plug == NULL)
		return NULL;
	else if (!plug->isOperational()) {
		delete plug;
		return NULL;
	}
	
	return plug->getAeffect();
}
