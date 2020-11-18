Code snippets in C/C++ for extracting various sensor values from hardware components such as temperature or fan speed

### RyzenMonitoring

This is how to extract temperature from AMD Ryzen CPU using their official SDK (Windows only).

To use this you need to download and install Ryzen Master Monitoring SDK: https://developer.amd.com/amd-ryzentm-master-monitoring-sdk/
then install their driver via their `DriverUtility.bat` and setup include and library paths of your project to point to the SDK directory.

### ADLMonitoring

This is how to get temperature, clocks and fan speeds from most Radeon GPUs using the official SDK and driver.
(This can also work on Linux with some modifications)

To use this you need to clone the AMD Display Library repo: https://github.com/GPUOpen-LibrariesAndSDKs/display-library
install the official drivers from AMD and setup include directories to point to the SDK directory.
