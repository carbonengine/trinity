#pragma once
#ifndef EffectCompilerGL4_H
#define EffectCompilerGL4_H

struct EffectData;


class EffectCompilerGL4
{
public:
	bool Create();
	bool CompileEffect( const char* source, size_t sourceLength, const D3DXMACRO* defines, ID3DXInclude* include, EffectData& result );
private:
};

#endif 