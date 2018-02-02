#ifndef TrinityALTest_StdAfx_H
#define TrinityALTest_StdAfx_H

#include <string>

#ifdef _WIN32
#include <Windows.h>
typedef HWND Tr2WindowHandle;
#else
#include <cstdint>
typedef uintptr_t Tr2WindowHandle;
#endif

#include "TrinityAL/include/TrinityAL.h"

#if( TRINITY_PLATFORM==TRINITY_DIRECTX9 )
#define SHADER_PATH Shaders.dx9
#elif( TRINITY_PLATFORM==TRINITY_DIRECTX11 )
#define SHADER_PATH Shaders.dx11
#elif( TRINITY_PLATFORM==TRINITY_OPENGLES2 )
#define SHADER_PATH Shaders.gles2
#elif( TRINITY_PLATFORM==TRINITY_STUB )
#define SHADER_PATH Shaders.dx11
#endif

#define INCLUDE_SHADER_CODE( name ) CCP_STRINGIZE(SHADER_PATH/name.h)

#include "gtest/gtest.h"

#ifndef ASSERT_HRESULT_SUCCEEDED

namespace testing
{
    namespace internal
    {
        inline AssertionResult HRESULTFailureHelper( const char* expr, const char* expected, HRESULT hr )
        {
            const String error_hex( String::Format( "0x%08X", hr ) );
            return AssertionFailure() << "Expected: " << expr << " " << expected << ".\n"
                << "  Actual: " << error_hex << "\n";
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

#define EXPECT_HRESULT_SUCCEEDED(expr) EXPECT_PRED_FORMAT1(::testing::internal::IsHRESULTSuccess, (expr))
#define ASSERT_HRESULT_SUCCEEDED(expr) ASSERT_PRED_FORMAT1(::testing::internal::IsHRESULTSuccess, (expr))
#define EXPECT_HRESULT_FAILED(expr) EXPECT_PRED_FORMAT1(::testing::internal::IsHRESULTFailure, (expr))
#define ASSERT_HRESULT_FAILED(expr) ASSERT_PRED_FORMAT1(::testing::internal::IsHRESULTFailure, (expr))

#endif

#endif