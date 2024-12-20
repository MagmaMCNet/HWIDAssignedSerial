#include "HardwareInfo.h"
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "iphlpapi.lib")

#include "SMBIOS.h"
#include <memory>
#include "WCharHelper.h"


std::string HardwareInfo::GetCPU()
{
	std::array<int, 4> integerbuffer;
	constexpr size_t sizeofintegerbuffer = sizeof(int) * integerbuffer.size();

	std::array<char, 64> charbuffer = {};
	// https://docs.microsoft.com/en-us/cpp/intrinsics/cpuid-cpuidex?view=vs-2019
	constexpr std::array<int, 3> functionids = {
		// Manufacturer
		0x8000'0002,
		// Model
		0x8000'0003,
		// Clockspeed
		0x8000'0004,
	};

	std::string cpuname;

	for (int id : functionids)
	{
		__cpuid(integerbuffer.data(), id);

		std::memcpy(charbuffer.data(), integerbuffer.data(), sizeofintegerbuffer);

		cpuname += std::string(charbuffer.data());
	}
	return cpuname;
}

std::string HardwareInfo::GetGPU()
{
	IDXGIFactory* factory = nullptr;
	if (!SUCCEEDED(CreateDXGIFactory(__uuidof(IDXGIFactory), (void**)&factory)) && factory)
		return "";
	IDXGIAdapter* adapter = nullptr;
	if (!SUCCEEDED(factory->EnumAdapters(0, &adapter)) && adapter)
		return "";
	DXGI_ADAPTER_DESC adapterdesc;
	if (!SUCCEEDED(adapter->GetDesc(&adapterdesc)))
		return "";
	return WCharHelper::WStringToConstChar(adapterdesc.Description);

	adapter->Release();
	factory->Release();
}
std::vector<std::string> HardwareInfo::GetMacAddresses()
{
	std::vector<std::string> ret;
	DWORD buffer = 0;

	if (GetAdaptersInfo(NULL, &buffer) == ERROR_BUFFER_OVERFLOW)
	{
		PIP_ADAPTER_INFO adapterinfo = (PIP_ADAPTER_INFO)malloc(buffer);

		if (adapterinfo)
		{
			if (GetAdaptersInfo(adapterinfo, &buffer) == ERROR_SUCCESS)
			{
				PIP_ADAPTER_INFO pipadapterinfo = adapterinfo;
				while (pipadapterinfo)
				{
					std::string mac;
					for (int i = 0; i < pipadapterinfo->AddressLength; i++)
					{
						char buffer[3];
						if (i > 0) mac += ":";
						sprintf_s(buffer, "%02X", pipadapterinfo->Address[i]);
						mac += buffer;
					}
					ret.push_back(mac);
					pipadapterinfo = pipadapterinfo->Next;
				}
			}
			free(adapterinfo);
		}
	}

	return ret;
}


size_t HardwareInfo::GetTotalMemory()
{
	MEMORYSTATUSEX status;
	status.dwLength = sizeof(status);
	if (!GlobalMemoryStatusEx(&status))
		return 1024;
	return status.ullTotalPhys / (static_cast<unsigned long long>(1024) * 1024);
}
std::vector<std::string> HardwareInfo::GetDriveSerialNumbers()
{
	std::vector<std::string> ret;
	int drivenumber = 0;
	while (true)
	{
		std::wstring path = L"\\\\.\\PhysicalDrive" + std::to_wstring(drivenumber);

		// Get a handle to the physical drive
		HANDLE h = CreateFileW(path.c_str(), 0, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);

		if (h == INVALID_HANDLE_VALUE) // End of drives
			break;


		// Automatic Cleanup, Smart Pointers
		std::unique_ptr<std::remove_pointer<HANDLE>::type, void(*)(HANDLE)> hDevice{ h, [](HANDLE handle) { CloseHandle(handle); } };

		STORAGE_PROPERTY_QUERY storagepropertyquery{};
		storagepropertyquery.PropertyId = StorageDeviceProperty;
		storagepropertyquery.QueryType = PropertyStandardQuery;

		STORAGE_DESCRIPTOR_HEADER storagedescriptorheader{};

		DWORD bytesret = 0;
		if (!DeviceIoControl(hDevice.get(), IOCTL_STORAGE_QUERY_PROPERTY, &storagepropertyquery, sizeof(STORAGE_PROPERTY_QUERY), &storagedescriptorheader, sizeof(STORAGE_DESCRIPTOR_HEADER), &bytesret, NULL))
			continue;


		const DWORD buffersize = storagedescriptorheader.Size;
		std::unique_ptr<BYTE[]> buffer{ new BYTE[buffersize]{} };


		if (!DeviceIoControl(hDevice.get(), IOCTL_STORAGE_QUERY_PROPERTY, &storagepropertyquery, sizeof(STORAGE_PROPERTY_QUERY), buffer.get(), buffersize, &bytesret, NULL))
			continue;


		// Read the serial number out of the output buffer
		STORAGE_DEVICE_DESCRIPTOR* devicedescriptor = reinterpret_cast<STORAGE_DEVICE_DESCRIPTOR*>(buffer.get());
		const DWORD serialnumberoffset = devicedescriptor->SerialNumberOffset;

		if (serialnumberoffset != 0)
		{
			const char* serialnumber = reinterpret_cast<const char*>(buffer.get() + serialnumberoffset);
			ret.push_back(serialnumber);
		}
		drivenumber += 1;
	}

	return ret;
}