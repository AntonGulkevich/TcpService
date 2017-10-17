#include "TestMediator.h"
#include <iostream>


#if __has_include(<SccDeviceLib>)
#  include "SccDeviceLib.h"
#  define SccInSolution 1
#elif __has_include(<SccDeviceLib/ErrorCodes>)
#  include "ErrorCodes"
#  define SccInSolution 1
#  define SccInSolution_ErrorCodes 1
#else
#  define SccInSolution 0
#  include "../../../SccLib/SccStaticLib/SccDeviceLib/SccDeviceLib.h"
#  include "../../../SccLib/SccStaticLib/SccDeviceLib/ErrorCodes.h"
#endif
									 

int main()
{
	TestMediator t;
	t.Start();
	return 0;
}
