#pragma once
#ifndef EffectCompilerDx9_H
#define EffectCompilerDx9_H

struct EffectData;

class EffectCompilerDX9
{
public:
	bool Create();
	bool CompileEffect( const char* source, size_t sourceLength, const D3DXMACRO* defines, ID3DXInclude* include, EffectData& result, bool disableListing = false );
};

#endif // EffectCompilerDx9_H