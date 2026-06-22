// Copyright © 2023 CCP ehf.

#include "StdAfx.h"

TEST( ALResult, DefaultResultIsSuccess )
{
	ALResult result;
	EXPECT_EQ( S_OK, result.GetResult() );
}

TEST( ALResult, CanCreateALResultFromHRESULT )
{
	ALResult result( E_INVALIDARG );
	EXPECT_EQ( E_INVALIDARG, result.GetResult() );
}

TEST( ALResult, CanCopyResult )
{
	ALResult result1;
	ALResult result2( result1 );
	EXPECT_EQ( result1.GetResult(), result2.GetResult() );

	ALResult result3( E_INVALIDARG );
	ALResult result4( result3 );
	EXPECT_EQ( result3.GetResult(), result4.GetResult() );
}

TEST( ALResult, CanAssignResult )
{
	ALResult result1( E_INVALIDARG );
	ALResult result2( S_OK );
	result2 = result1;
	EXPECT_EQ( result1.GetResult(), result2.GetResult() );

	ALResult result3( E_INVALIDARG );
	ALResult result4;
	result4 = result3;
	EXPECT_EQ( result3.GetResult(), result4.GetResult() );
}

TEST( ALResult, CanCastALResultToHRESULT )
{
	ALResult result1;
	EXPECT_EQ( S_OK, HRESULT( result1 ) );

	ALResult result2( E_INVALIDCALL );
	EXPECT_EQ( E_INVALIDCALL, HRESULT( result2 ) );
}

TEST( ALResult, CanQueryResultCategory )
{
	ALResult result1;
	EXPECT_EQ( ALResult::SUCCESS, result1.GetCategory() );

	ALResult result2( E_INVALIDCALL );
	EXPECT_EQ( ALResult::INVALID_CALL, result2.GetCategory() );

	ALResult result3( E_INVALIDARG );
	EXPECT_EQ( ALResult::INVALID_CALL, result3.GetCategory() );

	ALResult result4( E_OUTOFMEMORY );
	EXPECT_EQ( ALResult::OUT_OF_MEMORY, result4.GetCategory() );

	ALResult result5( E_DEVICELOST );
	EXPECT_EQ( ALResult::DEVICE_LOST, result5.GetCategory() );

	ALResult result6( HRESULT( -34 ) );
	EXPECT_EQ( ALResult::OTHER, result6.GetCategory() );
}


TEST( ALResult, CanCheckIfResultIsSuccessful )
{
	ALResult result1;
	EXPECT_TRUE( BeIsSuccess( result1 ) );

	ALResult result11( HRESULT( 121 ) );
	EXPECT_TRUE( BeIsSuccess( result11 ) );

	ALResult result2( E_INVALIDCALL );
	EXPECT_FALSE( BeIsSuccess( result2 ) );

	ALResult result3( E_INVALIDARG );
	EXPECT_FALSE( BeIsSuccess( result3 ) );

	ALResult result4( E_OUTOFMEMORY );
	EXPECT_FALSE( BeIsSuccess( result4 ) );

	ALResult result5( E_DEVICELOST );
	EXPECT_FALSE( BeIsSuccess( result5 ) );

	ALResult result6( HRESULT( -34 ) );
	EXPECT_FALSE( BeIsSuccess( result6 ) );
}


TEST( ALResult, CanGetErrorMessage )
{
	const char* str = BeGetErrorMessage( ALResult( E_INVALIDARG ) );
	EXPECT_NE( nullptr, str );
}
