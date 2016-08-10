#pragma once
#ifndef EffectCompilerDX11_H
#define EffectCompilerDX11_H

struct EffectData;

class EffectCompilerDX11
{
public:
	bool Create();
	bool CompileEffect( const char* source, size_t sourceLength, const D3DXMACRO* defines, ID3DXInclude* include, EffectData& result, bool patchShaders = true );
};

#endif //EffectCompilerDX11_H