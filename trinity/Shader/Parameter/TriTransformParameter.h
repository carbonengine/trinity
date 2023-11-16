#pragma once
#ifndef TriTransformParameter_H
#define TriTransformParameter_H

#include "include/ITriEffectParameter.h"
#include "include/ITriConstants.h"


BLUE_DECLARE( TriTransformParameter );
BLUE_CLASS_ALLOW_DELAYED_DELETE( TriTransformParameter );

class TriTransformParameter:
	public ITriEffectParameter
{

public:

	TriTransformParameter(IRoot* lockobj = NULL);
	~TriTransformParameter() {};

	using ITriEffectParameter::Lock;
	using ITriEffectParameter::Unlock;

	/////////////////////////////////////////////////////////////////////////////////////
	// ITriEffectParameter
	/////////////////////////////////////////////////////////////////////////////////////
	virtual void CopyValueToEffect(	Tr2RenderContextEnum::ShaderType inputType, 
									unsigned char* destHandle, 
									size_t size,
									Tr2RenderContext &renderContext ) const;
	const char* GetParameterName() const { return m_name.c_str(); }
	// The following methods are potentially used by the old scene.  This parameter will
	// never be used there so I'm simply going to stub them out.
	void RebuildEffectHandles( Tr2Shader* effectRes ) {};
	unsigned GetHashValue( unsigned startingHash ) const;


	EXPOSE_TO_BLUE();

private:
	BlueSharedString m_name;

	TRITRANSFORMBASE m_transformBase; 
	Vector3 m_scaling;
	Vector3 m_rotationCenter;
	Quaternion m_rotation;
	Vector3 m_translation;
	Matrix m_worldTransform;
};

TYPEDEF_BLUECLASS(TriTransformParameter);

#endif 
