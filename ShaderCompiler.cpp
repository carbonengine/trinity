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
#include "EffectCompilerGL3.h"
#include "EffectCompilerGL4.h"
#include "EffectData.h"
#include "Mutex.h"
#include "WorkQueue.h"
#include "Macro.h"
#include "ModifiedTime.h"


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


// Maximum length of defines string
const unsigned BUFFER_SIZE = 4096;

// Flag indicating a compile error (in some worker thread) causes all worker threads to exit
LONG g_error = 0;
// Include file handler
CachingIncludeHandler g_includeHandler;
// Compiled effect code (indexed by permutaitons IDs)
std::map<unsigned, ID3DXBuffer*> g_compiledEffects;
// Critical section for g_compiledEffects
Mutex g_compiledEffectsCS( 1000 );
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
Mutex g_listingCS;

// Optimization level
unsigned g_optimizationLevel = 3;

// Number of clip planes for DX11 patched vertex shaders
int g_maxClipPlanes = 1;

// Emulate sampler states not supported by GLES (namely border mode)
bool g_glesEmulateSampler = false;
// Avoid flow control in compiled shaders (especially useful for GLES)
bool g_avoidFlowControl = false;
// Try validating converted OpenGL
bool g_validateOpenGL = true;

// Allowed GLES extensions
GlesExtensionInfo g_glesExtensions;

StringTable g_stringTable;

EffectCompilerDX9 g_compilerDX9;
EffectCompilerDX11 g_compilerDX11;
EffectCompilerGL2 g_compilerGL2;
EffectCompilerGL4 g_compilerGL4;
EffectCompilerGL3 g_compilerGL3;


enum Platform
{
	PLATFORM_DX9 = 1,
	PLATFORM_DX11 = 2,
	PLATFORM_GL2 = 3,
	PLATFORM_GL4 = 6,
	PLATFORM_GL3 = 7,

	_PLATFORM_END,
	_PLATFORM_BEGIN = 1,
};


Platform GetPlatform( const std::vector<Macro>& defines )
{
	for( auto it = begin( defines ); it != end( defines ); ++it )
	{
		if( it->name == "PLATFORM" )
		{
			int p = atoi( it->value.c_str() );
			if( p >= _PLATFORM_BEGIN && p < _PLATFORM_END )
			{
				return Platform( p );
			}
			else
			{
				printf( "Unrecognized platform define \"%s\". Reverting to DX9\n", it->value.c_str() );
				fflush( stdout );
				return PLATFORM_DX9;
			}
		}
	}
	return PLATFORM_DX9; 
}


struct CompileShaderArguments
{
	uint32_t permutation;
	std::vector<Macro> defines;
};

