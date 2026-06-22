// Copyright © 2024 CCP ehf.

#include "gtest/gtest.h"

extern std::string g_metalToolsPath;

class ThrowListener : public testing::EmptyTestEventListener
{
	void OnTestPartResult( const testing::TestPartResult& result ) override
	{
		if( result.type() == testing::TestPartResult::kFatalFailure )
		{
			throw testing::AssertionException( result );
		}
	}
};

int main( int argc, char** argv )
{
#if _WIN32
	char metalToolsPath[MAX_PATH] = { 0 };
	size_t metalToolsPathSize;
	if( getenv_s( &metalToolsPathSize, metalToolsPath, "METAL_TOOLS_PATH" ) == 0 )
	{
		g_metalToolsPath = metalToolsPath;
	}
#else
	if( auto metalToolsPath = getenv( "METAL_TOOLS_PATH" ) )
	{
		g_metalToolsPath = metalToolsPath;
	}
#endif

	for( int i = 1; i < argc; ++i )
	{
		if( strcmp( argv[i], "/metal" ) == 0 )
		{
			++i;
			if( i < argc )
			{
				g_metalToolsPath = argv[i];
			}
			else
			{
				return 1;
			}
		}
	}

	testing::InitGoogleTest( &argc, argv );
	testing::UnitTest::GetInstance()->listeners().Append( new ThrowListener );
	return RUN_ALL_TESTS();
}