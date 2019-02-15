#include "stdafx.h"
#include "ModifiedTime.h"
#include "Mutex.h"
#include "WorkQueue.h"
#include "Macro.h"
#include "CachingIncludeHandler.h"


namespace
{
	CachingIncludeHandler s_includeHandler;

	// --------------------------------------------------------------------------------------
	// Description:
	//   A custom include handler for getting last modification time for effect and all its
	//   dependencies.
	// --------------------------------------------------------------------------------------
	class MTimeIncludeHandler : public ID3DXInclude
	{
	public:
		MTimeIncludeHandler()
		{
			m_lastMTime.dwHighDateTime = 0;
			m_lastMTime.dwLowDateTime = 0;
		}

		HRESULT __stdcall Open( D3DXINCLUDE_TYPE includeType,
			LPCSTR fileName,
			LPCVOID parentData,
			LPCVOID *data,
			UINT *bytes )
		{
			FILETIME t;
			HRESULT hr = s_includeHandler.Open(
				includeType,
				fileName,
				parentData,
				data,
				bytes,
				m_rootPath.c_str(),
				t );
			if( SUCCEEDED( hr ) )
			{
				if( m_lastMTime.dwHighDateTime < t.dwHighDateTime ||
					m_lastMTime.dwHighDateTime == t.dwHighDateTime &&
					m_lastMTime.dwLowDateTime < t.dwLowDateTime )
				{
					m_lastMTime = t;
					m_newestFile = fileName;
				}
			}
			return hr;
		}

		HRESULT __stdcall Close( LPCVOID data )
		{
			return s_includeHandler.Close( data );
		}

		const FILETIME& GetLastModifiedTime()
		{
			return m_lastMTime;
		}

		void SetRootPath( const char* rootPath )
		{
			const char* filename = PathFindFileName( rootPath );
			m_rootPath = std::string( rootPath, filename - rootPath );
		}
	private:
		// Last modification time
		FILETIME m_lastMTime;
		// Path to original effect (for resolving relative paths)
		std::string m_rootPath;
		// Path to the newest file for debugging
		std::string m_newestFile;
	};

	bool FileTimeLess( const FILETIME& t0, const FILETIME& t1 )
	{
		return t0.dwHighDateTime < t1.dwHighDateTime || t0.dwHighDateTime == t1.dwHighDateTime && t0.dwLowDateTime < t1.dwLowDateTime;
	}

	bool GetPathTime( const char* path, FILETIME& time )
	{
		HANDLE file = CreateFile(
			path,
			GENERIC_READ,
			FILE_SHARE_READ | FILE_SHARE_WRITE,
			nullptr,
			OPEN_EXISTING,
			0,
			nullptr );
		if( file == INVALID_HANDLE_VALUE )
		{
			return false;
		}
		if( !GetFileTime( file, nullptr, nullptr, &time ) )
		{
			CloseHandle( file );
			return false;
		}
		CloseHandle( file );
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


	Mutex s_modifiedOutputsCS;
	std::set<std::string> s_modifiedOutputs;


	bool CheckMTime( const MTimeArguments& query )
	{
		{
			MutexScope scope( s_modifiedOutputsCS );
			if( s_modifiedOutputs.find( query.outputPath ) != s_modifiedOutputs.end() )
			{
				return true;
			}
		}


		const char* shaderSource;
		UINT shaderLength;
		MTimeIncludeHandler includeHandler;

		FILETIME outputTime;
		if( !GetPathTime( query.outputPath.c_str(), outputTime ) )
		{
			MutexScope scope( s_modifiedOutputsCS );
			s_modifiedOutputs.insert( query.outputPath );
		}
		if( FAILED( includeHandler.Open( D3DXINC_LOCAL, query.sourcePath.c_str(), NULL, (LPCVOID*)&shaderSource, &shaderLength ) ) )
		{
			MutexScope scope( s_modifiedOutputsCS );
			s_modifiedOutputs.insert( query.outputPath );
			return true;
		}
		if( FileTimeLess( outputTime, includeHandler.GetLastModifiedTime() ) )
		{
			MutexScope scope( s_modifiedOutputsCS );
			s_modifiedOutputs.insert( query.outputPath );
			return true;
		}

		includeHandler.SetRootPath( query.sourcePath.c_str() );

		D3DXMACRO defines[256];
		Macro::FillDxMacros( defines, query.defines.begin(), query.defines.end() );
		CComPtr<ID3DXBuffer> buffer;

		// Preprocess the file using MS preprocessor
		if( FAILED( D3DXPreprocessShader( shaderSource, shaderLength, defines, &includeHandler, &buffer, nullptr ) ) )
		{
			MutexScope scope( s_modifiedOutputsCS );
			s_modifiedOutputs.insert( query.outputPath );
		}
		else if( FileTimeLess( outputTime, includeHandler.GetLastModifiedTime() ) )
		{
			MutexScope scope( s_modifiedOutputsCS );
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
		if( !gets_s( buffer ) )
		{
			break;
		}
		size_t length = strlen( buffer ) + 1;
		char* inputLine = new char[length];
		strcpy_s( inputLine, length, buffer );

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
