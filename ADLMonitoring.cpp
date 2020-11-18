//======================================================================================================================
//  Utils to get GPU temperature using AMD Display Library
//======================================================================================================================

#include "ADLMonitoring.hpp"

#include "adl_defines.h"
#include "adl_structures.h"
#include "adl_sdk.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <tchar.h>

#include <cstdlib>
#include <string>
#include <vector>


namespace adl {


typedef int (* ADL2_MAIN_CONTROL_CREATE)( ADL_MAIN_MALLOC_CALLBACK callback, int iEnumConnectedAdapters, ADL_CONTEXT_HANDLE * context );
typedef int (* ADL2_MAIN_CONTROL_DESTROY)( ADL_CONTEXT_HANDLE context );
typedef int (* ADL2_ADAPTER_NUMBEROFADAPTERS_GET)( ADL_CONTEXT_HANDLE context, int * lpNumAdapters );
typedef int (* ADL2_ADAPTER_ADAPTERINFO_GET)( ADL_CONTEXT_HANDLE context, LPAdapterInfo lpInfo, int iInputSize );
typedef int (* ADL2_OVERDRIVE_CAPS)( ADL_CONTEXT_HANDLE context, int iAdapterIndex, int * iSupported, int * iEnabled, int * iVersion );
typedef int (* ADL2_OVERDRIVE5_TEMPERATURE_GET)( ADL_CONTEXT_HANDLE context, int iAdapterIndex, int iThermalControllerIndex, ADLTemperature * lpTemperature );
typedef int (* ADL2_OVERDRIVE6_TEMPERATURE_GET)( ADL_CONTEXT_HANDLE context, int iAdapterIndex, int * lpTemperature );
typedef int (* ADL2_OVERDRIVEN_TEMPERATURE_GET)( ADL_CONTEXT_HANDLE context, int iAdapterIndex, int iTemperatureType, int * iTemperature );
typedef int (* ADL2_NEW_QUERYPMLOGDATA_GET)( ADL_CONTEXT_HANDLE context, int iAdapterIndex, ADLPMLogDataOutput * lpDataOutput );

ADL2_MAIN_CONTROL_CREATE          ADL2_Main_Control_Create = NULL;
ADL2_MAIN_CONTROL_DESTROY         ADL2_Main_Control_Destroy = NULL;
ADL2_ADAPTER_NUMBEROFADAPTERS_GET ADL2_Adapter_NumberOfAdapters_Get = NULL;
ADL2_ADAPTER_ADAPTERINFO_GET      ADL2_Adapter_AdapterInfo_Get = NULL;
ADL2_OVERDRIVE_CAPS               ADL2_Overdrive_Caps = NULL;
ADL2_OVERDRIVE5_TEMPERATURE_GET   ADL2_Overdrive5_Temperature_Get = NULL;
ADL2_OVERDRIVE6_TEMPERATURE_GET   ADL2_Overdrive6_Temperature_Get = NULL;
ADL2_OVERDRIVEN_TEMPERATURE_GET   ADL2_OverdriveN_Temperature_Get = NULL;
ADL2_NEW_QUERYPMLOGDATA_GET       ADL2_New_QueryPMLogData_Get = NULL;


static HINSTANCE hDLL;
static ADL_CONTEXT_HANDLE context = NULL;
static int iNumberAdapters;
static LPAdapterInfo lpAdapterInfo = NULL;
static std::vector< int > adapterAPIVersion;

static void * __stdcall ADL_Main_Memory_Alloc( int iSize )
{
	return malloc(iSize);
}

static void __stdcall ADL_Main_Memory_Free( void * * lpBuffer )
{
	if (NULL != *lpBuffer)
	{
		free( *lpBuffer );
		*lpBuffer = NULL;
	}
}

const TCHAR * InitStatusToStr( InitStatus errCode )
{
	static const TCHAR * errorStrings [] =
	{
		_T("Success"),
		_T("Failed to load ADL library"),
		_T("Required library functions not found"),
		_T("ADL2_Main_Control_Create failed"),
		_T("ADL2_Adapter_NumberOfAdapters_Get failed"),
		_T("No GPU has been detected"),
		_T("ADL2_Adapter_AdapterInfo_Get failed"),
		_T("Getting value from temperature sensor failed"),
	};
	if (size_t(errCode) >= size_t(InitStatus::Success) && size_t(errCode) <= size_t(InitStatus::GetAdapterInfoFailed))
		return errorStrings[ size_t(errCode) ];
	else
		return _T("<invalid>");
}

InitStatus initMonitoring()
{
	hDLL = LoadLibrary( _T("atiadlxx.dll") );
	if (hDLL == NULL)
	{
		return InitStatus::LoadLibraryFailed;
	}

	// load all the functions we need

	ADL2_Main_Control_Create          = (ADL2_MAIN_CONTROL_CREATE)         GetProcAddress( hDLL, "ADL2_Main_Control_Create" );
	ADL2_Main_Control_Destroy         = (ADL2_MAIN_CONTROL_DESTROY)        GetProcAddress( hDLL, "ADL2_Main_Control_Destroy" );
	ADL2_Adapter_NumberOfAdapters_Get = (ADL2_ADAPTER_NUMBEROFADAPTERS_GET)GetProcAddress( hDLL, "ADL2_Adapter_NumberOfAdapters_Get" );
	ADL2_Adapter_AdapterInfo_Get      = (ADL2_ADAPTER_ADAPTERINFO_GET)     GetProcAddress( hDLL, "ADL2_Adapter_AdapterInfo_Get" );
	ADL2_Overdrive_Caps               = (ADL2_OVERDRIVE_CAPS)              GetProcAddress( hDLL, "ADL2_Overdrive_Caps" );
	ADL2_Overdrive5_Temperature_Get   = (ADL2_OVERDRIVE5_TEMPERATURE_GET)  GetProcAddress( hDLL, "ADL2_Overdrive5_Temperature_Get" );
	ADL2_Overdrive6_Temperature_Get   = (ADL2_OVERDRIVE6_TEMPERATURE_GET)  GetProcAddress( hDLL, "ADL2_Overdrive6_Temperature_Get" );
	ADL2_OverdriveN_Temperature_Get   = (ADL2_OVERDRIVEN_TEMPERATURE_GET)  GetProcAddress( hDLL, "ADL2_OverdriveN_Temperature_Get" );
	ADL2_New_QueryPMLogData_Get       = (ADL2_NEW_QUERYPMLOGDATA_GET)      GetProcAddress( hDLL, "ADL2_New_QueryPMLogData_Get" );
	if (!ADL2_Main_Control_Create)           return InitStatus::LibraryFunctionsNotFound;
	if (!ADL2_Main_Control_Destroy)          return InitStatus::LibraryFunctionsNotFound;
	if (!ADL2_Adapter_NumberOfAdapters_Get)  return InitStatus::LibraryFunctionsNotFound;
	if (!ADL2_Adapter_AdapterInfo_Get)       return InitStatus::LibraryFunctionsNotFound;
	if (!ADL2_Overdrive_Caps)                return InitStatus::LibraryFunctionsNotFound;
	if (!ADL2_Overdrive5_Temperature_Get)    return InitStatus::LibraryFunctionsNotFound;
	if (!ADL2_Overdrive6_Temperature_Get)    return InitStatus::LibraryFunctionsNotFound;
	if (!ADL2_OverdriveN_Temperature_Get)    return InitStatus::LibraryFunctionsNotFound;
	if (!ADL2_New_QueryPMLogData_Get)        return InitStatus::LibraryFunctionsNotFound;

	if (ADL2_Main_Control_Create( ADL_Main_Memory_Alloc, 1, &context ) != ADL_OK)
	{
		return InitStatus::MainControlCreateFailed;
	}

	// retrieve info about installed graphical adapters

	if (ADL2_Adapter_NumberOfAdapters_Get( context, &iNumberAdapters ) != ADL_OK)
	{
		return InitStatus::GetNumberOfAdaptersFailed;
	}

	if (iNumberAdapters < 1)
	{
		return InitStatus::NoGPUFound;
	}

	lpAdapterInfo = new AdapterInfo [ iNumberAdapters ];
	memset( lpAdapterInfo, 0, sizeof(AdapterInfo) * iNumberAdapters );
	if (ADL2_Adapter_AdapterInfo_Get( context, lpAdapterInfo, sizeof(AdapterInfo) * iNumberAdapters ) != ADL_OK)
	{
		return InitStatus::GetAdapterInfoFailed;
	}

	// for each adapter get its supported API version

	for (int adapterIdx = 0; adapterIdx < iNumberAdapters; ++adapterIdx)
	{
		int isSupported;
		int isEnabled;
		int version;
		int status = ADL2_Overdrive_Caps( context, adapterIdx, &isSupported, &isEnabled, &version );
		if (status != ADL_OK)
		{
			adapterAPIVersion.push_back( 0 );
		}
		else if (!isSupported)
		{
			adapterAPIVersion.push_back( -1 );
		}
		else if (!isEnabled)
		{
			adapterAPIVersion.push_back( -2 );
		}
		else
		{
			adapterAPIVersion.push_back( version );
		}
	}

	return InitStatus::Success;
}

void quitMonitoring()
{
	delete lpAdapterInfo;
	ADL2_Main_Control_Destroy( context );
	FreeLibrary( hDLL );
}

int getNumAdapters()
{
	return iNumberAdapters;
}

const AdapterInfo * getAdapterInfo( int adapterIdx )
{
	if (adapterIdx < iNumberAdapters)
	{
		return &lpAdapterInfo[ adapterIdx ];
	}
	return nullptr;
}

enum class ADLODNTemperatureType
{
	Core = 1,
	Memory = 2,
	VrmCore = 3,
	VrmMemory = 4,
	Liquid = 5,
	Plx = 6,
	Hotspot = 7,
};

const TCHAR * TemperatureStatusToStr( TemperatureStatus errCode )
{
	static const TCHAR * errorStrings [] =
	{
		_T("Success"),
		_T("Failed to retrieve API version of this device"),
		_T("This function is not supported on this device"),
		_T("This function is disabled on this device"),
		_T("Failed to retrieve value from temperature sensor"),
	};
	if (size_t(errCode) >= size_t(TemperatureStatus::Success) && size_t(errCode) <= size_t(TemperatureStatus::GetTemperatureFailed))
		return errorStrings[ size_t(errCode) ];
	else
		return _T("<invalid>");
}

TemperatureStatus getGPUTemperature( int adapterIdx, float & temperature )
{
	int version = adapterAPIVersion[ adapterIdx ];
	if (version == 0)
	{
		return TemperatureStatus::GetVersionFailed;
	}
	else if (version == -1)
	{
		return TemperatureStatus::NotSupported;
	}
	else if (version == -2)
	{
		return TemperatureStatus::Disabled;
	}
	else if (version == 5)
	{
		ADLTemperature adlTemp;
		if (ADL2_Overdrive5_Temperature_Get( context, adapterIdx, 0, &adlTemp ) != ADL_OK)
		{
			return TemperatureStatus::GetTemperatureFailed;
		}
		temperature = float( adlTemp.iTemperature ) / 1000;
	}
	else if (version == 6)
	{
		int tempInt;
		if (ADL2_Overdrive6_Temperature_Get( context, adapterIdx, &tempInt ) != ADL_OK)
		{
			return TemperatureStatus::GetTemperatureFailed;
		}
		temperature = float( tempInt ) / 1000;
	}
	else if (version == 7)
	{
		int tempInt;
		if (ADL2_OverdriveN_Temperature_Get( context, adapterIdx, int(ADLODNTemperatureType::Core), &tempInt ) != ADL_OK)
		{
			return TemperatureStatus::GetTemperatureFailed;
		}
		temperature = float( tempInt ) / 1000;
	}
	else if (version == 8)
	{
		ADLPMLogDataOutput data;
		if (ADL2_New_QueryPMLogData_Get( context, adapterIdx, &data ) != ADL_OK)
		{
			return TemperatureStatus::GetTemperatureFailed;
		}
		int type = ADLSensorType::PMLOG_TEMPERATURE_EDGE;
		if (!data.sensors[ type ].supported)
		{
			return TemperatureStatus::NotSupported;
		}
		temperature = float( data.sensors[ type ].value ) * 1.0f;
	}
	return TemperatureStatus::Success;
}


} // namespace adl
