#pragma once
#ifndef EffectCompilerDX11_H
#define EffectCompilerDX11_H

struct EffectData;
struct Macro;

class EffectCompilerDX11
{
public:
	bool Create();
	bool CompileEffect( const char* source, size_t sourceLength, const std::vector<Macro>& defines, ID3DXInclude* include, EffectData& result, bool patchShaders = true );
};

#endif //EffectCompilerDX11_H