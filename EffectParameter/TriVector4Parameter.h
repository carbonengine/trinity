/* 
	*************************************************************************

	TriPerformer.h

	Created:   October 2005
	OS:        Win32
	Project:   Trinity

	Description:   
		
		FORMERLY TriEffect
		TriPerformer is essentially a version of an animation that actually has children that need rendering.

	Dependencies:

		DirectX 9.0, Probably more, ytbd.

	(c) CCP 2005

	*************************************************************************
*/

#ifndef _TRIVECTOR4PARAMETER_H_
#define _TRIVECTOR4PARAMETER_H_


#include "include/ITriEffectParameter.h"

#define TRIVECTOR4PARAMETER_Description "TriFloatParameter"

BLUE_DECLARE_INTERFACE( ITr2ShaderState );

class TriVector4Parameter:
	public ITriEffectParameter,
	public INotify,
	public IInitialize
{

public:
	EXPOSE_TO_BLUE();

	TriVector4Parameter(IRoot* lockobj = NULL);
	~TriVector4Parameter();

	using ITriEffectParameter::Lock;
	using ITriEffectParameter::Unlock;

	// NB: this class should really be linked to a SEPERATE float entity...
	// I'm going with the easy implementation to start with
	Vector4 m_value;
	BlueSharedString m_name;

	bool m_isUsedByEffect;
	bool m_isSrgb;

	/////////////////////////////////////////////////////////////////////////////////////
	// ITriEffectParameter
	/////////////////////////////////////////////////////////////////////////////////////

	size_t GetValueSize() const;

	void CopyValueToEffect(	Tr2RenderContextEnum::ShaderType inputType, 
							unsigned char* destHandle, 
							size_t size,
							Tr2RenderContext &renderContext ) const;

	const char* GetParameterName() const;
	virtual bool IsZeroOrNull( void ) const;
	void RebuildEffectHandles( ITr2ShaderState* effectRes );

	/////////////////////////////////////////////////////////////////////////////////////
	// INotify
	/////////////////////////////////////////////////////////////////////////////////////

	bool OnModified(
		Be::Var* val
		);

	/////////////////////////////////////////////////////////////////////////////////////
	// IInitialize
	/////////////////////////////////////////////////////////////////////////////////////
	bool Initialize() { return true; };
	// This is just here to prevent us from getting an "on modified" call while blue is still reading the member table

private:
	ITr2ShaderStatePtr m_cachedEffect;
};
TYPEDEF_BLUECLASS(TriVector4Parameter);

#endif 
