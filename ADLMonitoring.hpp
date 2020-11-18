//======================================================================================================================
//  Utils to get GPU temperature using AMD Display Library
//======================================================================================================================

#include "adl_structures.h"

#include <tchar.h>


namespace adl {


enum class InitStatus
{
	Success,
	LoadLibraryFailed,
	LibraryFunctionsNotFound,
	MainControlCreateFailed,
	GetNumberOfAdaptersFailed,
	NoGPUFound,
	GetAdapterInfoFailed,
};
const TCHAR * InitStatusToStr( InitStatus errCode );

InitStatus initMonitoring();

void quitMonitoring();

int getNumAdapters();

const AdapterInfo * getAdapterInfo( int idx );

enum class TemperatureStatus
{
	Success,
	GetVersionFailed,
	NotSupported,
	Disabled,
	GetTemperatureFailed,
};
const TCHAR * TemperatureStatusToStr( TemperatureStatus errCode );

TemperatureStatus getGPUTemperature( int adapterIdx, float & temperature );


} // namespace adl
