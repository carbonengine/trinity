////////////////////////////////////////////////////////////
//
//    Created:   November 2011
//    Copyright: CCP 2011
//

#include "stdafx.h"
#include "CachingIncludeHandler.h"
#include "CompileMessageQueue.h"
#include "StringTable.h"
#include "EffectCompilerDx9.h"
#include "EffectCompilerDX11.h"
#include "EffectCompilerGL2.h"
#include "EffectCompilerOrbis.h"
#include "EffectData.h"


typedef BOOL (WINAPI *LPFN_GLPI)(
    PSYSTEM_LOGICAL_PROCESSOR_INFORMATION, 
    PDWORD);

// --------------------------------------------------------------------------------------
// Description:
//   Get number of physical cores on machine.
// Return value:
//   number of physical cores on machine
// --------------------------------------------------------------------------------------
unsigned GetNumberOfCores()
{
    BOOL done;
    BOOL rc;
    DWORD returnLength;
    DWORD procCoreCount;
    DWORD byteOffset;
    PSYSTEM_LOGICAL_PROCESSOR_INFORMATION buffer;
    LPFN_GLPI Glpi;

    Glpi = (LPFN_GLPI) GetProcAddress( GetModuleHandle( TEXT( "kernel32" ) ), "GetLogicalProcessorInformation" );
    if( NULL == Glpi ) 
    {
        return 1;
    }

    done = FALSE;
    buffer = NULL;
    returnLength = 0;

    while( !done ) 
    {
        rc = Glpi( buffer, &returnLength );

        if( FALSE == rc ) 
        {
            if( GetLastError() == ERROR_INSUFFICIENT_BUFFER ) 
            {
                if( buffer ) 
				{
                    free( buffer );
				}
                buffer=(PSYSTEM_LOGICAL_PROCESSOR_INFORMATION)malloc( returnLength );

                if( NULL == buffer ) 
                {
                    return 1;
                }
            } 
            else 
            {
                return 1;
            }
        } 
        else 
		{
			done = TRUE;
		}
    }

    procCoreCount = 0;
    byteOffset = 0;

    while( byteOffset < returnLength ) 
    {
        if( buffer->Relationship == RelationProcessorCore ) 
        {
            procCoreCount++;
        }
        byteOffset += sizeof( SYSTEM_LOGICAL_PROCESSOR_INFORMATION );
        buffer++;
    }

    return procCoreCount;
}

// --------------------------------------------------------------------------------------
// Description:
//   Maintains a queue of situation permutations to be compiled. Can be accessed from
//   different threads.
// --------------------------------------------------------------------------------------
class WorkQueue
{
public:
	WorkQueue()
		:m_head( 0 )
	{
		InitializeCriticalSectionAndSpinCount( &m_cs, 1000 );
		m_added = CreateSemaphore( NULL, 0, LONG_MAX, NULL );
	}

	~WorkQueue()
	{
		DeleteCriticalSection( &m_cs );
		for( auto it = m_queue.begin(); it != m_queue.end(); ++it )
		{
			delete []*it;
		}
	}

	bool Empty()
	{
		EnterCriticalSection( &m_cs );
		bool isEmpty = m_queue.size() <= m_head;
		LeaveCriticalSection( &m_cs );
		return isEmpty;
	}

	char* Get()
	{
		for( ;; )
		{
			EnterCriticalSection( &m_cs );
			if( m_queue.size() > m_head )
			{
				char* item = m_queue[m_head++];
				LeaveCriticalSection( &m_cs );
				return item;
			}
			LeaveCriticalSection( &m_cs );

			WaitForSingleObject( m_added, INFINITE );
		}
	}

	void Put( char* item )
	{
		EnterCriticalSection( &m_cs );
		m_queue.push_back( item );
		ReleaseSemaphore( m_added, 1, NULL );
		LeaveCriticalSection( &m_cs );
	}
private:
	// Queue of situation permutations
	std::vector<char*> m_queue;
	// Head element index
	unsigned m_head;
	// Critical section for shared data
	CRITICAL_SECTION m_cs;
	// Semaphore for blocking Get
	HANDLE m_added;
};

// Maximum length of defines string
const unsigned BUFFER_SIZE = 4096;

