//======================================================================================================================
//  Utils to get CPU temperature using RadeonMasterMonitoringSDK
//======================================================================================================================

#include "RyzenMonitoring.hpp"

#include "IPlatform.h"
#include "IDeviceManager.h"
#include "ICPUEx.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <tchar.h>


namespace ryzen {


static IPlatform * platform = nullptr;
static ICPUEx * cpu = nullptr;


static bool isDriverInstalled()
{
	SC_HANDLE hSCM = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	if (!hSCM)
	{
		return false;
	}

	SC_HANDLE hOpenService = OpenService(hSCM, _T("AMDRyzenMasterDriverV16"), SC_MANAGER_ALL_ACCESS);
	if (!hOpenService)
	{
		CloseServiceHandle(hSCM);
		return false;
	}

	SERVICE_STATUS ServiceStatus;
	BOOL queried = QueryServiceStatus(hOpenService, &ServiceStatus);
	if (!queried || ServiceStatus.dwCurrentState != SERVICE_RUNNING)
	{
		CloseServiceHandle(hOpenService);
		CloseServiceHandle(hSCM);
		return false;
	}

	CloseServiceHandle(hOpenService);
	CloseServiceHandle(hSCM);
	return true;
}

InitStatus initMonitoring()
{
	if (!isDriverInstalled())
	{
		return InitStatus::DriverNotInstalled;
	}

	platform = &GetPlatform();
	if (!platform->Init())
	{
		return InitStatus::PlatformInitFailed;
	}

	IDeviceManager& deviceManager = platform->GetIDeviceManager();

	cpu = (ICPUEx*)deviceManager.GetDevice( dtCPU, 0 );
	if (!cpu)
	{
		platform->UnInit();
		platform = nullptr;
		return InitStatus::CpuNotFound;
	}

	return InitStatus::Success;
}

const TCHAR * InitStatusToStr( InitStatus errCode )
{
	static const TCHAR * errorStrings [] =
	{
		_T("success"),
		_T("driver is not installed"),
		_T("failed to init platform"),
		_T("cpu not found")
	};
	if (size_t(errCode) >= size_t(InitStatus::Success) && size_t(errCode) <= size_t(InitStatus::CpuNotFound))
		return errorStrings[ size_t(errCode) ];
	else
		return _T("<invalid>");
}

void quitMonitoring()
{
	if (platform)
	{
		platform->UnInit();
		platform = nullptr;
	}
}

TemperatureStatus getCPUTemperature( double & temperature )
{
	if (!cpu)
	{
		return TemperatureStatus::NotInitialized;
	}

	CPUParameters cpuParams;
	int result = cpu->GetCPUParameters( cpuParams );
	if (result != 0)
	{
		return TemperatureStatus( result + 2 );
	}

	temperature = cpuParams.dTemperature;
	return TemperatureStatus::Success;
}

const TCHAR * TemperatureStatusToStr( TemperatureStatus errCode )
{
	static const TCHAR * errorStrings [] =
	{
		_T("Ryzen monitoring not initialized"),
		_T("Failure"),
		_T("Success"),
		_T("Invalid value"),
		_T("Method is not implemented by the BIOS"),
		_T("Cores are already parked. First Enable all the cores"),
		_T("Unsupported Function")
	};
	if (size_t(errCode) >= size_t(TemperatureStatus::NotInitialized) && size_t(errCode) <= size_t(TemperatureStatus::Unsupported))
		return errorStrings[ size_t(errCode) ];
	else
		return _T("<invalid>");
}


} // namespace ryzen
