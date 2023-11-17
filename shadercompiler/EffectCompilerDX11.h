#pragma once

#if _WIN32
#include "EffectCompilerBase.h"


class EffectCompilerDX11: public EffectCompilerBase
{
public:
	bool Create() override;
	bool CompileEffect( const char* source, size_t sourceLength, const std::vector<Macro>& defines, EffectData& result ) override;

	struct CompileOptions
	{
		const char* minShaderVersion; // minimal shader version (5_0 by default)
		bool addSpaces; // add space declarations to shader resources (dx12)
	};
	bool CompileEffect( const char* source, size_t sourceLength, const std::vector<Macro>& defines, EffectData& result, const CompileOptions& compileOptions );

private:
	std::unordered_map<std::string, CComPtr<ID3D10Blob>> m_compiled;
	std::mutex m_compiledCS;
};
#endif
