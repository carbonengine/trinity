#pragma once

#include "EffectCompilerDx9.h"


class EffectCompilerGL3: public EffectCompilerBase
{
public:
	bool Create() override;
	bool CompileEffect( const char* source, size_t sourceLength, const std::vector<Macro>& defines, ID3DXInclude* include, EffectData& result ) override;
private:
	EffectCompilerDX9 m_compilerDX9;
};