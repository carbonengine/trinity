// Copyright © 2022 CCP ehf.

#include "StdAfx.h"
#if TRINITY_PLATFORM == TRINITY_DIRECTX12

#include "GpuCrashTracker.h"
#include <GFSDK_Aftermath_GpuCrashDump.h>
#include <GFSDK_Aftermath_GpuCrashDumpDecoding.h>
#include <sstream>
#include <fstream>
#include <iomanip>

#define AFTERMATH_CHECK_ERROR( FC )               \
	[&]() {                                       \
		GFSDK_Aftermath_Result _result = FC;      \
		if( !GFSDK_Aftermath_SUCCEED( _result ) ) \
		{                                         \
			return false;                         \
		}                                         \
		return true;                              \
	}()

#define AFTERMATH_CHECK_AND_LOG_ERROR( FC )                              \
	[&]() {                                                              \
		GFSDK_Aftermath_Result _result = FC;                             \
		if( !GFSDK_Aftermath_SUCCEED( _result ) )                        \
		{                                                                \
			CCP_LOGERR( Aftermath::GetErrorMessage( _result ).c_str() ); \
			return false;                                                \
		}                                                                \
		return true;                                                     \
	}()

namespace Aftermath
{
template <typename T>
inline std::string to_hex_string( T n )
{
	std::stringstream stream;
	stream << std::setfill( '0' ) << std::setw( 2 * sizeof( T ) ) << std::hex << n;
	return stream.str();
}

inline std::string to_string( GFSDK_Aftermath_Result result )
{
	return std::string( "0x" ) + to_hex_string( static_cast<uint32_t>( result ) );
}

inline std::string to_string( const GFSDK_Aftermath_ShaderDebugInfoIdentifier& identifier )
{
	return to_hex_string( identifier.id[0] ) + "-" + to_hex_string( identifier.id[1] );
}

inline std::string to_string( const GFSDK_Aftermath_ShaderHash& hash )
{
	return to_hex_string( hash.hash );
}

inline std::string to_string( const GFSDK_Aftermath_ShaderInstructionsHash& hash )
{
	return to_hex_string( hash.hash ) + "-" + to_hex_string( hash.hash );
}

std::string GetErrorMessage( GFSDK_Aftermath_Result result )
{
	switch( result )
	{
	case GFSDK_Aftermath_Result_FAIL_DriverVersionNotSupported:
		return "Unsupported driver version - requires at least an NVIDIA R435 display driver.";
	case GFSDK_Aftermath_Result_FAIL_D3dDllInterceptionNotSupported:
		return "Aftermath is incompatible with D3D API interception, such as PIX or Nsight Graphics.";
	default:
		return "Aftermath Error 0x" + to_hex_string( result );
	}
}
}

