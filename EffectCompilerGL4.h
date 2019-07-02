#pragma once

#include "EffectCompilerDx11.h"


class EffectCompilerGL4 : public EffectCompilerBase
{
public:
	bool Create() override;
	bool CompileEffect( const char* source, size_t sourceLength, const std::vector<Macro>& defines, ID3DXInclude* include, EffectData& result ) override;
private:
	EffectCompilerDX11 m_compilerDX11;
};
