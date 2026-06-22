// Copyright © 2024 CCP ehf.

#pragma once

#include "gtest/gtest.h"
#include "EffectData.h"
#include "Macro.h"
#include "CompileMessageQueue.h"

extern CompileMessageQueue g_messages;



template <typename Compiler>
bool Compiles( const char* src )
{
	Compiler compiler;
	auto created = compiler.Create();
	[created] { ASSERT_TRUE( created ) << "Failed to create a compiler object: the environment is not set up correctly"; }();

	EffectData data;
	auto compiled = compiler.CompileEffect( src + 1, strlen( src ), {}, data, nullptr );
	g_messages.Flush();
	return compiled;
}

template <typename Compiler>
EffectData Compile( const char* src )
{
	Compiler compiler;
	auto created = compiler.Create();
	[created] { ASSERT_TRUE( created ) << "Failed to create a compiler object: the environment is not set up correctly"; }();

	EffectData data;
	auto compiled = compiler.CompileEffect( src + 1, strlen( src ), {}, data, nullptr );
	g_messages.Flush();
	[compiled] { ASSERT_TRUE( compiled ); }();
	return data;
}

inline std::string FormatString( const char* format, ... )
{
	va_list args;

	va_start( args, format );
	int count = vsnprintf( nullptr, 0, format, args ) + 1;
	va_end( args );

	std::string message;
	message.resize( count );

	va_start( args, format );
	count = vsnprintf( message.data(), message.size(), format, args );
	va_end( args );

	return message;
}
