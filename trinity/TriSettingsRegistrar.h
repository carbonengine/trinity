#pragma once
#ifndef TriSettingsRegistrar_H
#define TriSettingsRegistrar_H

#include "TriSettings.h"
#include "Tr2Renderer.h"

// Use this macro to register settings at filescope!  Like this:
// static s_bingo = 10;
// TRI_REGISTER_SETTING( "Bingo", s_bingo );
#define TRI_REGISTER_SETTING( name_, value_ ) \
	static TriSettingsRegistrar value_##Registration( name_, &value_ )

class TriSettingsRegistrar
{
public:
	template<typename T> TriSettingsRegistrar( const char* name, T* value )
	{
		Tr2Renderer::GetSettings().RegisterSetting( name, value );
	}
};



#endif // TriSettingsRegistrar_H