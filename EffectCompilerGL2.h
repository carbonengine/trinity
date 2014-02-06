#pragma once
#ifndef EffectCompilerGL2_H
#define EffectCompilerGL2_H

struct EffectData;

// --------------------------------------------------------------------------------------
// Description:
//   Information on allowed GLES extensions. This structure is filled from command line
//   parameters. It is possible to ENABLE, WARN (issue a warning when extension is used)
//   or DISABLE (issue an error when extension is used) individual extensions as well as
//   all extensions. By default each extension used will result in a warning. If 
//   extension used in the shader is ENABLEd the shader code will assume the extension
//   exists on the target hardware. If extension used has WARN flag, the shader code
//   checks if the extension exists on the target hardware and if not reverts to 
//   /semi/sensible fallback.
// --------------------------------------------------------------------------------------
struct GlesExtensionInfo
{
	enum Support
	{
		// Issue an error if extension is used
		DISABLE,
		// Expect an extension to exist
		ENABLE,
		// Issue a warning if extension is used
		WARN,
	};

	typedef std::map<std::string, Support> ExtensionMap;

	// ----------------------------------------------------------------------------------
	// Description:
	//   Check requested support for extension.
	// Arguments:
	//   name - Extension name
	// Return value:
	//   Extension support level
	// ----------------------------------------------------------------------------------
	Support Supports( const char* name )
	{
		auto it = m_extensions.find( name );
		if( it != m_extensions.end() )
		{
			return it->second;
		}
		return m_all;
	}

	// Fallback support for all extensions
	Support m_all;
	// Support level for individual extensions
	ExtensionMap m_extensions;
};

class EffectCompilerGL2
{
public:
	bool Create();
	bool CompileEffect( const char* source, size_t sourceLength, const D3DXMACRO* defines, ID3DXInclude* include, EffectData& result );
private:
};

#endif //EffectCompilerGL2_H