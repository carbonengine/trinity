#pragma once
#ifndef EffectCompilerGL3_H
#define EffectCompilerGL3_H

struct EffectData;

class EffectCompilerGL3
{
public:
	bool Create();
	bool CompileEffect( const char* source, size_t sourceLength, const D3DXMACRO* defines, ID3DXInclude* include, EffectData& result );
private:
};

#endif 