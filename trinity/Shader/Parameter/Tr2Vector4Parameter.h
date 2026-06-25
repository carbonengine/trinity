// Copyright © 2023 CCP ehf.

#ifndef TR2VECTOR4PARAMETER_H
#define TR2VECTOR4PARAMETER_H


#include "include/ITriEffectParameter.h"
#include "ITriReroutable.h"

BLUE_DECLARE( Tr2Vector4Parameter );
BLUE_CLASS_ALLOW_DELAYED_DELETE( Tr2Vector4Parameter );

BLUE_CLASS( Tr2Vector4Parameter ) :
	public ITriEffectParameter,
	public ITriReroutable,
	public IInitialize
{

public:
	EXPOSE_TO_BLUE();

	Tr2Vector4Parameter( IRoot* lockobj = NULL );

	using ITriEffectParameter::Lock;
	using ITriEffectParameter::Unlock;

	Vector4 m_value;
	Vector4 m_linearValue;
	BlueSharedString m_name;

	bool m_isUsedByEffect;
	bool m_isSrgb;

	const Vector4& GetValue( void );
	void SetValue( const Vector4& value );

	/////////////////////////////////////////////////////////////////////////////////////
	// ITriEffectParameter

	void CopyValueToEffect( Tr2RenderContextEnum::ShaderType inputType,
							unsigned char* destHandle,
							size_t size,
							Tr2RenderContext& renderContext ) const;
	const char* GetParameterName() const;
	void RebuildEffectHandles( Tr2Shader * effectRes );
	unsigned GetHashValue( unsigned startingHash ) const;

	/////////////////////////////////////////////////////////////////////////////////////
	// ITriReroutable
	void SetDestination( void* dest, size_t size );
	void GetDestination( void*& dest, size_t& size );
	void RegisterBinding( TriValueBinding * vb );
	void UnregisterBinding( TriValueBinding * vb );
	bool IsRerouted() const;

	//////////////////////////////////////////////////////////////////////////
	// IInitialize
	bool Initialize();

private:
	// Utility getters
	float GetX( void );
	float GetY( void );
	float GetZ( void );
	float GetW( void );

	// Utility setters
	void SetX( float x );
	void SetY( float y );
	void SetZ( float z );
	void SetW( float w );

private:
	// If this parameter is bound to a curve we have to inform the binding of the
	// destination value the effect sets when the parameters are mapped to shader
	// constants.
	typedef TrackableStdVector<TriValueBinding*> BindingVector_t;
	BindingVector_t m_bindings;
	float* m_reroutedValue;
};

TYPEDEF_BLUECLASS( Tr2Vector4Parameter );

#endif