// --------------------------------------------------------------------------------------
// Description:
//   Worker thread for compiling shader permutations. Fetches next permutation from input
//   queue, compiles it and puts result into g_compiledEffects.
// Arguments:
//   param - unused
// Return value:
//   0 always
// --------------------------------------------------------------------------------------
bool CompileShader( const CompileShaderArguments& arguments )
{
	if( InterlockedCompareExchange( &g_error, 0, 0 ) )
	{
		return false;
	}

	D3DXMACRO defines[BUFFER_SIZE];
	Macro::FillDxMacros( defines, arguments.defines.begin(), arguments.defines.end() );
	Platform platform = GetPlatform( arguments.defines );

	{
		MutexScope scope( g_compiledEffectsCS );
		if( g_compiledEffects.find( arguments.permutation ) != g_compiledEffects.end() )
		{
			return true;
		}
	}

	EffectData outputData;

	switch( platform )
	{
	case PLATFORM_DX9:
		if( !g_compilerDX9.CompileEffect( g_shaderSource, g_shaderLength, defines, &g_includeHandler, outputData ) )
		{
			InterlockedExchange( &g_error, 1 );
			return false;
		}
		break;
	case PLATFORM_DX11:
		if( !g_compilerDX11.CompileEffect( g_shaderSource, g_shaderLength, defines, &g_includeHandler, outputData ) )
		{
			InterlockedExchange( &g_error, 1 );
			return false;
		}
		break;
	case PLATFORM_GL2:
		if( !g_compilerGL2.CompileEffect( g_shaderSource, g_shaderLength, defines, &g_includeHandler, outputData ) )
		{
			InterlockedExchange( &g_error, 1 );
			return false;
		}
		break;
	case PLATFORM_GL3:
		if( !g_compilerGL3.CompileEffect( g_shaderSource, g_shaderLength, defines, &g_includeHandler, outputData ) )
		{
			InterlockedExchange( &g_error, 1 );
			return false;
		}
		break;
	case PLATFORM_GL4:
		if( !g_compilerGL4.CompileEffect( g_shaderSource, g_shaderLength, defines, &g_includeHandler, outputData ) )
		{
			InterlockedExchange( &g_error, 1 );
			return false;
		}
		break;
	}


	CComPtr<ID3DXBuffer> outputBuffer;
	if( !SaveEffectData( outputData, &outputBuffer ) )
	{
		g_messages.AddMessage( "\\memory(0): error X000: Error packing effect data" );
		InterlockedExchange( &g_error, 1 );
		return false;
	}

	{
		MutexScope scope( g_compiledEffectsCS );
		g_compiledEffects[arguments.permutation] = outputBuffer.Detach();
	}

	return true;
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
	printf( "  /novalidate - Skip validating converted GLSL code\n" );
	printf( "  /permutations - Print permutations of the shader\n" );
	printf( "input_file - Path to input .fx file\n" );
	printf( "output_file - Path to output .fxp file\n" );
}

#include "ParserUtils.h"
#include "HLSLParser.h"
#include "ParserState.h"

bool DiscoverPermutations( Permutations& permutations, const char* shaderSource, size_t shaderLength )
{
	ParserState state( MakeInlineString( shaderSource, shaderSource + shaderLength ) );
	return state.DiscoverPermutations( permutations );
}


struct ProgramArguments
{
	ProgramArguments()
		:shaderPath( nullptr ),
		outputPath( nullptr ),
		coreCount( GetNumberOfCores() * 2 ),
		listingFile( nullptr ),
		checkMTime( false ),
		printPermutations( false ),
		ignorePermutations( false )
	{

	}

	char* shaderPath;
	char* outputPath;
	unsigned coreCount;
	std::vector<Macro> defines;
	char* listingFile;
	bool checkMTime;
	bool printPermutations;
	bool ignorePermutations;
};


