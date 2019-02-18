#pragma once
#ifndef EffectCompilerDx9_H
#define EffectCompilerDx9_H

struct EffectData;
struct Macro;

class EffectCompilerDX9
{
public:
	bool Create();
	bool CompileEffect( const char* source, size_t sourceLength, const std::vector<Macro>& defines, ID3DXInclude* include, EffectData& result, bool disableListing = false );
};

#endif // EffectCompilerDx9_H