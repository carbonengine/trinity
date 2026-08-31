// Copyright © 2024 CCP ehf.

#include "gtest/gtest.h"

extern std::string g_metalToolsPath;
bool g_metalCompilerAvailable = true;

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

#ifdef _WIN32
	std::ostringstream cmd;
	if( !g_metalToolsPath.empty() )
	{
		cmd << "\"" << g_metalToolsPath << "\\macos\\bin\\metal.exe\" --version";
	}
	else
	{
		char programFiles[MAX_PATH] = { 0 };
		size_t programFilesSize;
		getenv_s( &programFilesSize, programFiles, "PROGRAMFILES" );

		cmd << "\"" << std::string( programFiles ) << "\\Metal Developer Tools\\metal\\macos\\bin\\metal2.exe\" --version";
	}
	FILE* process = _popen( cmd.str().c_str(), "r" );
	char readBuffer[128];
	while( fgets( readBuffer, sizeof( readBuffer ), process ) )
	{
	}
	if( !process )
	{
		g_metalCompilerAvailable = false;
	}
	else
	{
		g_metalCompilerAvailable = _pclose( process ) == 0;
	}
#endif

	testing::InitGoogleTest( &argc, argv );
	testing::UnitTest::GetInstance()->listeners().Append( new ThrowListener );
	return RUN_ALL_TESTS();
}