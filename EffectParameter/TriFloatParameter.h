/* 
	*************************************************************************

	TriFloatParameter.h

	Created:   March 2006
	OS:        Win32
	Project:   Trinity

	Description:   
		
		a float parameter for effects

	Dependencies:

		DirectX 9.0, Probably more, ytbd.

	(c) CCP 2006

	*************************************************************************
*/

#ifndef _TRIFLOATPARAMETER_H_
#define _TRIFLOATPARAMETER_H_

#include "include/ITriEffectParameter.h"

BLUE_DECLARE_INTERFACE( ITr2ShaderState );
BLUE_DECLARE_INTERFACE( ITriScalarFunction );

#define TRIFLOATPARAMETER_Description "TriFloatParameter"

class TriFloatParameter:
	public ITriEffectParameter,
	public IInitialize,
	public INotify
{

public:

	EXPOSE_TO_BLUE();

	TriFloatParameter(IRoot* lockobj = NULL);
	~TriFloatParameter();

	using ITriEffectParameter::Lock;
	using ITriEffectParameter::Unlock;

	float m_value;
	BlueSharedString m_name;

	bool m_isUsedByEffect;


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


protected:
	ITr2ShaderStatePtr m_cachedEffect;
};

BLUE_CLASS_ALLOW_DELAYED_DELETE( TriFloatParameter );
TYPEDEF_BLUECLASS(TriFloatParameter);



// Note - this class is deprecated - see http://core/wiki/Trinity_Animation_Curves
// for a discussion on how to animate parameters.
class TriAniFloatParameter:
	public TriFloatParameter
{
public:
	EXPOSE_TO_BLUE();

	TriAniFloatParameter(IRoot* lockobj = NULL);
	~TriAniFloatParameter();

	// NB: this class should really be linked to a SEPERATE float entity...
	// I'm going with the easy implementation to start with
	ITriScalarFunctionPtr mValueCurve;
};
TYPEDEF_BLUECLASS(TriAniFloatParameter);
#endif 