// Queue of situation permutations to be compiled
WorkQueue g_workQueue;
// Flag indicating a compile error (in some worker thread) causes all worker threads to exit
LONG g_error = 0;
// Include file handler
CachingIncludeHandler g_includeHandler;
// Compiled effect code (indexed by permutaitons IDs)
std::map<unsigned, ID3DXBuffer*> g_compiledEffects;
// Critical section for g_compiledEffects
CRITICAL_SECTION g_compiledEffectsCS;
// Permutation aliases (permutation ID -> permutation ID)
std::map<unsigned, unsigned> g_aliases;
// Critical section for permutation aliases
CRITICAL_SECTION g_aliasesCS;
// Queue of compiler output messages
CompileMessageQueue g_messages;
// Entry point shader file contents
char* g_shaderSource = NULL;
// Entry point shader file length
unsigned g_shaderLength = 0;
// Print warning messages?
bool g_printWarnings = true;

std::string g_glExternalCompilerSwitch;
std::string g_glExternalCompilerPath;


// Generate DX11 HLSL listing file
bool g_generateListing = false;
// Listing text
std::string g_listing;
// CS for listing access
CRITICAL_SECTION g_listingCS;

// Optimization level
unsigned g_optimizationLevel = 3;

// Print compiled shader statistics
bool g_printShaderStats = false;

// Number of clip planes for DX11 patched vertex shaders
int g_maxClipPlanes = 1;

// Emulate sampler states not supported by GLES (namely border mode)
bool g_glesEmulateSampler = false;
// Avoid flow control in compiled shaders (especially useful for GLES)
bool g_avoidFlowControl = false;
// Allowed GLES extensions
GlesExtensionInfo g_glesExtensions;

StringTable g_stringTable;

EffectCompilerDX9 g_compilerDX9;
EffectCompilerDX11 g_compilerDX11;
EffectCompilerGL2 g_compilerGL2;
EffectCompilerOrbis g_compilerOrbis;


enum Platform
{
	PLATFORM_DX9 = 1,
	PLATFORM_DX11 = 2,
	PLATFORM_GL2 = 3,
	PLATFORM_ORBIS = 4,

	_PLATFORM_END,
	_PLATFORM_BEGIN = 1,
};

// --------------------------------------------------------------------------------------
// Description:
//   Get next word (space-delimed) from an input string.
// Arguments:
//   string - a string containing space-delimed words
// Return value:
//   pointer to the next character after space in an input string (or to the trailing 0)
// --------------------------------------------------------------------------------------
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

// --------------------------------------------------------------------------------------
// Description:
//   Utility function for comparing file times.
// Arguments:
//   t0 - File time
//   t1 - Another file time
// Return value:
//   true If t0 is less (before) t1
//   false Otherwise
// --------------------------------------------------------------------------------------
static bool FileTimeLess( const FILETIME& t0, const FILETIME& t1 )
{
	return t0.dwHighDateTime < t1.dwHighDateTime ||
		t0.dwHighDateTime == t1.dwHighDateTime && t0.dwLowDateTime < t1.dwLowDateTime;
}

// --------------------------------------------------------------------------------------
// Description:
//   Utility function for parsing input string containing permutation info. The string
//   needs to be in a format: permutation alias [defineName1 defineValue1 [...]]
//   If alias part of the line is not equal to -1, the rest of the string is ignored.
// Arguments:
//   line - Input line
//   alias - (out) Alias part of the line
//   permutation - (out) Permutation part of the line
//   platform - (out) Platform ID (from PLATFORM define)
//   defines - (out) Array of macro defines
// Return value:
//   true If line was parsed successfully
//   false Otherwise
// --------------------------------------------------------------------------------------
static bool ParseLine( char* line, unsigned& alias, unsigned& permutation, Platform& platform, D3DXMACRO* defines )
{
	// Parse the input line. The format is: permutation alias name0 definition0 name1 definition1...
	// Where permutation - is shader permute index
	// alias - if unsigned(-1) then shader needs compiling, otherwise permutation is should be 
	// 
	char* permutationString = line;
	line = GetNextWord( line );
	char* aliasString = line;
	line = GetNextWord( line );
	permutation = atol( permutationString );
	alias = atol( aliasString );

	if( alias != -1 )
	{
		return true;
	}

	unsigned defineCount = 0;
	while( *line )
	{
		defines[defineCount].Name = line;
		line = GetNextWord( line );
		if( *line == 0 )
		{
			printf( "Invalid input string near %s\n", defines[defineCount].Name );
			fflush( stdout );
			return false;
		}
		defines[defineCount].Definition = line;
		if( strcmp( defines[defineCount].Name, "PLATFORM" ) == 0 )
		{
			int p = atoi( defines[defineCount].Definition );
			if( p >= _PLATFORM_BEGIN && p < _PLATFORM_END )
			{
				platform = Platform( p );
			}
			else
			{
				printf( "Unrecognized platform define \"%s\". Reverting to DX9\n", defines[defineCount].Definition );
				fflush( stdout );
				platform = PLATFORM_DX9;
			}
		}
		line = GetNextWord( line );
		defineCount++;
	}
	defines[defineCount].Name = defines[defineCount].Definition = NULL;
	return true;
}

