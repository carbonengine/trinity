////////////////////////////////////////////////////////////
//
//    Created:   November 2011
//    Copyright: CCP 2011
//

#include "stdafx.h"
#include "CachingIncludeHandler.h"
#include "CompileMessageQueue.h"
#include "StringTable.h"
#include "EffectCompilerDX11.h"
#include "EffectCompilerDX12.h"
#include "EffectCompilerMetal.h"
#include "EffectData.h"
#include "WorkQueue.h"
#include "Macro.h"
#include "ModifiedTime.h"
#include "Platforms.h"
#include "ShaderCompilerConfig.h"
#include <atomic>



struct CompiledData
{
	EffectData data;
	size_t packedSize;
	std::unique_ptr<uint8_t[]> packed;
};

// Flag indicating a compile error (in some worker thread) causes all worker threads to exit
std::atomic<long> g_error( 0 );
// Include file handler
CachingIncludeHandler g_includeHandler;
// Compiled effect code (indexed by permutaitons IDs)
std::map<uint32_t, std::unique_ptr<CompiledData>> g_compiledEffects;
// Critical section for g_compiledEffects
std::mutex g_compiledEffectsCS;
// Queue of compiler output messages
CompileMessageQueue g_messages;
// Entry point shader file contents
const char* g_shaderSource = NULL;
// Entry point shader file length
size_t g_shaderLength = 0;
// Print warning messages?
bool g_printWarnings = true;
std::string g_metalToolsPath;


// Generate DX11 HLSL listing file
bool g_generateListing = false;
// Listing text
std::string g_listing;
// CS for listing access
std::mutex g_listingCS;

// Optimization level
unsigned g_optimizationLevel = 3;

// Avoid flow control in compiled shaders (especially useful for GLES)
bool g_avoidFlowControl = false;

StringTable g_stringTable;

std::mutex g_compilersMutex;
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
#if _WIN32
	return PLATFORM_DX11;
#else
	return PLATFORM_METAL;
#endif
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
	if( g_error.load( std::memory_order_relaxed ) )
	{
		return false;
	}

	Platform platform = GetPlatform( arguments.defines );

	{
		std::lock_guard scope( g_compiledEffectsCS );
		if( g_compiledEffects.find( arguments.permutation ) != g_compiledEffects.end() )
		{
			return true;
		}
	}

	std::unique_ptr<CompiledData> compiledData( new CompiledData );


	//EffectData outputData;

	EffectCompilerBase* compiler = nullptr;
	{
		std::lock_guard lockCompilers( g_compilersMutex );

		auto found = g_compilers.find( platform );
		if( found == end( g_compilers ) )
		{
			std::unique_ptr<EffectCompilerBase> newCompiler;
			switch( platform )
			{
#if _WIN32
			case PLATFORM_DX11:
				newCompiler.reset( new EffectCompilerDX11() );
				break;
			case PLATFORM_DX12:
				newCompiler.reset( new EffectCompilerDX12() );
				break;
#endif
			case PLATFORM_METAL:
				newCompiler.reset( new EffectCompilerMetal() );
				break;
			default:
				g_messages.AddMessage( "\\memory(0): error X0000: unsupported platform %i", int( platform ) );
				g_error = 1;
				return false;
			}
			if( !newCompiler->Create() )
			{
				g_messages.AddMessage( "\\memory(0): error X0000: failed to create %s compiler", GetPlatformLongName( platform ) );
				g_error = 1;
				return false;
			}
			compiler = newCompiler.get();
			g_compilers[platform] = std::move( newCompiler );
		}
		else
		{
			compiler = found->second.get();
		}
	}
	if( !compiler->CompileEffect( g_shaderSource, g_shaderLength, arguments.defines, compiledData->data ) )
	{
		g_error = 1;
		return false;
	}

	{
		std::lock_guard scope( g_compiledEffectsCS );
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
	printf( "  /no_permutations - Ignore permutation pragmas\n" );
	printf( "  /O{0,1,2,3} - Set optimization level. 3 is default\n" );
	printf( "  /mtime - Determine which compiled files are out of date\n" );
	printf( "  /Gfa - Avoid flow control constructs\n" );
	printf( "  /Gc <switch> <path> - Compile GLSL to binary shader using external compiler\n" );
	printf( "  /E{e,w,d}[extension] - Specify support for all or certain GLES extensions\n" );
	printf( "  /novalidate - Skip validating converted GLSL code\n" );
	printf( "  /permutations - Print permutations of the shader\n" );
#if CCP_TELEMETRY_ENABLED
	printf( "  /telemetry - Enable RAD Telemetry\n" );
#endif
	printf( "  /metal <path> - Path to Metal Developer Tools for Windows\n" );
	printf( "input_file - Path to input HLSL file\n" );
	printf( "output_file - Path to output binary file\n" );
}