bool ExtractCommandLineArguments( ProgramArguments& args, int argc, _TCHAR* argv[] )
{
	g_glesExtensions.m_all = GlesExtensionInfo::WARN;

	for( int i = 1; i < argc; ++i )
	{
		if( strcmp( argv[i], "/threads" ) == 0 )
		{
			++i;
			if( i < argc )
			{
				args.coreCount = max( atoi( argv[i] ), 1 );
			}
			else
			{
				return false;
			}
		}
		else if( strcmp( argv[i], "/no_warnings" ) == 0 )
		{
			g_printWarnings = false;
		}
		else if( strcmp( argv[i], "/shaderStats" ) == 0 )
		{
		}
		else if( strcmp( argv[i], "/single" ) == 0 )
		{
		}
		else if( strcmp( argv[i], "/novalidate" ) == 0 )
		{
			g_validateOpenGL = false;
		}
		else if( strcmp( argv[i], "/permutations" ) == 0 )
		{
			args.printPermutations = true;
		}
		else if( strcmp( argv[i], "/no_permutations" ) == 0 )
		{
			args.ignorePermutations = true;
		}
		else if( strcmp( argv[i], "/define" ) == 0 )
		{
			Macro define;
			++i;
			if( i < argc )
			{
				define.name = argv[i];
			}
			else
			{
				return false;
			}
			++i;
			if( i < argc )
			{
				define.value = argv[i];
			}
			else
			{
				return false;
			}
			args.defines.push_back( define );
		}
		else if( strcmp( argv[i], "/listing" ) == 0 )
		{
			g_generateListing = true;
			++i;
			if( i < argc )
			{
				args.listingFile = argv[i];
			}
			else
			{
				return false;
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
				return false;
			}
		}
		else if( strncmp( argv[i], "/O", 2 ) == 0 && strlen( argv[i] ) == 3 && argv[i][2] >= '0' && argv[i][2] <= '3' )
		{
			g_optimizationLevel = argv[i][2] - '0';
		}
		else if( strcmp( argv[i], "/mtime" ) == 0 )
		{
			args.checkMTime = true;
		}
		else if( strcmp( argv[i], "/GS" ) == 0 )
		{
			g_glesEmulateSampler = true;
		}
		else if( strcmp( argv[i], "/Gfa" ) == 0 )
		{
			g_avoidFlowControl = true;
		}
		else if( argv[i][0] == '/' && argv[i][1] == 'E' && ( argv[i][2] == 'e' || argv[i][2] == 'w' || argv[i][2] == 'd' ) )
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
		else if( args.shaderPath == nullptr )
		{
			args.shaderPath = argv[i];
		}
		else if( args.outputPath == nullptr )
		{
			args.outputPath = argv[i];
		}
		else
		{
			return false;
		}
	}
	if( args.checkMTime )
	{
		return true;
	}
	if( args.outputPath == nullptr && !args.printPermutations )
	{
		return false;
	}
	return true;
}

bool PrintPermutations( const char* shaderPath )
{
	char* shaderSource;
	unsigned shaderLength;

	if( FAILED( g_includeHandler.Open( D3DXINC_LOCAL, shaderPath, NULL, (LPCVOID*)&shaderSource, &shaderLength ) ) )
	{
		printf( "%s: error X0000: Could not open input file \"%s\"\n", shaderPath, shaderPath );
		return false;
	}
	g_includeHandler.SetRootPath( shaderPath );
	g_messages.SetEntryFileName( shaderPath );

	Permutations permutations;
	if( !DiscoverPermutations( permutations, shaderSource, shaderLength ) )
	{
		g_messages.Flush();
		return false;
	}

	for( auto it = permutations.begin(); it != permutations.end(); ++it )
	{
		printf( "%s:\n", it->name.c_str() );
		printf( "  options:\n" );
		for( auto jt = it->options.begin(); jt != it->options.end(); ++jt )
		{
			printf( "  - name: %s\n", jt->name.c_str() );
			printf( "    value: %i\n", jt->value );
		}
		printf( "  default: %s\n", it->defaultOption.c_str() );
		printf( "  description: %s\n", it->description.c_str() );
	}
	return true;
}

typedef WorkQueue<CompileShaderArguments, decltype( &CompileShader )> CompileQueue;

void AddPermutationsToWorkQueue( CompileQueue& queue, const Permutations& permutations, bool ignorePermutations, const std::vector<Macro>& defines )
{
	std::vector<size_t> indexes;
	indexes.resize( permutations.size() );
	unsigned permutation = 0;
	bool done = false;
	while( !done )
	{
		CompileShaderArguments args;
		args.permutation = uint32_t( permutation );
		if( !defines.empty() )
		{
			args.defines.insert( args.defines.end(), defines.begin(), defines.end() );
		}

		if( !ignorePermutations )
		{
			for( size_t i = 0; i < permutations.size(); ++i )
			{
				Macro define;
				define.name = permutations[i].name;
				define.value = std::to_string( int64_t( permutations[i].options[indexes[i]].value ) );
				args.defines.push_back( define );
			}
		}

		queue.Put( args );

		for( size_t i = 0; i < permutations.size(); ++i )
		{
			++indexes[i];
			if( indexes[i] >= permutations[i].options.size() )
			{
				if( i + 1 == permutations.size() )
				{
					done = true;
				}
				indexes[i] = 0;
			}
			else
			{
				break;
			}
		}
		if( ignorePermutations || permutations.empty() )
		{
			break;
		}
		++permutation;
	}
}


