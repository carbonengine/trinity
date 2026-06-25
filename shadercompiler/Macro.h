// Copyright © 2023 CCP ehf.

#pragma once


struct Macro
{
	std::string name;
	std::string value;

	template <typename Iterator, size_t Size>
	static bool FillDxMacros( D3D_SHADER_MACRO ( &outMacros )[Size], Iterator inBegin, Iterator inEnd )
	{
		bool result = true;
		size_t i = 0;
		for( auto it = inBegin; it != inEnd; ++it )
		{
			if( i + 1 >= Size )
			{
				result = false;
				break;
			}
			outMacros[i].Name = it->name.c_str();
			outMacros[i].Definition = it->value.c_str();
			++i;
		}
		if( Size )
		{
			outMacros[i].Name = outMacros[i].Definition = nullptr;
		}
		return result;
	}
};

inline const Macro* FindMacro( const std::vector<Macro>& macros, const char* name )
{
	for( auto& macro : macros )
	{
		if( macro.name == name )
		{
			return &macro;
		}
	}
	return nullptr;
}

inline Macro* FindMacro( std::vector<Macro>& macros, const char* name )
{
	for( auto& macro : macros )
	{
		if( macro.name == name )
		{
			return &macro;
		}
	}
	return nullptr;
}
