////////////////////////////////////////////////////////////
//
//    Created:   March 2022
//    Copyright: CCP 2022
//

#pragma once
#if TRINITY_PLATFORM == TRINITY_DIRECTX12

#include <GFSDK_Aftermath.h>
#include <GFSDK_Aftermath_GpuCrashDump.h>
#include <GFSDK_Aftermath_GpuCrashDumpDecoding.h>
#include "../../include/Tr2ShaderAL.h"

// Helper for comparing GFSDK_Aftermath_ShaderHash.
inline bool operator<(const GFSDK_Aftermath_ShaderHash& lhs, const GFSDK_Aftermath_ShaderHash& rhs)
{
	return lhs.hash < rhs.hash;
}

// Helper for comparing GFSDK_Aftermath_ShaderInstructionsHash.
inline bool operator<(const GFSDK_Aftermath_ShaderInstructionsHash& lhs, const GFSDK_Aftermath_ShaderInstructionsHash& rhs)
{
	return lhs.hash < rhs.hash;
}

// Helper for comparing GFSDK_Aftermath_ShaderInstructionsHash.
inline bool operator<(const GFSDK_Aftermath_ShaderDebugInfoIdentifier& lhs, const GFSDK_Aftermath_ShaderDebugInfoIdentifier& rhs)
{
	if (lhs.id[0] == rhs.id[0])
	{
		return lhs.id[1] < rhs.id[1];
	}
	return lhs.id[0] < rhs.id[0];
}

namespace TrinityALImpl
{
	class GpuCrashTracker
	{
	public: 
		GpuCrashTracker();
		~GpuCrashTracker();

		void Initialize( ID3D12Device* device );
		void SetCrashDumpFolder( std::wstring folder);
		bool IsValid() const;

		void PutMarker( ID3D12GraphicsCommandList2* commandList, const char* marker );

		void UnRegisterCommandList(ID3D12GraphicsCommandList2* commandList);
		void RegisterShaderBinary(const Tr2ShaderBytecodeAL& bytecode, const char* shaderPath);

		void GetOffendingShader(std::string& shaderString) const;
	private:

		GFSDK_Aftermath_ContextHandle GetCommandListContextHandle(ID3D12GraphicsCommandList2* commandList);

		// Callback handlers for GPU crash dumps and related data.
		void OnShaderDebugInfo(const void* pShaderDebugInfo, const uint32_t shaderDebugInfoSize);
		void OnDescription( PFN_GFSDK_Aftermath_AddGpuCrashDumpDescription addDescription );
		void OnShaderLookup( const GFSDK_Aftermath_ShaderHash& shaderHash, PFN_GFSDK_Aftermath_SetData setShaderBinary);
		void OnShaderDebugInfoLookup( const GFSDK_Aftermath_ShaderDebugInfoIdentifier& identifier, PFN_GFSDK_Aftermath_SetData setShaderDebugInfo) const;

		void ResolveCrash( const void* pGpuCrashDump, const uint32_t gpuCrashDumpSize );

		bool m_initialized;
		bool m_initializedForDevice;
		std::string m_offendingShader;

		std::map<GFSDK_Aftermath_ShaderHash, std::pair<	const void*,  size_t>> m_shaderHashToBytecode;
		std::map<GFSDK_Aftermath_ShaderInstructionsHash, GFSDK_Aftermath_ShaderHash> m_shaderInstructionsToShaderHash;
		std::map<GFSDK_Aftermath_ShaderDebugInfoIdentifier, std::vector<uint8_t>> m_shaderDebugInfo;
		std::map<ID3D12GraphicsCommandList2*, GFSDK_Aftermath_ContextHandle> m_commandListContextMap;

		std::map<GFSDK_Aftermath_ShaderHash, std::string> m_shaderHashToPath;

		// Static callback wrappers.
		static void GpuCrashDumpCallback( const void* pGpuCrashDump, const uint32_t gpuCrashDumpSize, void* pUserData );
		static void ShaderDebugInfoCallback( const void* pShaderDebugInfo, const uint32_t shaderDebugInfoSize, void* pUserData);
		static void ShaderLookupCallback( const GFSDK_Aftermath_ShaderHash* pShaderHash, PFN_GFSDK_Aftermath_SetData setShaderBinary, void* pUserData);
		static void ShaderDebugInfoLookupCallback( const GFSDK_Aftermath_ShaderDebugInfoIdentifier* pIdentifier, PFN_GFSDK_Aftermath_SetData setShaderDebugInfo, void* pUserData);

	};
}
#endif