#include "ParserUtils.h"
#include "HLSLParser.h"
#include "ParserState.h"

bool DiscoverPermutations( Permutations& permutations, const char* shaderSource, size_t shaderLength )
{
	ZoneScoped;

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
		coreCount( std::max( 1u, std::thread::hardware_concurrency() ) ),
		listingFile( nullptr ),
		checkMTime( false ),
		printPermutations( false ),
		ignorePermutations( false ),
		telemetry( false )
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
	bool telemetry;
};


bool ExtractCommandLineArguments( ProgramArguments& args, int argc, char* argv[] )
{
	for( int i = 1; i < argc; ++i )
	{
		if( strcmp( argv[i], "/threads" ) == 0 )
		{
			++i;
			if( i < argc )
			{
				args.coreCount = std::max( atoi( argv[i] ), 1 );
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
		else if( strcmp( argv[i], "/single" ) == 0 )
		{
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
		else if( strcmp( argv[i], "/metal" ) == 0 )
		{
			++i;
			if( i < argc )
			{
				g_metalToolsPath = argv[i];
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
		else if( strcmp( argv[i], "/Gfa" ) == 0 )
		{
			g_avoidFlowControl = true;
		}
#if CCP_TELEMETRY_ENABLED
		else if( strcmp( argv[i], "/telemetry" ) == 0 )
		{
			args.telemetry = true;
		}
#endif
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
	auto shader = g_includeHandler.Open( shaderPath );

	if( !shader )
	{
		printf( "%s: error X0000: Could not open input file \"%s\"\n", shaderPath, shaderPath );
		return false;
	}
	g_includeHandler.SetRootPath( shaderPath );
	g_messages.SetEntryFileName( shaderPath );

	Permutations permutations;
	if( !DiscoverPermutations( permutations, shader->data, shader->size ) )
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

struct WithTelemetry
{
#if CCP_TELEMETRY_ENABLED
	explicit WithTelemetry( bool enable ) : m_enabled(enable)
	{
		if( !enable )
		{
			return;
		}
#if TRACY_MANUAL_LIFETIME
		tracy::StartupProfiler();
#endif
	}

	~WithTelemetry()
	{
		if ( m_enabled )
		{
			FrameMark;
#if TRACY_MANUAL_LIFETIME
			tracy::ShutdownProfiler();
#endif
		}
	}

	private:
		bool m_enabled{false};
#else
	explicit WithTelemetry( bool )
	{
	}
#endif
};

#if _WIN32
int _tmain(int argc, _TCHAR* argv[])
#else
int main(int argc, char* argv[])
#endif
{
	ProgramArguments args;
	if( !ExtractCommandLineArguments( args, argc, argv ) )
	{
		PrintUsage();
		return 1;
	}

	WithTelemetry withTelemetry( args.telemetry );

	if( args.checkMTime )
	{
		PrintOutOfDateFiles( args.coreCount );
		return 0;
	}
	if( args.printPermutations )
	{
		return PrintPermutations( args.shaderPath ) ? 0 : 1;
	}

	// Preload shader file
	auto shader = g_includeHandler.Open( args.shaderPath );
	if( !shader )
	{
		printf( "%s: error X0000: Could not open input file \"%s\"\n", args.shaderPath, args.shaderPath );
		return 1;
	}
	g_shaderSource = shader->data;
	g_shaderLength = shader->size;

	g_includeHandler.SetRootPath( args.shaderPath );
	g_messages.SetEntryFileName( args.shaderPath );

	CompileQueue compileQueue( args.coreCount, &CompileShader );

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

		if( auto newShader = g_includeHandler.AddPrefix( args.shaderPath, prefix.c_str() ) )
		{
			g_shaderSource = newShader->data;
			g_shaderLength = newShader->size;
		}

		AddPermutationsToWorkQueue( compileQueue, permutations, args.ignorePermutations, args.defines );
	}

	compileQueue.Join();

	if( g_error.load( std::memory_order_relaxed ) )
	{
		g_messages.Flush();
		return 1;
	}

	g_messages.Flush();

	std::vector<unsigned> keys;

	{
		// Add strings for permutations
		for( auto it = permutations.begin(); it != permutations.end(); ++it )
		{
			g_stringTable.AddString( it->name.c_str() );
			g_stringTable.AddString( it->description.c_str() );
			for( auto jt = it->options.begin(); jt != it->options.end(); ++jt )
			{
				g_stringTable.AddString( jt->name.c_str() );
			}
		}
	}

	{
		ZoneScopedN( "Packing" );

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
	}

	std::map<unsigned, unsigned> aliases;

	{
		ZoneScopedN( "Find aliases" );
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
	}

	{
		ZoneScopedN( "Saving" );
		// Open the output file
		FILE* file = nullptr;
		if( fopen_s( &file, args.outputPath, "wb" ) != 0 )
		{
			printf( "%s: error X0000: Could not open output file \"%s\" for writing\n", args.shaderPath, args.outputPath );
			fflush( stdout );
			return 1;
		}

		size_t permutationSize = 1;
		for( auto it = permutations.begin(); it != permutations.end(); ++it )
		{
			permutationSize += 2 * sizeof( uint32_t ) + 1 + 1 + 1 + it->options.size() * sizeof( uint32_t );
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

		uint32_t* header = reinterpret_cast<uint32_t*>( headerHead );
		unsigned index = 0;

		header[index++] = uint32_t( totalSize );
		size_t offset = sizeof( uint32_t ) + sizeof( uint32_t ) + 32 + headerSize + g_stringTable.GetSize();

		std::map<uint32_t, std::pair<size_t, size_t>> offsets;
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
				auto aliasOffset = offsets[aliases[it->first]];
				header[index++] = uint32_t( aliasOffset.first );
				header[index++] = uint32_t( aliasOffset.second );
			}
		}

		uint32_t version = DATA_VERSION;
		fwrite( &version, 1, sizeof( uint32_t ), file );

		fwrite( &ShaderCompilerVersion, sizeof( ShaderCompilerVersion ), 1, file );
		
		auto hash = GetSourceHash( args.shaderPath, args.defines );
		fwrite( hash.c_str(), 1, hash.length(), file );

		g_stringTable.Write( file );
		fwrite( fullHeader, 1, headerSize, file );

		// Write compiled code
		for( auto it = g_compiledEffects.begin(); it != g_compiledEffects.end(); ++it )
		{
			if( it->second )
			{
				fwrite( it->second->packed.get(), 1, it->second->packedSize, file );
			}
		}

		delete[] fullHeader;

		fclose( file );
		file = nullptr;
	}

	if( g_generateListing )
	{
		FILE* file = nullptr;
		if( fopen_s( &file, args.listingFile, "wb" ) != 0 )
		{
			printf( "%s: error X0000: Could not open listing file \"%s\" for writing\n", args.shaderPath, args.listingFile );
			fflush( stdout );
			return 1;
		}
		fwrite( g_listing.c_str(), 1, g_listing.length(), file );
		fclose( file );
	}

	return 0;
}
