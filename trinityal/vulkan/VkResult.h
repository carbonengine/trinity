#pragma once

#include "ALResult.h"

#if TRINITY_PLATFORM == TRINITY_VULKAN

inline ALResult Vk2Al( VkResult result )
{
	switch( result )
	{
	case VK_SUCCESS:
		return S_OK;
	default:
		return E_FAIL;
	}
}

#endif