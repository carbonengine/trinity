// Copyright © 2023 CCP ehf.

#pragma once
#ifndef Tr2Shader_h
#define Tr2Shader_h

#include "Tr2EffectDescription.h"

extern const BlueSharedString DEFAULT_TECHNIQUE;
extern const BlueSharedString ANY_TECHNIQUE;


BLUE_CLASS( Tr2Shader ) :
	public IRoot
{
public:
	Tr2Shader( IRoot* lockobj = NULL );
	~Tr2Shader();

	EXPOSE_TO_BLUE();

	bool GetTechniqueIndex( const BlueSharedString& name, uint32_t& index ) const;

	uint32_t GetPassCount( uint32_t techniqueIndex ) const;
	void ApplyAllStateForPass( uint32_t techniqueIndex, uint32_t passIndex, Tr2RenderContext& renderContext ) const;

	void ApplyRenderStates( uint32_t techniqueIndex, uint32_t passIndex, Tr2RenderContext& renderContext ) const;
	uint32_t ApplyShaderOverride( uint32_t techniqueIndex, uint32_t passIndex, const Tr2Shader& overrideShader, uint32_t overridePassIndex, Tr2RenderContext& renderContext ) const;

	uint32_t GetShaderTypeMask( uint32_t techniqueIndex ) const;

	const Tr2EffectConstant* GetConstant( const char* name ) const;
	const Tr2EffectResource* GetResource( const char* name ) const;
	const Tr2EffectParameterAnnotationMap* GetParameterAnnotations( const char* parameterName ) const;

	unsigned int GetSortValue() const;
	const Tr2EffectDescription& GetEffectDescription() const;
	Tr2EffectDescription& GetEffect();

	void ProcessEffect();

	bool HasVertexBufferAccessInRtShadow();

private:
	unsigned int m_sortValue;
	Tr2EffectDescription m_effect;
	bool m_hasVertexBufferAccessInRtShadow;
};

TYPEDEF_BLUECLASS( Tr2Shader );

#endif