namespace TrinityALImpl
{

GpuCrashTracker::GpuCrashTracker() :
	m_offendingShader( "" ),
	m_initializedForDevice( false )
{
	m_initialized = AFTERMATH_CHECK_ERROR( GFSDK_Aftermath_EnableGpuCrashDumps(
		GFSDK_Aftermath_Version_API,
		GFSDK_Aftermath_GpuCrashDumpWatchedApiFlags_DX,
		GFSDK_Aftermath_GpuCrashDumpFeatureFlags_DeferDebugInfoCallbacks, // Let the Nsight Aftermath library cache shader debug information.
		GpuCrashDumpCallback, // Register callback for GPU crash dumps.
		ShaderDebugInfoCallback, // Register callback for shader debug information.
		nullptr, // callback for GPU crash dump description.
		this ) ); // Set the GpuCrashTracker object as user data for the above callbacks.
}

GpuCrashTracker::~GpuCrashTracker()
{
	if( m_initialized )
	{
		GFSDK_Aftermath_DisableGpuCrashDumps();
	}
}

void GpuCrashTracker::GetOffendingShader( std::string& shaderString ) const
{
	shaderString = m_offendingShader;
}

bool GpuCrashTracker::IsValid() const
{
	return m_initialized;
}

void GpuCrashTracker::Initialize( ID3D12Device* device )
{
	const uint32_t aftermathFlags =
		GFSDK_Aftermath_FeatureFlags_EnableMarkers | // Enable event marker tracking.
		GFSDK_Aftermath_FeatureFlags_EnableResourceTracking | // Enable tracking of resources.
		GFSDK_Aftermath_FeatureFlags_GenerateShaderDebugInfo; // Generate debug information for shaders.
	// There are additional flag to set, but we don't because it will affect performance

	m_initializedForDevice = AFTERMATH_CHECK_ERROR( GFSDK_Aftermath_DX12_Initialize(
		GFSDK_Aftermath_Version_API,
		aftermathFlags,
		device ) );

	if( m_initializedForDevice )
	{
		CCP_LOGNOTICE( "Aftermath enabled for device" );
	}
}

void GpuCrashTracker::RegisterShaderBinary( const Tr2ShaderBytecodeAL& bytecode, const char* shaderPath )
{
	if( !m_initializedForDevice )
	{
		return;
	}
	// Create shader hashes for the shader bytecode
	const D3D12_SHADER_BYTECODE shader{ bytecode.bytecode, bytecode.size };
	GFSDK_Aftermath_ShaderHash shaderHash;
	GFSDK_Aftermath_ShaderInstructionsHash shaderInstructionsHash;
	AFTERMATH_CHECK_AND_LOG_ERROR( GFSDK_Aftermath_GetShaderHash(
		GFSDK_Aftermath_Version_API,
		&shader,
		&shaderHash,
		&shaderInstructionsHash ) );

	// Store the data for shader instruction address mapping when decoding GPU crash dumps.
	m_shaderHashToBytecode[shaderHash] = std::pair( bytecode.bytecode, bytecode.size );
	m_shaderHashToPath[shaderHash] = shaderPath;
	m_shaderInstructionsToShaderHash[shaderInstructionsHash] = shaderHash;
}

// This function should be used to dump the shader debug information to a file
// The code commented out here below is how it should work, if we have dxil shaders at some point
// Handler for shader debug information callbacks (called by ShaderDebugInfoCallback)
void GpuCrashTracker::OnShaderDebugInfo( const void* pShaderDebugInfo, const uint32_t shaderDebugInfoSize )
{
	// Get shader debug information identifier.
	GFSDK_Aftermath_ShaderDebugInfoIdentifier identifier = {};
	AFTERMATH_CHECK_AND_LOG_ERROR( GFSDK_Aftermath_GetShaderDebugInfoIdentifier( GFSDK_Aftermath_Version_API, pShaderDebugInfo, shaderDebugInfoSize, &identifier ) );

	// store the debug info for retrieval in the json file
	std::vector<uint8_t> data( (uint8_t*)pShaderDebugInfo, (uint8_t*)pShaderDebugInfo + shaderDebugInfoSize );
	m_shaderDebugInfo[identifier].swap( data );
}

// Handler for shader debug information lookup callbacks.
// This is used by the JSON decoder for mapping shader instruction
// addresses to DXIL lines or HLSl source lines.ho
void GpuCrashTracker::OnShaderDebugInfoLookup( const GFSDK_Aftermath_ShaderDebugInfoIdentifier& identifier, PFN_GFSDK_Aftermath_SetData setShaderDebugInfo ) const
{
	// Search the list of shader debug information blobs received earlier.
	auto i_debugInfo = m_shaderDebugInfo.find( identifier );
	if( i_debugInfo == m_shaderDebugInfo.end() )
	{
		// Early exit, nothing found. No need to call setShaderDebugInfo.
		return;
	}
	// Let the GPU crash dump decoder know about the shader debug information
	// that was found.
	// This is very important so we get a call to OnShaderLookup
	setShaderDebugInfo( i_debugInfo->second.data(), uint32_t( i_debugInfo->second.size() ) );
}

void GpuCrashTracker::OnShaderLookup( const GFSDK_Aftermath_ShaderHash& shaderHash, PFN_GFSDK_Aftermath_SetData )
{
	// store the crashed shader so we can report it!
	auto shaderPath = m_shaderHashToPath.find( shaderHash );
	if( shaderPath != m_shaderHashToPath.end() )
	{
		m_offendingShader = shaderPath->second;
	}
}

GFSDK_Aftermath_ContextHandle GpuCrashTracker::GetCommandListContextHandle( ID3D12GraphicsCommandList2* commandList )
{
	auto results = m_commandListContextMap.find( commandList );
	if( results != m_commandListContextMap.end() )
	{
		return results->second;
	}
	GFSDK_Aftermath_ContextHandle context = nullptr;
	AFTERMATH_CHECK_AND_LOG_ERROR( GFSDK_Aftermath_DX12_CreateContextHandle( commandList, &context ) );

	m_commandListContextMap[commandList] = context;
	return context;
}

void GpuCrashTracker::UnRegisterCommandList( ID3D12GraphicsCommandList2* commandList )
{
	auto results = m_commandListContextMap.find( commandList );
	if( results == m_commandListContextMap.end() )
	{
		return;
	}

	m_commandListContextMap.erase( results );
}

void GpuCrashTracker::PutMarker( ID3D12GraphicsCommandList2* commandList, const char* marker )
{
	if( !m_initializedForDevice )
	{
		return;
	}
	auto context = GetCommandListContextHandle( commandList );
	AFTERMATH_CHECK_AND_LOG_ERROR( GFSDK_Aftermath_SetEventMarker( context, (void*)marker, (unsigned int)strlen( marker ) + 1 ) );
}

// Helper for writing a GPU crash dump to a file
void GpuCrashTracker::ResolveCrash( const void* pGpuCrashDump, const uint32_t gpuCrashDumpSize )
{
	// Create a GPU crash dump decoder object for the GPU crash dump.
	GFSDK_Aftermath_GpuCrashDump_Decoder decoder = {};
	AFTERMATH_CHECK_AND_LOG_ERROR( GFSDK_Aftermath_GpuCrashDump_CreateDecoder(
		GFSDK_Aftermath_Version_API,
		pGpuCrashDump,
		gpuCrashDumpSize,
		&decoder ) );

	// Decode the crash dump to a JSON string.
	// Fake most of this since we don't care about anything except the shader that failed
	uint32_t jsonSize = 0;
	AFTERMATH_CHECK_AND_LOG_ERROR( GFSDK_Aftermath_GpuCrashDump_GenerateJSON(
		decoder,
		GFSDK_Aftermath_GpuCrashDumpDecoderFlags_ALL_INFO,
		GFSDK_Aftermath_GpuCrashDumpFormatterFlags_NONE,
		ShaderDebugInfoLookupCallback, // need this so we can actually look up what shader crashed
		ShaderLookupCallback, // need this so we can actually look up what shader crashed
		nullptr,
		nullptr,
		this,
		&jsonSize ) );

	// Destroy the GPU crash dump decoder object.
	AFTERMATH_CHECK_AND_LOG_ERROR( GFSDK_Aftermath_GpuCrashDump_DestroyDecoder( decoder ) );
}

// Static callback wrapper for OnCrashDump
void GpuCrashTracker::GpuCrashDumpCallback( const void* pGpuCrashDump, const uint32_t gpuCrashDumpSize, void* pUserData )
{
	auto tracker = reinterpret_cast<GpuCrashTracker*>( pUserData );
	tracker->ResolveCrash( pGpuCrashDump, gpuCrashDumpSize );
}

void GpuCrashTracker::ShaderDebugInfoCallback(
	const void* pShaderDebugInfo,
	const uint32_t shaderDebugInfoSize,
	void* pUserData )
{
	auto pGpuCrashTracker = reinterpret_cast<GpuCrashTracker*>( pUserData );
	pGpuCrashTracker->OnShaderDebugInfo( pShaderDebugInfo, shaderDebugInfoSize );
}

void GpuCrashTracker::ShaderDebugInfoLookupCallback(
	const GFSDK_Aftermath_ShaderDebugInfoIdentifier* pIdentifier,
	PFN_GFSDK_Aftermath_SetData setShaderDebugInfo,
	void* pUserData )
{
	auto pGpuCrashTracker = reinterpret_cast<GpuCrashTracker*>( pUserData );
	pGpuCrashTracker->OnShaderDebugInfoLookup( *pIdentifier, setShaderDebugInfo );
}

void GpuCrashTracker::ShaderLookupCallback(
	const GFSDK_Aftermath_ShaderHash* pShaderHash,
	PFN_GFSDK_Aftermath_SetData setShaderBinary,
	void* pUserData )
{
	auto pGpuCrashTracker = reinterpret_cast<GpuCrashTracker*>( pUserData );
	pGpuCrashTracker->OnShaderLookup( *pShaderHash, setShaderBinary );
}

}

#endif