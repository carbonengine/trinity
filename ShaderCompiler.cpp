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
#include "EffectCompilerDX12.h"
#include "EffectData.h"
#include "Mutex.h"
#include "WorkQueue.h"
#include "Macro.h"
#include "ModifiedTime.h"
#include "Platforms.h"


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


struct CompiledData
{
	EffectData data;
	size_t packedSize;
	std::unique_ptr<uint8_t[]> packed;
};

// Flag indicating a compile error (in some worker thread) causes all worker threads to exit
LONG g_error = 0;
// Include file handler
CachingIncludeHandler g_includeHandler;
// Compiled effect code (indexed by permutaitons IDs)
std::map<uint32_t, std::unique_ptr<CompiledData>> g_compiledEffects;
// Critical section for g_compiledEffects
Mutex g_compiledEffectsCS( 1000 );
// Queue of compiler output messages
CompileMessageQueue g_messages;
// Entry point shader file contents
char* g_shaderSource = NULL;
// Entry point shader file length
uint32_t g_shaderLength = 0;
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

std::map<Platform, std::unique_ptr<EffectCompilerBase>> g_compilers;


Platform GetPlatform( const std::vector<Macro>& defines )
{
	if( auto macro = FindMacro( defines, "PLATFORM" ) )
	{
		auto platform = ParsePlatform( macro->value.c_str() );
		if( platform == PLATFORM_INVALID )
		{
			printf( "Unrecognized platform define \"%s\". Reverting to dx11\n", macro->value.c_str() );
			fflush( stdout );
			return PLATFORM_DX11;
		}
		else
		{
			return platform;
		}
	}
	return PLATFORM_DX11; 
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

	Platform platform = GetPlatform( arguments.defines );

	{
		MutexScope scope( g_compiledEffectsCS );
		if( g_compiledEffects.find( arguments.permutation ) != g_compiledEffects.end() )
		{
			return true;
		}
	}

	std::unique_ptr<CompiledData> compiledData( new CompiledData );


	//EffectData outputData;

	auto compiler = g_compilers.find( platform );
	if( compiler == end( g_compilers ) )
	{
		g_messages.AddMessage( "\\memory(0): error C0000: trying to compile %s effect when %s compiler failed to initialize", GetPlatformShortName( platform ), GetPlatformShortName( platform ) );
		InterlockedExchange( &g_error, 1 );
		return false;
	}
	if( !compiler->second->CompileEffect( g_shaderSource, g_shaderLength, arguments.defines, &g_includeHandler, compiledData->data ) )
	{
		InterlockedExchange( &g_error, 1 );
		return false;
	}

	{
		MutexScope scope( g_compiledEffectsCS );
		g_compiledEffects[arguments.permutation] = std::move( compiledData );
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
	if( !state.DiscoverPermutations( permutations ) )
	{
		return false;
	}
	for( auto it = begin( permutations ); it != end( permutations ); ++it )
	{
		g_stringTable.AddString( it->name.c_str() );
		g_stringTable.AddString( it->description.c_str() );
		for( auto jt = begin( it->options ); jt != end( it->options ); ++jt )
		{
			g_stringTable.AddString( jt->name.c_str() );
		}
	}
	return true;
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
	uint32_t shaderLength;

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
	uint32_t permutation = 0;
	bool done = false;
	while( !done )
	{
		CompileShaderArguments args;
		args.permutation = permutation;
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

	for( int32_t i = 0; i < _PLATFORM_END; ++i )
	{
		std::unique_ptr<EffectCompilerBase> compiler;
		switch( i )
		{
		case PLATFORM_DX9:
			compiler.reset( new EffectCompilerDX9() );
			break;
		case PLATFORM_DX11:
			compiler.reset( new EffectCompilerDX11() );
			break;
		case PLATFORM_GL2:
			compiler.reset( new EffectCompilerGL2() );
			break;
		case PLATFORM_DX12:
			compiler.reset( new EffectCompilerDX12() );
			break;
		case PLATFORM_GL3:
			compiler.reset( new EffectCompilerGL3() );
			break;
		case PLATFORM_GL4:
			compiler.reset( new EffectCompilerGL4() );
			break;
		default:
			continue;
		}
		if( !compiler->Create() )
		{
			printf( "%s: warning X0000: Failed to create %s compiler\n", args.shaderPath, GetPlatformLongName( Platform( i ) ) );
			fflush( stdout );
		}
		g_compilers[Platform( i )] = std::move( compiler );
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

		auto& compiledData = *it->second;

		SizeCountStream size;
		compiledData.data.Save( size );
		compiledData.packedSize = size.GetSize();
		compiledData.packed.reset( new uint8_t[compiledData.packedSize] );

		PackedStream stream( compiledData.packed.get(), compiledData.packedSize );
		compiledData.data.Save( stream );
	}

	std::map<unsigned, unsigned> aliases;

	for( size_t i = 0; i < keys.size(); ++i )
	{
		auto& b0 = g_compiledEffects[keys[i]];
		if( !b0 )
		{
			continue;
		}
		for( size_t j = i + 1; j < keys.size(); ++j )
		{
			auto& b1 = g_compiledEffects[keys[j]];
			if( !b1 )
			{
				continue;
			}
			if( b0->packedSize == b1->packedSize && memcmp( b0->packed.get(), b1->packed.get(), b0->packedSize ) == 0 )
			{
				aliases[keys[j]] = keys[i];
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
	size_t totalSize = g_compiledEffects.size();
	size_t headerSize = ( totalSize * 3 + 1 ) * sizeof( uint32_t ) + permutationSize;
	uint8_t* fullHeader = new uint8_t[headerSize];
	uint8_t* headerHead = fullHeader;

	*headerHead++ = uint8_t( permutations.size() );
	for( auto it = permutations.begin(); it != permutations.end(); ++it )
	{
		*reinterpret_cast<uint32_t*>( headerHead ) = g_stringTable.GetOffset( g_stringTable.AddString( it->name.c_str() ) );
		headerHead += sizeof( uint32_t );
		for( size_t j = 0; j < it->options.size(); ++j )
		{
			if( it->options[j].name == it->defaultOption )
			{
				*reinterpret_cast<uint8_t*>( headerHead ) = uint8_t( j );
				++headerHead;
				break;
			}
		}
		*reinterpret_cast<uint32_t*>( headerHead ) = g_stringTable.GetOffset( g_stringTable.AddString( it->description.c_str() ) );
		headerHead += sizeof( uint32_t );

		*reinterpret_cast<uint8_t*>( headerHead ) = it->type;
		headerHead += sizeof( uint8_t );

		*headerHead++ = uint8_t( it->options.size() );
		for( auto jt = it->options.begin(); jt != it->options.end(); ++jt )
		{
			*reinterpret_cast<uint32_t*>( headerHead ) = g_stringTable.GetOffset( g_stringTable.AddString( jt->name.c_str() ) );
			headerHead += sizeof( uint32_t );
		}
	}


	uint32_t *header = reinterpret_cast<uint32_t*>( headerHead );
	unsigned index = 0;
	



	header[index++] = uint32_t( totalSize );
	size_t offset = sizeof( uint32_t ) + headerSize + g_stringTable.GetSize();

	std::map<uint32_t, std::pair<size_t, size_t> > offsets;
	for( auto it = g_compiledEffects.begin(); it != g_compiledEffects.end(); ++it )
	{
		header[index++] = it->first;
		if( it->second )
		{
			header[index++] = uint32_t( offset );
			header[index++] = uint32_t( it->second->packedSize );
			offsets[it->first] = std::make_pair( offset, it->second->packedSize );
			offset += it->second->packedSize;
		}
		else
		{
			auto offset = offsets[aliases[it->first]];
			header[index++] = uint32_t( offset.first );
			header[index++] = uint32_t( offset.second );
		}
	}
	DWORD bytesWritten;
	uint32_t version = 9;
	WriteFile( file, &version, sizeof( uint32_t ), &bytesWritten, NULL );
	g_stringTable.Write( file );
	WriteFile( file, fullHeader, DWORD( headerSize ), &bytesWritten, NULL );

	// Write compiled code
	for( auto it = g_compiledEffects.begin(); it != g_compiledEffects.end(); ++it )
	{
		if( it->second )
		{
			WriteFile( file, it->second->packed.get(), DWORD( it->second->packedSize ), &bytesWritten, NULL );
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
		WriteFile( file, g_listing.c_str(), DWORD( g_listing.length() ), &bytesWritten, NULL );
		CloseHandle( file );
	}

	return 0;
}

