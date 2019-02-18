#pragma once
#ifndef EffectCompilerGL4_H
#define EffectCompilerGL4_H

struct EffectData;
struct Macro;


class EffectCompilerGL4
{
public:
	bool Create();
	bool CompileEffect( const char* source, size_t sourceLength, const std::vector<Macro>& defines, ID3DXInclude* include, EffectData& result );
private:
};

#endif 