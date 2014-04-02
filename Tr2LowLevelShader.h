#pragma once
#ifndef Tr2LowLevelShader_h
#define Tr2LowLevelShader_h

// Trinity headers
#include "ITr2ShaderState.h"

// --------------------------------------------------------------------------------------
// Description:
//   Shows where does the code for a low-level shader comes from (cache, source or error
//   shader). Used for debugging.
// SeeAlso:
//   Tr2LowLevelShader, Tr2HighLevelShader
// --------------------------------------------------------------------------------------
enum Tr2ShaderCodeSource
{
	// Shader code comes from error effect
	SS_ERROR_SHADER,
	// Shader code comes from pre-compiled cache file
	SS_FROM_CACHE,
	// Shader code comes from source file
	SS_FROM_SOURCE,
};

// --------------------------------------------------------------------------------------
// Description:
//   Tr2LowLevelShader is a class that sets up the low level RS and SS, and sets up pixel
//   and vertex shaders.
// SeeAlso:
//   Tr2ShaderMaterial, Tr2LowLevelShaderPass
// --------------------------------------------------------------------------------------
BLUE_CLASS( Tr2LowLevelShader ) :
	public IInitialize,
	public ITr2ShaderState
{
public:
	Tr2LowLevelShader( IRoot* lockobj = NULL );
	~Tr2LowLevelShader();

	EXPOSE_TO_BLUE();

	/////////////////////////////////////////////////////////////////////////////////////
	// IInitialize
	virtual bool Initialize();

	/////////////////////////////////////////////////////////////////////////////////////
	// ITr2ShaderState
	virtual uint32_t GetPassCount() const;

	virtual void ApplyAllStateForPass(	uint32_t passIndex,
										Tr2RenderContext &renderContext ) const;
	// Return a bitfield that indicates which shader types are used by this effect/material.
	virtual uint32_t GetShaderTypeMask();

	virtual const Tr2EffectConstantVector& GetConstantTable( 
					uint32_t passIx, 
					Tr2RenderContextEnum::ShaderType type );

	virtual void GetDefaultConstantValues(
					uint32_t passIndex,
					Tr2RenderContextEnum::ShaderType type,
					unsigned &count,
					const void*& values );

	virtual const Tr2EffectResourceMap& GetInputResources(
					uint32_t passIx, 
					Tr2RenderContextEnum::ShaderType type );

	virtual const Tr2EffectResourceMap& GetInputUavs(
					uint32_t passIx, 
					Tr2RenderContextEnum::ShaderType type );

	// individual accessors if needed.
	virtual void ApplyRenderStates( 
					uint32_t passIndex,
					Tr2RenderContext &renderContext ) const;

	virtual void ApplySamplerStates( 
					uint32_t passIndex, 
					Tr2RenderContextEnum::ShaderType type,
					Tr2RenderContext &renderContext ) const;

	virtual void ApplyShader( 
					uint32_t passIndex, 
					Tr2RenderContextEnum::ShaderType type,
					Tr2RenderContext &renderContext ) const;

	virtual const Tr2EffectConstant* GetConstant( const char* name ) const;
	virtual const Tr2EffectResource* GetResource( const char* name ) const;
	virtual const Tr2EffectParameterAnnotationMap* GetParameterAnnotations( const char* parameterName ) const;

	virtual bool HasShaderStage( uint32_t passIndex, Tr2RenderContextEnum::ShaderType type ) const;

	/// 
	unsigned int GetSortValue() const;
	
	unsigned int GetPermuteIndex() const { return m_permuteIndex; }
	Tr2ShaderCodeSource GetCodeSource() const { return m_codeSource; }
	Tr2EffectDefine* GetCompilerDefines() const { return m_compilerDefines; }
	const std::string& GetEffectPath() const { return m_effectPath; }
		
	void SetPermuteIndex( unsigned int permuteIndex );
	void SetCodeSource( Tr2ShaderCodeSource codeSource );
	void SetCompilerDefines( Tr2EffectDefine* d );
	Tr2EffectDescription& GetEffect();
	void SetEffectPath( const std::string& effectPath );

	const Tr2Pass& GetPass( unsigned int passIx ) const;

	// Break apart the effect and extract state and parameters
	void ProcessEffect( void );

	friend class Tr2EffectStateManager;

private:
	// Permute index, derived from situation flag bits
	unsigned int m_permuteIndex;
	// Actual fxc compiler defines
	Tr2EffectDefine* m_compilerDefines;
	// Where does shader source comes from (for debugging)
	Tr2ShaderCodeSource m_codeSource;

	bool m_inErrorState;

	unsigned int m_sortValue;
	unsigned m_shaderTypeMask;

	Tr2EffectDescription m_effect;

	std::string m_effectPath;

#if BLUE_WITH_PYTHON
	static PyObject* PyGetDefines( PyObject* , PyObject* );
#endif
};

TYPEDEF_BLUECLASS( Tr2LowLevelShader );

#endif
