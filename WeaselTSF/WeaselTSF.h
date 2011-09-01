// The following ifdef block is the standard way of creating macros which make exporting 
// from a DLL simpler. All files within this DLL are compiled with the WEASELTSF_EXPORTS
// symbol defined on the command line. this symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see 
// WEASELTSF_API functions as being imported from a DLL, whereas this DLL sees symbols
// defined with this macro as being exported.
#ifdef WEASELTSF_EXPORTS
#define WEASELTSF_API __declspec(dllexport)
#else
#define WEASELTSF_API __declspec(dllimport)
#endif

// This class is exported from the WeaselTSF.dll
class WEASELTSF_API CWeaselTSF {
public:
	CWeaselTSF(void);
	// TODO: add your methods here.
};

extern WEASELTSF_API int nWeaselTSF;

WEASELTSF_API int fnWeaselTSF(void);
