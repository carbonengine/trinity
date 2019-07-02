#pragma once

#include "EffectCompilerDX11.h"


class EffectCompilerDX12: public EffectCompilerBase
{
public:
	bool Create();
	bool CompileEffect( const char* source, size_t sourceLength, const std::vector<Macro>& defines, ID3DXInclude* include, EffectData& result );
private:
	EffectCompilerDX11 m_compiler;
};
