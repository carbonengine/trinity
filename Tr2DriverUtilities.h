#pragma once
#ifndef Tr2DriverUtilities_H
#define Tr2DriverUtilities_H

#include "ALResult.h"
#include "Tr2AdapterStructures.h"

namespace Tr2DriverUtilities
{

ALResult GetDriverVersion( uint32_t deviceId, Tr2VideoDriverInfo& info );

}

#endif