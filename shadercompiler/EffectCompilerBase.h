// Copyright © 2023 CCP ehf.

#pragma once

struct EffectData;
struct Macro;

class EffectCompilerBase
{
public:
	virtual ~EffectCompilerBase()
	{
	}

	virtual bool Create() = 0;
	virtual bool CompileEffect( const char* source, size_t sourceLength, const std::vector<Macro>& defines, EffectData& result, class IWorkQueue* workQueue ) = 0;
};