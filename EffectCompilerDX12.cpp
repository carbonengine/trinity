#include "stdafx.h"
#include "EffectCompilerDX12.h"
#include "Macro.h"
#include "Platforms.h"


bool EffectCompilerDX12::Create()
{
	return m_compiler.Create();
}

bool EffectCompilerDX12::CompileEffect( const char* source, size_t sourceLength, const std::vector<Macro>& defines, ID3DXInclude* include, EffectData& result )
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
	return m_compiler.CompileEffect( source, sourceLength, defines, include, result, { "5_1", false, false, true } );
}
