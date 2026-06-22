// Copyright © 2023 CCP ehf.

#include "stdafx.h"
#if _WIN32
#include "EffectCompilerDX12.h"
#include "Macro.h"
#include "Platforms.h"

#include "CompileMessageQueue.h"
extern CompileMessageQueue g_messages;

bool EffectCompilerDX12::Create()
{
	ZoneScoped;

	return m_compiler.Create();
}

bool EffectCompilerDX12::CompileEffect( const char* source, size_t sourceLength, const std::vector<Macro>& defines, EffectData& result, IWorkQueue* workQueue )
{
	std::vector<Macro> newDefines = defines;
	if( auto define = FindMacro( newDefines, "PLATFORM" ) )
	{
		define->value = GetPlatformIdString( PLATFORM_DX12 );
	}
	else
	{
		newDefines.push_back( { "PLATFORM", GetPlatformIdString( PLATFORM_DX12 ) } );
	}
	return m_compiler.CompileEffect( source, sourceLength, defines, result, { "5_1", true, true }, workQueue );
}
#endif
