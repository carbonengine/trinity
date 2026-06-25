// Copyright © 2023 CCP ehf.

#pragma once

#if _WIN32
#include "EffectCompilerDX11.h"


class EffectCompilerDX12 : public EffectCompilerBase
{
public:
	bool Create();
	bool CompileEffect( const char* source, size_t sourceLength, const std::vector<Macro>& defines, EffectData& result, class IWorkQueue* workQueue );

private:
	EffectCompilerDX11 m_compiler;
};
#endif
