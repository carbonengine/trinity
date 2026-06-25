// Copyright © 2023 CCP ehf.

#ifndef TrinityALTest_StdAfx_H
#define TrinityALTest_StdAfx_H

#include <string>

#ifdef _WIN32

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <Windows.h>
typedef HWND Tr2WindowHandle;
#elif defined( __APPLE__ )
#include <objc/objc-runtime.h>
typedef id Tr2WindowHandle;
#else
#include <cstdint>
typedef uintptr_t Tr2WindowHandle;
#endif

#include <include/TrinityAL.h>

#if ( TRINITY_PLATFORM == TRINITY_DIRECTX11 )
#define SHADER_PATH Shaders.DX11
#elif ( TRINITY_PLATFORM == TRINITY_STUB )
#define SHADER_PATH Shaders.DX11
#elif ( TRINITY_PLATFORM == TRINITY_DIRECTX12 )
#define SHADER_PATH Shaders.DX12
#elif ( TRINITY_PLATFORM == TRINITY_METAL )
#define SHADER_PATH Shaders.metal
#else
#error Define shader path for this TrinityAL platform
#endif

// clang-format off
#define INCLUDE_SHADER_CODE( name ) CCP_STRINGIZE( SHADER_PATH/name.h )
// clang-format on

#include "gtest/gtest.h"

#ifndef ASSERT_HRESULT_SUCCEEDED

namespace testing
{
namespace internal
{
inline AssertionResult HRESULTFailureHelper( const char* expr, const char* expected, HRESULT hr )
{
#if __APPLE__
	std::stringstream error_hex;
	error_hex << "0x" << std::hex << hr;
	return AssertionFailure() << "Expected: " << expr << " " << expected << ".\n"
							  << "  Actual: " << error_hex.str() << "\n";
#else
	const std::string error_hex( "0x" + String::FormatHexInt( hr ) );
	return AssertionFailure() << "Expected: " << expr << " " << expected << ".\n"
							  << "  Actual: " << error_hex << "\n";
#endif
}

inline AssertionResult IsHRESULTSuccess( const char* expr, HRESULT hr )
{
	if( SUCCEEDED( hr ) )
	{
		return AssertionSuccess();
	}
	return HRESULTFailureHelper( expr, "succeeds", hr );
}

inline AssertionResult IsHRESULTFailure( const char* expr, HRESULT hr )
{
	if( FAILED( hr ) )
	{
		return AssertionSuccess();
	}
	return HRESULTFailureHelper( expr, "succeeds", hr );
}
}
}

#define EXPECT_HRESULT_SUCCEEDED( expr ) EXPECT_PRED_FORMAT1( ::testing::internal::IsHRESULTSuccess, ( expr ) )
#define ASSERT_HRESULT_SUCCEEDED( expr ) ASSERT_PRED_FORMAT1( ::testing::internal::IsHRESULTSuccess, ( expr ) )
#define EXPECT_HRESULT_FAILED( expr ) EXPECT_PRED_FORMAT1( ::testing::internal::IsHRESULTFailure, ( expr ) )
#define ASSERT_HRESULT_FAILED( expr ) ASSERT_PRED_FORMAT1( ::testing::internal::IsHRESULTFailure, ( expr ) )

#endif

#endif
