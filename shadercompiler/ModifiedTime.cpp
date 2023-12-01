#include "stdafx.h"
#include "ModifiedTime.h"
#include "WorkQueue.h"
#include "Macro.h"
#include "CachingIncludeHandler.h"
#include "EffectData.h"
#include "md5.h"
#include "ShaderCompilerConfig.h"


namespace
{
	CachingIncludeHandler s_includeHandler;

	char* GetNextWord( char* string )
	{

		char* end = string;
		while( *end && *end != ' ' && *end != '\n' )
		{
			++end;
		}
		switch (*end)
		{
		case 0:
			return end;
		case '\n':
			*end = 0;
			return end;
		default:
			*end = 0;
			return end + 1;
		}
	}


	struct HashCheckArguments
	{
		std::string sourcePath;
		std::string outputPath;
		std::vector<Macro> defines;
	};


	std::mutex s_modifiedOutputsCS;
	std::set<std::string> s_modifiedOutputs;

	const std::regex s_include( "#[[:space:]]*include[[:space:]]*[<\"]([^>\"]*)" );

	bool IsOutputUpToDate(const char* path, const std::string& sourceHash )
	{
		FILE* file = nullptr;
		fopen_s( &file, path, "rb" );
		if( !file )
		{
			return false;
		}
		uint32_t fileVersion;
		if( fread( &fileVersion, sizeof( fileVersion ), 1, file ) != 1 )
		{
			fclose( file );
			return false;
		}
		if( fileVersion != DATA_VERSION )
		{
			fclose( file );
			return false;
		}
		uint8_t compilerVersion[4];
		if( fread( compilerVersion, sizeof( compilerVersion ), 1, file ) != 1 )
		{
			fclose( file );
			return false;
		}
		if( compilerVersion[0] != ShaderCompilerVersion[0] || compilerVersion[1] != ShaderCompilerVersion[1] || compilerVersion[2] != ShaderCompilerVersion[2] )
		{
			fclose( file );
			return false;
		}

		char buffer[MD5::HashBytes * 2];
		if( fread( buffer, sizeof( buffer ), 1, file ) != 1 )
		{
			fclose( file );
			return false;
		}
		fclose( file );

		return sourceHash == std::string( buffer, buffer + sizeof( buffer ) );
	}

	bool CheckHash( const HashCheckArguments& query )
	{
		{
			std::lock_guard scope( s_modifiedOutputsCS );
			if( s_modifiedOutputs.find( query.outputPath ) != s_modifiedOutputs.end() )
			{
				return true;
			}
		}

		auto in = ::GetSourceHash( query.sourcePath.c_str(), query.defines );

		if( !IsOutputUpToDate( query.outputPath.c_str(), in ) )
		{
			std::lock_guard scope( s_modifiedOutputsCS );
			s_modifiedOutputs.insert( query.outputPath );
		}

		return true;
	}

	void GetSourceHash( const char* sourcePath, const char* parentData, const char* rootPath, std::set<std::string>& visited, MD5& md5 )
	{
		if( visited.find( sourcePath ) != end( visited ) )
		{
			return;
		}
		visited.insert( sourcePath );
		if( auto opened = s_includeHandler.Open( sourcePath, parentData, rootPath ) )
		{
			md5.add( opened->data, opened->size );

			std::cmatch match;
			auto begin = opened->data;
			while( std::regex_search( begin, opened->data + opened->size, match, s_include ) )
			{
				GetSourceHash( match[1].str().c_str(), opened->data, rootPath, visited, md5 );
				begin += match.position() + match.length();
			}
		}
	}

}

extern unsigned g_optimizationLevel;
extern bool g_avoidFlowControl;

std::string GetSourceHash( const char* sourcePath, const std::vector<Macro>& defines )
{
	MD5 md5;
	std::set<std::string> visited;

	// Include defines from command line and relevant compiler settings into the hash

	for( auto& each : defines )
	{
		md5.add( each.name.c_str(), each.name.length() );
		md5.add( each.value.c_str(), each.value.length() );
	}
	
	md5.add( &g_optimizationLevel, sizeof( g_optimizationLevel ) );
	md5.add( &g_avoidFlowControl, sizeof( g_avoidFlowControl ) );
	
	GetSourceHash( sourcePath, nullptr, sourcePath, visited, md5 );
	return md5.getHash();
}

void PrintOutOfDateFiles( size_t workerCount )
{
	WorkQueue<HashCheckArguments, decltype( &CheckHash )> workQueue( workerCount, &CheckHash );

	char buffer[4096];
	while( !feof( stdin ) )
	{
		if( !fgets( buffer, sizeof(buffer), stdin ) )
		{
			break;
		}

		HashCheckArguments query;

		char* line = buffer;
		auto next = GetNextWord( line );
		query.sourcePath = line;
		line = next;
		next = GetNextWord( line );
		query.outputPath = line;
		line = next;

		while( *line )
		{
			Macro macro;
			next = GetNextWord( line );
			if( *next == 0 )
			{
				printf( "Invalid input string near %s\n", line );
				fflush( stdout );
				return;
			}
			macro.name = line;
			line = next;
			next = GetNextWord( line );
			macro.value = line;
			query.defines.push_back( macro );
			line = next;
		}

		workQueue.Put( query );
	}

	workQueue.Join();

	for( auto it = s_modifiedOutputs.begin(); it != s_modifiedOutputs.end(); ++it )
	{
		puts( it->c_str() );
		puts( "\n" );
	}
}
