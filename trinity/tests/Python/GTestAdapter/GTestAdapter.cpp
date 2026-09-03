// Copyright © 2026 CCP ehf.

#include <gtest/gtest.h>
#include <string>
#include <array>
#include <cstdlib>
#include <cstdio>

#include "TouchedPyFiles.h"


// Including this test ensures that gtest is linked into the executable.
TEST( DummyTestSuite, DummyTest )
{
	EXPECT_TRUE( true );
}

#ifdef _WIN32
#define popen _popen
#define pclose _pclose
#endif

int main( int argc, char* argv[] )
{
	std::string command = std::string( EXECUTABLE_PATH ) + " /inherit /buildflavor=" BUILDFLAVOR " /py " + TEST_PATH;
	for( int i = 1; i < argc; ++i )
	{
		command += " ";
		command += argv[i];
	}
#ifdef _WIN32
	_putenv_s( "PYTHONPATH", PYTHON_LIB_PATH );
	_putenv_s( "TRINITYPLATFORM", TRINITYPLATFORM );
	_putenv_s( "TRINITYFLAVOR", TRINITYFLAVOR );
#else
	setenv( "PYTHONPATH", PYTHON_LIB_PATH, 1 );
	setenv( "TRINITYPLATFORM", TRINITYPLATFORM, 1 );
	setenv( "TRINITYFLAVOR", TRINITYFLAVOR, 1 );
#endif
	FILE* pipe = popen( command.c_str(), "r" );
	if( !pipe )
	{
		return -1;
	}

	std::array<char, 128> buffer;
	while( fgets( buffer.data(), int( buffer.size() ), pipe ) != nullptr )
	{
		// output to stdout
		printf( "%s", buffer.data() );
	}
	return pclose( pipe );
}