// Pilot.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "Pilot.h"


// This is an example of an exported variable
PILOT_API int nPilot=0;

// This is an example of an exported function.
PILOT_API int fnPilot(void)
{
	return 42;
}

// This is the constructor of a class that has been exported.
// see Pilot.h for the class definition
CPilot::CPilot()
{
	return;
}
