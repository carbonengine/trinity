#pragma once

#include "EffectCompilerBase.h"


class EffectCompilerDX9: public EffectCompilerBase
{
public:
	bool Create() override;
	bool CompileEffect( const char* source, size_t sourceLength, const std::vector<Macro>& defines, ID3DXInclude* include, EffectData& result ) override;

	bool CompileEffect( const char* source, size_t sourceLength, const std::vector<Macro>& defines, ID3DXInclude* include, EffectData& result, bool disableListing );
};