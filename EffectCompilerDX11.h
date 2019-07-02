#pragma once

#include "EffectCompilerBase.h"


class EffectCompilerDX11: public EffectCompilerBase
{
public:
	bool Create() override;
	bool CompileEffect( const char* source, size_t sourceLength, const std::vector<Macro>& defines, ID3DXInclude* include, EffectData& result ) override;

	struct CompileOptions
	{
		const char* minShaderVersion; // minimal shader version (5_0 by default)
		bool addPixelOffset; // add 0.5 pixel offset to VS to emulate dx9
		bool compileShadowShaders; // add "shadow" shaders with dx9 feature emulation (clip planes, alpha test) when required
		bool addSpaces; // add space declarations to shader resources (dx12)
	};
	bool CompileEffect( const char* source, size_t sourceLength, const std::vector<Macro>& defines, ID3DXInclude* include, EffectData& result, const CompileOptions& compileOptions );
};
