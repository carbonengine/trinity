#include "stdafx.h"
#include "ModifiedTime.h"
#include "WorkQueue.h"
#include "Macro.h"
#include "CachingIncludeHandler.h"

#if !_WIN32
#include <libgen.h>
#include <sys/param.h>
#include <sys/stat.h>
#endif


namespace
{
	CachingIncludeHandler s_includeHandler;

	bool GetPathTime( const char* path, time_t& time )
	{
		struct stat buf;
		if( stat( path, &buf ) == 0 )
		{
			time = buf.st_mtime;
		}
		else
		{
			return false;
		}
		return true;
	}

	char* GetNextWord( char* string )
	{

		char* end = string;
		while( *end && *end != 32 )
		{
			++end;
		}
		if( *end )
		{
			*end = 0;
			return end + 1;
		}
		return end;
	}


	struct MTimeArguments
	{
		std::string sourcePath;
		std::string outputPath;
		std::vector<Macro> defines;
	};


	std::mutex s_modifiedOutputsCS;
	std::set<std::string> s_modifiedOutputs;

	const std::regex s_include( "#[[:space:]]*include[[:space:]]*[<\"]([^>\"]*)" );

	time_t GetSourceMTime( const char* sourcePath, const char* parentData, const char* rootPath, std::set<std::string>& visited )
	{
		if( visited.find( sourcePath ) != end( visited ) )
		{
			return 0;
		}
		visited.insert( sourcePath );
		time_t result = 0;
		if( auto opened = s_includeHandler.Open( sourcePath, parentData, rootPath ) )
		{
			result = opened->modifiedTime;
			std::cmatch match;
			auto begin = opened->data;
			while( std::regex_search( begin, opened->data + opened->size, match, s_include ) )
			{
				result = std::max( result, GetSourceMTime( match[1].str().c_str(), opened->data, rootPath, visited ) );
				begin += match.position() + match.length();
			}
		}
		return result;
	}

	bool CheckMTime( const MTimeArguments& query )
	{
		{
			std::lock_guard scope( s_modifiedOutputsCS );
			if( s_modifiedOutputs.find( query.outputPath ) != s_modifiedOutputs.end() )
			{
				return true;
			}
		}

		time_t outputTime;
		if( !GetPathTime( query.outputPath.c_str(), outputTime ) )
		{
			std::lock_guard scope( s_modifiedOutputsCS );
			s_modifiedOutputs.insert( query.outputPath );
			return true;
		}

		std::set<std::string> visited;
		time_t inputTime = GetSourceMTime( query.sourcePath.c_str(), nullptr, query.sourcePath.c_str(), visited );

		if( outputTime < inputTime )
		{
			std::lock_guard scope( s_modifiedOutputsCS );
			s_modifiedOutputs.insert( query.outputPath );
		}

		return true;
	}
}

void PrintModificationTime( size_t workerCount )
{
	WorkQueue<MTimeArguments, decltype( &CheckMTime )> workQueue( workerCount, &CheckMTime );

	char buffer[4096];
	while( !feof( stdin ) )
	{
		if( !fgets( buffer, sizeof(buffer), stdin ) )
		{
			break;
		}
		// size_t length = strlen( buffer ) + 1;
		// char* inputLine = new char[length];
		// strcpy_s( inputLine, length, buffer );

		MTimeArguments query;

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
