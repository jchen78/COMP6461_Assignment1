// The following ifdef block is the standard way of creating macros which make exporting 
// from a DLL simpler. All files within this DLL are compiled with the PILOT_EXPORTS
// symbol defined on the command line. This symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see 
// PILOT_API functions as being imported from a DLL, whereas this DLL sees symbols
// defined with this macro as being exported.
#ifdef PILOT_EXPORTS
#define PILOT_API __declspec(dllexport)
#else
#define PILOT_API __declspec(dllimport)
#endif

// This class is exported from the Pilot.dll
class PILOT_API CPilot {
public:
	CPilot(void);
	// TODO: add your methods here.
};

extern PILOT_API int nPilot;

PILOT_API int fnPilot(void);