int _tmain(int argc, _TCHAR* argv[])
{
	ProgramArguments args;
	if( !ExtractCommandLineArguments( args, argc, argv ) )
	{
		PrintUsage();
		return 1;
	}

	if( args.checkMTime )
	{
		PrintModificationTime( args.coreCount );
		return 0;
	}
	if( args.printPermutations )
	{
		return PrintPermutations( args.shaderPath ) ? 0 : 1;
	}

	// Preload shader file
	if( FAILED( g_includeHandler.Open( D3DXINC_LOCAL, args.shaderPath, NULL, (LPCVOID*)&g_shaderSource, &g_shaderLength ) ) )
	{
		printf( "%s: error X0000: Could not open input file \"%s\"\n", args.shaderPath, args.shaderPath );
		return 1;
	}
	
	g_includeHandler.SetRootPath( args.shaderPath );
	g_messages.SetEntryFileName( args.shaderPath );

	CompileQueue compileQueue( args.coreCount, &CompileShader );

	if( !g_compilerDX9.Create() )
	{
		fflush( stdout );
		return 1;
	}

	if( !g_compilerDX11.Create() )
	{
		fflush( stdout );
		return 1;
	}
	
	if( !g_compilerGL2.Create() )
	{
		fflush( stdout );
		return 1;
	}
	
	if( !g_compilerGL3.Create() )
	{
		fflush( stdout );
		return 1;
	}
	
	if( !g_compilerGL4.Create() )
	{
		fflush( stdout );
		return 1;
	}

	Permutations permutations;
	{
		if( !DiscoverPermutations( permutations, g_shaderSource, g_shaderLength ) )
		{
			g_messages.Flush();
			return 1;
		}

		std::ostringstream os;
		os << '\n';
		for( auto it = permutations.begin(); it != permutations.end(); ++it )
		{
			for( auto jt = it->options.begin(); jt != it->options.end(); ++jt )
			{
				os << "#line " << it->location.lineNumber << std::endl << "#define " << jt->name << ' ' << jt->value << std::endl;
			}
		}
		os << "#line 1" << std::endl;
		auto prefix = os.str();

		g_includeHandler.AddPrefix( args.shaderPath, prefix.c_str(), (LPCVOID*)&g_shaderSource, &g_shaderLength );

		AddPermutationsToWorkQueue( compileQueue, permutations, args.ignorePermutations, args.defines );
	}

	compileQueue.Join();

	if( InterlockedCompareExchange( &g_error, 0, 0 ) )
	{
		g_messages.Flush();
		return 1;
	}

	g_messages.Flush();

	std::vector<unsigned> keys;

	for( auto it = g_compiledEffects.begin(); it != g_compiledEffects.end(); ++it )
	{
		keys.push_back( it->first );
	}

	std::map<unsigned, unsigned> aliases;

	for( size_t i = 0; i < keys.size(); ++i )
	{
		auto b0 = g_compiledEffects[keys[i]];
		if( !b0 )
		{
			continue;
		}
		for( size_t j = i + 1; j < keys.size(); ++j )
		{
			auto b1 = g_compiledEffects[keys[j]];
			if( !b1 )
			{
				continue;
			}
			if( b0->GetBufferSize() == b1->GetBufferSize() && memcmp( b0->GetBufferPointer(), b1->GetBufferPointer(), b0->GetBufferSize() ) == 0 )
			{
				aliases[keys[j]] = keys[i];
				b1->Release();
				g_compiledEffects[keys[j]] = nullptr;
				keys.erase( keys.begin() + j );
				--j;
			}
		}
	}

	// Open the output file
	HANDLE file = CreateFile( args.outputPath, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL );
	if( file == INVALID_HANDLE_VALUE )
	{
		printf( "%s: error X0000: Could not open output file \"%s\" for writing\n", args.shaderPath, args.outputPath );
		fflush( stdout );
		return 1;
	}

	size_t permutationSize = 1;
	for( auto it = permutations.begin(); it != permutations.end(); ++it )
	{
		permutationSize += 2 * sizeof( DWORD ) + 1 + 1 + 1 + it->options.size() * sizeof( DWORD );
	}

	// Write file header
	unsigned totalSize = unsigned( g_compiledEffects.size() );
	unsigned headerSize = ( totalSize * 3 + 1 ) * sizeof( unsigned ) + permutationSize;
	BYTE* fullHeader = new BYTE[headerSize];
	BYTE* headerHead = fullHeader;

	*headerHead++ = BYTE( permutations.size() );
	for( auto it = permutations.begin(); it != permutations.end(); ++it )
	{
		*reinterpret_cast<DWORD*>( headerHead ) = g_stringTable.AddString( it->name.c_str() );
		headerHead += sizeof( DWORD );
		for( size_t j = 0; j < it->options.size(); ++j )
		{
			if( it->options[j].name == it->defaultOption )
			{
				*reinterpret_cast<BYTE*>( headerHead ) = BYTE( j );
				++headerHead;
				break;
			}
		}
		*reinterpret_cast<DWORD*>( headerHead ) = g_stringTable.AddString( it->description.c_str() );
		headerHead += sizeof( DWORD );

		*reinterpret_cast<BYTE*>( headerHead ) = it->type;
		headerHead += sizeof( BYTE );

		*headerHead++ = BYTE( it->options.size() );
		for( auto jt = it->options.begin(); jt != it->options.end(); ++jt )
		{
			*reinterpret_cast<DWORD*>( headerHead ) = g_stringTable.AddString( jt->name.c_str() );
			headerHead += sizeof( DWORD );
		}
	}


	DWORD *header = reinterpret_cast<DWORD*>( headerHead );
	unsigned index = 0;
	



	header[index++] = totalSize;
	unsigned offset = sizeof( DWORD ) + headerSize + g_stringTable.GetSize();

	std::map<unsigned, std::pair<unsigned, unsigned> > offsets;
	for( auto it = g_compiledEffects.begin(); it != g_compiledEffects.end(); ++it )
	{
		header[index++] = it->first;
		if( it->second )
		{
			header[index++] = offset;
			header[index++] = it->second->GetBufferSize();
			offsets[it->first] = std::make_pair( offset, it->second->GetBufferSize() );
			offset += it->second->GetBufferSize();
		}
		else
		{
			auto offset = offsets[aliases[it->first]];
			header[index++] = offset.first;
			header[index++] = offset.second;
		}
	}
	DWORD bytesWritten;
	DWORD version = 8;
	WriteFile( file, &version, sizeof( DWORD ), &bytesWritten, NULL );
	g_stringTable.Write( file );
	WriteFile( file, fullHeader, headerSize, &bytesWritten, NULL );

	// Write compiled code
	for( auto it = g_compiledEffects.begin(); it != g_compiledEffects.end(); ++it )
	{
		if( it->second )
		{
			WriteFile( file, it->second->GetBufferPointer(), it->second->GetBufferSize(), &bytesWritten, NULL );
		}
	}

	delete[] fullHeader;

	CloseHandle( file );
	 
	if( g_generateListing )
	{
		HANDLE file = CreateFile( args.listingFile, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL );
		if( file == INVALID_HANDLE_VALUE )
		{
			printf( "%s: error X0000: Could not open listing file \"%s\" for writing\n", args.shaderPath, args.listingFile );
			fflush( stdout );
			return 1;
		}
		WriteFile( file, g_listing.c_str(), g_listing.length(), &bytesWritten, NULL );
		CloseHandle( file );
	}

	return 0;
}

