#ifndef TRIVECTOR4ARRAYPARAMETER_H_
#define TRIVECTOR4ARRAYPARAMETER_H_

#include "include/ITriEffectParameter.h"

BLUE_DECLARE( Tr2Shader );

//--------------------------------------------------------------------------------------------------
// vector4 - is content of the parameter array, TODO: make it more flexible content
//
BLUE_CLASS( TriVector4 ) : public IRoot
{
public:
	EXPOSE_TO_BLUE();

	TriVector4( IRoot* lockobj = 0 );
	~TriVector4();

    using IRoot::Lock;
    using IRoot::Unlock;

	// the actuall data
	Vector4 m_data;
};
TYPEDEF_BLUECLASS( TriVector4 );
BLUE_DECLARE_VECTOR( TriVector4 );

//--------------------------------------------------------------------------------------------------
// float array parameter
//
BLUE_CLASS( TriFloatArrayParameter ):
	public ITriEffectParameter,
	public INotify,
	public IInitialize
{
public:
	EXPOSE_TO_BLUE();

	TriFloatArrayParameter(IRoot* lockobj = NULL);
	~TriFloatArrayParameter();

	using ITriEffectParameter::Lock;
	using ITriEffectParameter::Unlock;

	// data
	PTriVector4Vector m_value;
	BlueSharedString m_name;

	bool m_isUsedByEffect;

	/////////////////////////////////////////////////////////////////////////////////////
	// ITriEffectParameter
	void CopyValueToEffect(	Tr2RenderContextEnum::ShaderType inputType, 
							unsigned char* destHandle, 
							size_t size,
							Tr2RenderContext &renderContext ) const;
	const char* GetParameterName() const;
	void RebuildEffectHandles( Tr2Shader* effectRes );
	unsigned GetHashValue( unsigned startingHash ) const;

	/////////////////////////////////////////////////////////////////////////////////////
	// INotify
	bool OnModified( Be::Var* val );

	/////////////////////////////////////////////////////////////////////////////////////
	// IInitialize
	bool Initialize() { return true; };
	// This is just here to prevent us from getting an "on modified" call while blue is still reading the member table

private:
	Tr2ShaderPtr m_cachedEffect;
};

BLUE_CLASS_ALLOW_DELAYED_DELETE( TriFloatArrayParameter );
TYPEDEF_BLUECLASS( TriFloatArrayParameter );

#endif 
