#pragma once
#ifndef EffectCompilerOrbis_H
#define EffectCompilerOrbis_H

struct EffectData;

class EffectCompilerOrbis
{
public:
	bool Create();
	bool CompileEffect( const char* source, size_t sourceLength, const D3DXMACRO* defines, ID3DXInclude* include, EffectData& result );
};

#endif