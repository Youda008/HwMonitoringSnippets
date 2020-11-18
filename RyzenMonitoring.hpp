//======================================================================================================================
//  Utils to get CPU temperature using RadeonMasterMonitoringSDK
//======================================================================================================================

#include <tchar.h>

namespace ryzen {


enum class InitStatus
{
	Success,
	DriverNotInstalled,
	PlatformInitFailed,
	CpuNotFound
};
const TCHAR * InitStatusToStr( InitStatus errCode );

InitStatus initMonitoring();

void quitMonitoring();

enum class TemperatureStatus
{
	NotInitialized,
	UnknownError,
	Success,
	InvalidValue,
	NotImplemented,
	CoresParked,
	Unsupported,
};
const TCHAR * TemperatureStatusToStr( TemperatureStatus errCode );

TemperatureStatus getCPUTemperature( double & temperature );


} // namespace ryzen
