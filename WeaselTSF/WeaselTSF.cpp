// WeaselTSF.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "WeaselTSF.h"


// This is an example of an exported variable
WEASELTSF_API int nWeaselTSF=0;

// This is an example of an exported function.
WEASELTSF_API int fnWeaselTSF(void)
{
	return 42;
}

// This is the constructor of a class that has been exported.
// see WeaselTSF.h for the class definition
CWeaselTSF::CWeaselTSF()
{
	return;
}
