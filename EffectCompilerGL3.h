#pragma once
#ifndef EffectCompilerGL3_H
#define EffectCompilerGL3_H

struct EffectData;
struct Macro;

class EffectCompilerGL3
{
public:
	bool Create();
	bool CompileEffect( const char* source, size_t sourceLength, const std::vector<Macro>& defines, ID3DXInclude* include, EffectData& result );
private:
};

#endif 