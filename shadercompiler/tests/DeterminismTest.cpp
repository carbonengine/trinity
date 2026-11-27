// Copyright © 2026 CCP ehf.

// Compiled shaders and listings must not depend on thread timing or on leftover memory.

#include <chrono>
#include <cstring>
#include <filesystem>
#include <string>

#include "gtest/gtest.h"
#include "Macro.h"
#include "ModifiedTime.h"

TEST( Determinism, SourceHashIgnoresLineEndings )
{
	std::error_code ignored;
	auto temp = std::filesystem::temp_directory_path( ignored );
	if( temp.empty() )
	{
		GTEST_SKIP() << "Skipping test: no temp folder";
	}
	auto folder = temp / ( "ShaderCompilerDeterminismTest-" + std::to_string( std::chrono::steady_clock::now().time_since_epoch().count() ) );
	std::filesystem::create_directories( folder, ignored );

	// Each file has its own name: the include handler caches files by path.
	bool writeFailed = false;
	auto hash = [&]( const char* name, const char* contents ) {
		auto path = ( folder / name ).string();
		FILE* f = nullptr;
		writeFailed |= fopen_s( &f, path.c_str(), "wb" ) || fwrite( contents, strlen( contents ), 1, f ) != 1;
		if( f )
		{
			fclose( f );
		}
		return GetSourceHash( path.c_str(), {} );
	};
	auto lf = hash( "lf.fx", "float a;\nfloat b;\n" );
	auto crlf = hash( "crlf.fx", "float a;\r\nfloat b;\r\n" );
	auto actuallyDifferent = hash( "actuallyDifferent.fx", "float a;\nfloat c;\n" );
	std::filesystem::remove_all( folder, ignored );
	if( writeFailed )
	{
		GTEST_SKIP() << "Skipping test: can't write test files to " << folder.string();
	}

	EXPECT_EQ( lf, crlf ); // only differ by line breaks
	EXPECT_NE( actuallyDifferent, crlf ); // the file is actually read and hashed
}