// --------------------------------------------------------------------------------------
// Description:
//   Function returns file modification time.
// Arguments:
//   path - Path to existing file
//   time - (out) File modification time
// Return value:
//   true If modification time was retrieved successfully
//   false Otherwise
// --------------------------------------------------------------------------------------
static bool GetPathTime( const char* path, FILETIME& time )
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

// --------------------------------------------------------------------------------------
// Description:
//   A custom include handler for getting last modification time for effect and all its
//   dependencies.
// --------------------------------------------------------------------------------------
class MTimeIncludeHandler: public ID3DXInclude
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
		HRESULT hr = g_includeHandler.Open( 
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
		return g_includeHandler.Close( data );
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

CRITICAL_SECTION s_modifiedOutputsCS;
std::set<std::string> s_modifiedOutputs;

// --------------------------------------------------------------------------------------
// Description:
//   Worker thread for determining which output files are out of date. The result of a
//   function is stored in s_modifiedOutputs as a list of output files that are out of 
//   date.
// Arguments:
//   param - Unused
// Return value:
//   0 always
// --------------------------------------------------------------------------------------
DWORD WINAPI CheckMTimeThread( LPVOID param )
{
	D3DXMACRO defines[BUFFER_SIZE];
	for(;;)
	{
		char* inputLine = g_workQueue.Get();
		char* line = inputLine;
		if( line == nullptr )
		{
			// End of input marker
			return 0;
		}
		size_t lineLength = strlen( line );
		char* shaderPath = line;
		line = GetNextWord( line );
		char* outputPath = line;
		line = GetNextWord( line );

		EnterCriticalSection( &s_modifiedOutputsCS );
		if( s_modifiedOutputs.find( outputPath ) != s_modifiedOutputs.end() )
		{
			LeaveCriticalSection( &s_modifiedOutputsCS );
			continue;
		}
		LeaveCriticalSection( &s_modifiedOutputsCS );

		Platform platform;
		unsigned alias, permutation;
		if( !ParseLine( line, alias, permutation, platform, defines ) )
		{
			InterlockedExchange( &g_error, 1 );
			return 1;
		}

		if( alias != -1 )
		{
			continue;
		}

		CComPtr<ID3DXBuffer> buffer;
		CComPtr<ID3DXBuffer> errors;

		const char* shaderSource;
		UINT shaderLength;
		MTimeIncludeHandler includeHandler;

		FILETIME outputTime;
		if( !GetPathTime( outputPath, outputTime ) )
		{
			EnterCriticalSection( &s_modifiedOutputsCS );
			s_modifiedOutputs.insert( outputPath );
			LeaveCriticalSection( &s_modifiedOutputsCS );
		}
		if( FAILED( includeHandler.Open( D3DXINC_LOCAL, shaderPath, NULL, (LPCVOID*)&shaderSource, &shaderLength ) ) )
		{
			EnterCriticalSection( &s_modifiedOutputsCS );
			s_modifiedOutputs.insert( outputPath );
			LeaveCriticalSection( &s_modifiedOutputsCS );
			continue;
		}
		if( FileTimeLess( outputTime, includeHandler.GetLastModifiedTime() ) )
		{
			EnterCriticalSection( &s_modifiedOutputsCS );
			s_modifiedOutputs.insert( outputPath );
			LeaveCriticalSection( &s_modifiedOutputsCS );
			continue;
		}

		includeHandler.SetRootPath( shaderPath );

		// Preprocess the file using MS preprocessor
		if( FAILED( D3DXPreprocessShader( shaderSource, shaderLength, defines, &includeHandler, &buffer, &errors ) ) )
		{
			EnterCriticalSection( &s_modifiedOutputsCS );
			s_modifiedOutputs.insert( outputPath );
			LeaveCriticalSection( &s_modifiedOutputsCS );
		}
		else if( FileTimeLess( outputTime, includeHandler.GetLastModifiedTime() ) )
		{
			EnterCriticalSection( &s_modifiedOutputsCS );
			s_modifiedOutputs.insert( outputPath );
			LeaveCriticalSection( &s_modifiedOutputsCS );
		}
	}
	return 0;
}

// --------------------------------------------------------------------------------------
// Description:
//   Worker thread for compiling shader permutations. Fetches next permutation from input
//   queue, compiles it and puts result into g_compiledEffects.
// Arguments:
//   param - unused
// Return value:
//   0 always
// --------------------------------------------------------------------------------------
DWORD WINAPI WorkerThread( LPVOID param )
{
	D3DXMACRO defines[BUFFER_SIZE];
	for(;;)
	{
		if( InterlockedCompareExchange( &g_error, 0, 0 ) )
		{
			return 0;
		}
		char* line = g_workQueue.Get();
		if( line == nullptr )
		{
			// End of input marker
			return 0;
		}
		Platform platform;
		unsigned alias, permutation;

		if( !ParseLine( line, alias, permutation, platform, defines ) )
		{
			InterlockedExchange( &g_error, 1 );
			return 1;
		}
		if( alias != -1 )
		{
			EnterCriticalSection( &g_aliasesCS );
			g_aliases[alias] = permutation;
			LeaveCriticalSection( &g_aliasesCS );
			continue;
		}

		EnterCriticalSection( &g_compiledEffectsCS );
		if( g_compiledEffects.find( permutation ) != g_compiledEffects.end() )
		{
			LeaveCriticalSection( &g_compiledEffectsCS );
			continue;
		}
		LeaveCriticalSection( &g_compiledEffectsCS );

		EffectData outputData;

		switch( platform )
		{
		case PLATFORM_DX9:
			if( !g_compilerDX9.CompileEffect( g_shaderSource, g_shaderLength, defines, &g_includeHandler, outputData ) )
			{
				InterlockedExchange( &g_error, 1 );
				return 1;
			}
			break;
		case PLATFORM_DX11:
			if( !g_compilerDX11.CompileEffect( g_shaderSource, g_shaderLength, defines, &g_includeHandler, outputData ) )
			{
				InterlockedExchange( &g_error, 1 );
				return 1;
			}
			break;
		case PLATFORM_GL2:
			if( !g_compilerGL2.CompileEffect( g_shaderSource, g_shaderLength, defines, &g_includeHandler, outputData ) )
			{
				InterlockedExchange( &g_error, 1 );
				return 1;
			}
			break;
		case PLATFORM_ORBIS:
			if( !g_compilerOrbis.CompileEffect( g_shaderSource, g_shaderLength, defines, &g_includeHandler, outputData ) )
			{
				InterlockedExchange( &g_error, 1 );
				return 1;
			}
			break;
		}


		CComPtr<ID3DXBuffer> outputBuffer;
		if( !SaveEffectData( outputData, &outputBuffer ) )
		{
			g_messages.AddMessage( "\\memory(0): error X000: Error packing effect data" );
			InterlockedExchange( &g_error, 1 );
			return 1;
		}

		EnterCriticalSection( &g_compiledEffectsCS );
		g_compiledEffects[permutation] = outputBuffer.Detach();
		LeaveCriticalSection( &g_compiledEffectsCS );
	}
	return 0;
}

void PrintUsage()
{
	printf( "ShaderCompiler: compile new-style Tr2 shaders\n" );
	printf( "Syntax: ShaderCompiler [<options>] input_file output_file\n" );
	printf( "Options:\n" );
	printf( "  /no_warnings - Do not output compile warnings\n" );
	printf( "  /threads <thread_count> - Limit worker thread count to <thread_count>\n" );
	printf( "  /single - Compile single permutation only\n" );
	printf( "  /define <name> <value> - Add define (only valid with /single)\n" );
	printf( "  /listing <listing_file> - Print DX11 HLSL output in a file\n" );
	printf( "  /clipPlanes <number> - Number of clip planes for DX11 patched VS\n" );
	printf( "  /O{0,1,2,3} - Set optimization level. 3 is default\n" );
	printf( "  /shaderStats - Print compiled shader statistics\n" );
	printf( "  /mtime - Determine which compiled files are out of date\n" );
	printf( "  /GS - Emulate sampler states in GLSL (namely border wrap mode)\n" );
	printf( "  /Gfa - Avoid flow control constructs\n" );
	printf( "  /Gc <switch> <path> - Compile GLSL to binary shader using external compiler\n" );
	printf( "  /E{e,w,d}[extension] - Specify support for all or certain GLES extensions\n" );
	printf( "input_file - Path to input .fx file\n" );
	printf( "output_file - Path to output .fxp file\n" );
}

// --------------------------------------------------------------------------------------
// Description:
//   Shader compiler entry point.
// Arguments:
//   argc - number of program arguments
//   argv - array of program arguments
// Return value:
//   0 on success
//   non-zero on error
// --------------------------------------------------------------------------------------
int _tmain(int argc, _TCHAR* argv[])
{
	char* shaderPath = nullptr;
	char* outputPath = nullptr;
	unsigned coreCount = GetNumberOfCores() * 2;

	bool singlePermutation = false;
	char singleDefines[BUFFER_SIZE];
	char* singleDefineStart = singleDefines;
	strcpy_s( singleDefines, "0 -1 " );
	singleDefineStart += strlen( singleDefineStart );

	char* listingFile = nullptr;

	// The /mtime option
	bool checkMTime = false;

	g_glesExtensions.m_all = GlesExtensionInfo::WARN;

	// Parse command line
	for( int i = 1; i < argc; ++i )
	{
		if( strcmp(argv[i], "/threads" ) == 0 )
		{
			++i;
			if( i < argc )
			{
				coreCount = max( atoi( argv[i] ), 1 );
			}
			else
			{
				PrintUsage();
				return 1;
			}
		}
		else if( strcmp(argv[i], "/no_warnings" ) == 0 )
		{
			g_printWarnings = false;
		}
		else if( strcmp(argv[i], "/shaderStats" ) == 0 )
		{
			g_printShaderStats = true;
		}
		else if( strcmp(argv[i], "/single" ) == 0 )
		{
			singlePermutation = true;
		}
		else if( strcmp(argv[i], "/define" ) == 0 )
		{
			++i;
			if( i < argc )
			{
				strcpy_s( singleDefineStart, BUFFER_SIZE - ( singleDefineStart - singleDefines ), argv[i] );
				size_t len = strlen( argv[i] );
				singleDefineStart[len] = ' ';
				singleDefineStart += len + 1;
			}
			else
			{
				PrintUsage();
				return 1;
			}
			++i;
			if( i < argc )
			{
				strcpy_s( singleDefineStart, BUFFER_SIZE - ( singleDefineStart - singleDefines ), argv[i] );
				size_t len = strlen( argv[i] );
				singleDefineStart[len] = ' ';
				singleDefineStart += len + 1;
			}
			else
			{
				PrintUsage();
				return 1;
			}
		}
		else if( strcmp(argv[i], "/listing" ) == 0 )
		{
			g_generateListing = true;
			++i;
			if( i < argc )
			{
				listingFile = argv[i];
			}
			else
			{
				PrintUsage();
				return 1;
			}
		}
		else if( strcmp( argv[i], "/clipPlanes" ) == 0 )
		{
			++i;
			if( i < argc )
			{
				g_maxClipPlanes = min( atoi( argv[i] ), 4 );
			}
			else
			{
				PrintUsage();
				return 1;
			}
		}
		else if( strncmp( argv[i], "/O", 2 ) == 0 && strlen( argv[i] ) == 3 && argv[i][2] >= '0' && argv[i][2] <= '3' )
		{
			g_optimizationLevel = argv[i][2] - '0';
		}
		else if( strcmp( argv[i], "/mtime" ) == 0 )
		{
			checkMTime = true;
		}
		else if( strcmp( argv[i], "/GS" ) == 0 )
		{
			g_glesEmulateSampler = true;
		}
		else if( strcmp( argv[i], "/Gfa" ) == 0 )
		{
			g_avoidFlowControl = true;
		}
		else if( argv[i][0] == '/' && argv[i][1] == 'E' &&
			( argv[i][2] == 'e' || argv[i][2] == 'w' || argv[i][2] == 'd' ) )
		{
			GlesExtensionInfo::Support support;
			switch( argv[i][2] )
			{
			case 'e':
				support = GlesExtensionInfo::ENABLE;
				break;
			case 'd':
				support = GlesExtensionInfo::DISABLE;
				break;
			default:
				support = GlesExtensionInfo::WARN;
				break;
			}
			if( argv[i][3] )
			{
				g_glesExtensions.m_extensions[argv[i] + 3] = support;
			}
			else
			{
				g_glesExtensions.m_all = support;
			}
		}
		else if( strcmp( argv[i], "/Gc" ) == 0 )
		{
			g_glExternalCompilerSwitch = argv[++i];
			g_glExternalCompilerPath = argv[++i];
		}
		else if( shaderPath == nullptr )
		{
			shaderPath = argv[i];
		}
		else if( outputPath == nullptr )
		{
			outputPath = argv[i];
		}
		else
		{
			PrintUsage();
			return 1;
		}
	}

	if( outputPath == nullptr && ( !checkMTime || singlePermutation ) )
	{
		PrintUsage();
		return 1;
	}

	if( checkMTime )
	{
		InitializeCriticalSection( &s_modifiedOutputsCS );

		HANDLE *workerThreads = new HANDLE[coreCount];
		for( unsigned i = 0; i < coreCount; ++i )
		{
			workerThreads[i] = CreateThread( NULL, 0, &CheckMTimeThread, NULL, 0, NULL );
		}

		if( singlePermutation )
		{
			*singleDefineStart = 0;
			size_t length = strlen( singleDefines ) + 1;
			length += strlen( shaderPath ) + strlen( outputPath ) + 2;
			char* inputLine = new char[length];
			strcpy_s( inputLine, length, shaderPath );
			strcat_s( inputLine, length, " " );
			strcat_s( inputLine, length, outputPath );
			strcat_s( inputLine, length, " " );
			strcat_s( inputLine, length, singleDefines );
			g_workQueue.Put( inputLine );
		}
		else
		{
			char buffer[BUFFER_SIZE];
			while( !feof( stdin ) )
			{
				if( !gets_s( buffer ) )
				{
					break;
				}
				size_t length = strlen( buffer ) + 1;
				char* inputLine = new char[length];
				strcpy_s( inputLine, length, buffer );
				g_workQueue.Put( inputLine );
			}
		}
		for( unsigned i = 0; i < coreCount; ++i )
		{
			g_workQueue.Put( nullptr );
		}
		WaitForMultipleObjects( coreCount, workerThreads, TRUE, INFINITE );

		DeleteCriticalSection( &s_modifiedOutputsCS );

		for( auto it = s_modifiedOutputs.begin(); it != s_modifiedOutputs.end(); ++it )
		{
			puts( it->c_str() );
			puts( "\n" );
		}
		return 0;
	}

	if( g_generateListing )
	{
		InitializeCriticalSection( &g_listingCS );
	}

	// Preload shader file
	if( FAILED( g_includeHandler.Open( D3DXINC_LOCAL, shaderPath, NULL, (LPCVOID*)&g_shaderSource, &g_shaderLength ) ) )
	{
		printf( "%s: error X0000: Could not open input file \"%s\"\n", shaderPath, shaderPath );
		return 1;
	}
	
	g_includeHandler.SetRootPath( shaderPath );
	g_messages.SetEntryFileName( shaderPath );

	// Initialize mutexes and threads
	InitializeCriticalSectionAndSpinCount( &g_compiledEffectsCS, 1000 );
	InitializeCriticalSectionAndSpinCount( &g_aliasesCS, 100 );

	HANDLE *workerThreads = new HANDLE[coreCount];
	for( unsigned i = 0; i < coreCount; ++i )
	{
		workerThreads[i] = CreateThread( NULL, 0, &WorkerThread, NULL, 0, NULL );
	}

	if( !g_compilerDX9.Create() )
	{
		return 1;
	}

	if( !g_compilerDX11.Create() )
	{
		return 1;
	}
	
	if( !g_compilerGL2.Create() )
	{
		return 1;
	}
	

	// Read permutation strings
	char buffer[BUFFER_SIZE];

	if( singlePermutation )
	{
		*singleDefineStart = 0;
		size_t length = strlen( singleDefines ) + 1;
		char* inputLine = new char[length];
		strcpy_s( inputLine, length, singleDefines );
		g_workQueue.Put( inputLine );
	}
	else
	{
		while( !feof( stdin ) )
		{
			if( !gets_s( buffer ) )
			{
				break;
			}
			size_t length = strlen( buffer ) + 1;
			char* inputLine = new char[length];
			strcpy_s( inputLine, length, buffer );
			g_workQueue.Put( inputLine );
		}
	}

	for( unsigned i = 0; i < coreCount; ++i )
	{
		g_workQueue.Put( nullptr );
	}
	
	// Wait for worker threads to finish compiling
	WaitForMultipleObjects( coreCount, workerThreads, TRUE, INFINITE );
	if( InterlockedCompareExchange( &g_error, 0, 0 ) )
	{
		g_messages.Flush();
		return 1;
	}

	g_messages.Flush();

	// Check if permutations are valid
	for( auto it = g_aliases.begin(); it != g_aliases.end(); ++it )
	{
		if( g_compiledEffects.find( it->second ) == g_compiledEffects.end() )
		{
			printf( "%s: error X0000: Invalid alias from permutation %u to permutation %u\n", shaderPath, it->first, it->second );
			return 1;
		}
	}

	// Open the output file
	HANDLE file = CreateFile( outputPath, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL );
	if( file == INVALID_HANDLE_VALUE )
	{
		printf( "%s: error X0000: Could not open output file \"%s\" for writing\n", shaderPath, outputPath );
		fflush( stdout );
		return 1;
	}

	if( g_printWarnings )
	{
		printf( "Compiled %u permutations and %u aliases\n", g_compiledEffects.size(), g_aliases.size() );
	}

	// Write file header
	unsigned totalSize = unsigned( g_compiledEffects.size() + g_aliases.size() );
	unsigned headerSize = ( totalSize * 3 + 1 ) * sizeof( unsigned );
	DWORD *header = new DWORD[totalSize * 3 + 1];
	header[0] = totalSize;
	unsigned index = 1;
	unsigned offset = sizeof( DWORD ) + headerSize + g_stringTable.GetSize();

	std::map<unsigned, std::pair<unsigned, unsigned> > offsets;
	for( auto it = g_compiledEffects.begin(); it != g_compiledEffects.end(); ++it )
	{
		header[index++] = it->first;
		header[index++] = offset;
		header[index++] = it->second->GetBufferSize();
		offsets[it->first] = std::make_pair( offset, it->second->GetBufferSize() );
		offset += it->second->GetBufferSize();
	}
	for( auto it = g_aliases.begin(); it != g_aliases.end(); ++it )
	{
		header[index++] = it->first;
		header[index++] = offsets[it->second].first;
		header[index++] = offsets[it->second].second;
	}
	DWORD bytesWritten;
	DWORD version = 3;
	WriteFile( file, &version, sizeof( DWORD ), &bytesWritten, NULL );
	WriteFile( file, header, headerSize, &bytesWritten, NULL );
	g_stringTable.Write( file );

	// Write compiled code
	for( auto it = g_compiledEffects.begin(); it != g_compiledEffects.end(); ++it )
	{
		WriteFile( file, it->second->GetBufferPointer(), it->second->GetBufferSize(), &bytesWritten, NULL );
	}

	delete[] header;

	CloseHandle( file );

	DeleteCriticalSection( &g_compiledEffectsCS );
	DeleteCriticalSection( &g_aliasesCS );

	for( unsigned i = 0; i < coreCount; ++i )
	{
		CloseHandle( workerThreads[i] );
	}
	 
	if( g_generateListing )
	{
		HANDLE file = CreateFile( listingFile, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL );
		if( file == INVALID_HANDLE_VALUE )
		{
			printf( "%s: error X0000: Could not open listing file \"%s\" for writing\n", shaderPath, listingFile );
			fflush( stdout );
			return 1;
		}
		WriteFile( file, g_listing.c_str(), g_listing.length(), &bytesWritten, NULL );
		CloseHandle( file );
	}

	return 0;
}

