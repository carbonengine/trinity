#include "StdAfx.h"
#include "gtest/gtest.h"

const char* g_moduleName = "TrinityEngineTest";

int main( int argc, char** argv )
{
	CCP::SetLogMainThreadId();

	::testing::InitGoogleTest( &argc, argv );
	return RUN_ALL_TESTS();
}
