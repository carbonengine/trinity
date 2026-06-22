// Copyright © 2023 CCP ehf.

#pragma once

#if _WIN32
#include "EffectCompilerBase.h"


class EffectCompilerDX11 : public EffectCompilerBase
{
public:
	bool Create() override;
	bool CompileEffect( const char* source, size_t sourceLength, const std::vector<Macro>& defines, EffectData& result, class IWorkQueue* workQueue ) override;

	struct CompileOptions
	{
		const char* minShaderVersion; // minimal shader version (5_0 by default)
		bool addSpaces; // add space declarations to shader resources (dx12)
		bool useStaticSamplers; // dx12
	};
	bool CompileEffect( const char* source, size_t sourceLength, const std::vector<Macro>& defines, EffectData& result, const CompileOptions& compileOptions, class IWorkQueue* workQueue );

private:
	struct SyncData
	{
		bool compiled = false;
		std::condition_variable conditionVariable;
		std::mutex mutex;
		CComPtr<ID3D10Blob> passResource;
		CComPtr<IDxcBlob> libraryResource;
		CComPtr<IDxcBlob> libraryReflection;
	};

	std::unordered_map<std::string, std::shared_ptr<SyncData>> m_compiled;
	std::mutex m_compiledCS;
	std::mutex m_pdbCS;
	CComPtr<IDxcUtils> m_dxilUtils;
};
#endif
