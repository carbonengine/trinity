#pragma once
#ifndef Tr2AdapterStructures_H
#define Tr2AdapterStructures_H

#include "Tr2RenderContextEnum.h"

struct AdapterGuid
{
    uint32_t data1;
    uint16_t data2;
    uint16_t data3;
    uint8_t data4[8];
};

struct Tr2DisplayModeInfo
{
	uint32_t width;
	uint32_t height;
	uint32_t refreshRateNumerator;
	uint32_t refreshRateDenominator;
	Tr2RenderContextEnum::PixelFormat format;
    Tr2RenderContextEnum::ScanlineOrdering scanlineOrdering;
    Tr2RenderContextEnum::DisplayScaling scaling;
};

struct Tr2VideoDriverInfo
{
	int64_t driverVersion;
	std::string driverVersionString;
	std::string driverVendor;
	std::string driverDate;
	bool isOptimus;
	bool isAmdDynamicSwitchable;
};

struct Tr2AdapterInfo
{
	std::string driver;
	std::wstring description;
	std::string deviceName;
	int64_t driverVersion;
	uint32_t vendorID;
	uint32_t deviceID;
	uint32_t subSystemID;
	uint32_t revision;
	AdapterGuid deviceIdentifier;
};

struct Tr2PresentParametersAL
{
	Tr2DisplayModeInfo mode;
	uint32_t backBufferCount;
	uint32_t msaaType;
	uint32_t msaaQuality;
	Tr2RenderContextEnum::SwapEffect swapEffect;
	Tr2WindowHandle outputWindow;
	bool windowed;
	bool software;
	Tr2RenderContextEnum::PresentInterval presentInterval;
};

#endif // Tr2AdapterStructures_H