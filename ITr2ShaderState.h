#pragma once

#ifndef ITr2ShaderState_h
#define ITr2ShaderState_h

#include "Tr2EffectDescription.h"

BLUE_DECLARE_INTERFACE( ITr2ShaderState );


BLUE_INTERFACE( ITr2ShaderState ) : public  IRoot
{
public:
	virtual uint32_t GetPassCount() const = 0;

	virtual void ApplyAllStateForPass(	uint32_t passIndex,
										Tr2RenderContext &renderContext ) const = 0;
	// Return a bitfield that indicates which shader types are used by this effect/material.
	virtual uint32_t GetShaderTypeMask() = 0;

	virtual const Tr2EffectDescription& GetEffectDescription() const = 0;

	// individual accessors if needed.
	virtual void ApplyRenderStates( 
					uint32_t passIndex,
					Tr2RenderContext &renderContext ) const = 0;

	virtual void ApplySamplerStates( 
					uint32_t passIndex, 
					Tr2RenderContextEnum::ShaderType type,
					Tr2RenderContext &renderContext ) const = 0;

	virtual void ApplyShader( 
					uint32_t passIndex, 
					Tr2RenderContextEnum::ShaderType type,
					Tr2RenderContext &renderContext ) const = 0;

	virtual const Tr2EffectConstant* GetConstant( const char* name ) const = 0;
	virtual const Tr2EffectResource* GetResource( const char* name ) const = 0;
	virtual const Tr2EffectParameterAnnotationMap* GetParameterAnnotations( const char* parameterName ) const = 0;
};


// Anti-copy-paste helpers

// Find an annotation called name in the map, and return its value. If not found, or it's not a
// bool, or map is nullptr, return defaultValue instead.
bool GetBool( const Tr2EffectParameterAnnotationMap* map, const char* annotationName, bool defaultValue = false );

// Find an annotation called name in effectResource's annoation map, and return its value. If not found, or it's not a
// bool, or the shader or map are nullptrs, return defaultValue instead.
bool GetBool( const ITr2ShaderState* shaderState, const char* paramName, const char* annotationName, bool defaultValue = false );


#endif
