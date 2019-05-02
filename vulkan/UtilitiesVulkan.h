#pragma once

#include "VkResult.h"

namespace TrinityALImpl
{

	template <typename Func, typename A1, typename Result>
	ALResult QueryArray( Func func, A1 a1, std::vector<Result>& result )
	{
		result.clear();
		uint32_t count = 0;
		CR_RETURN_HR( Vk2Al( ( *func )( a1, &count, nullptr ) ) );
		if( count )
		{
			result.resize( count );
			CR_RETURN_HR( Vk2Al( ( *func )( a1, &count, &result[0] ) ) );
		}
		return S_OK;
	}

	template <typename Func, typename A1, typename A2, typename Result>
	ALResult QueryArray( Func func, A1 a1, A2 a2, std::vector<Result>& result )
	{
		result.clear();
		uint32_t count = 0;
		CR_RETURN_HR( Vk2Al( ( *func )( a1, a2, &count, nullptr ) ) );
		if( count )
		{
			result.resize( count );
			CR_RETURN_HR( Vk2Al( ( *func )( a1, a2, &count, &result[0] ) ) );
		}
		return S_OK;
	}

	template <typename Func, typename A1, typename Result>
	void QueryArrayNoFail( Func func, A1 a1, std::vector<Result>& result )
	{
		result.clear();
		uint32_t count = 0;
		( *func )( a1, &count, nullptr );
		if( count )
		{
			result.resize( count );
			( *func )( a1, &count, &result[0] );
		}
	}

	template <typename Func, typename A1, typename Result>
	ALResult QueryArrayNotEmpty( Func func, A1 a1, std::vector<Result>& result )
	{
		FORWARD_HR( QueryArray( func, a1, result ) );
		if( result.empty() )
		{
			return E_FAIL;
		}
		return S_OK;
	}

	template <typename Func, typename A1, typename A2, typename Result>
	ALResult QueryArrayNotEmpty( Func func, A1 a1, A2 a2, std::vector<Result>& result )
	{
		FORWARD_HR( QueryArray( func, a1, a2, result ) );
		if( result.empty() )
		{
			return E_FAIL;
		}
		return S_OK;
	}
}