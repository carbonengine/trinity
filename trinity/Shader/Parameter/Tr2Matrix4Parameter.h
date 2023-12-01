#ifndef Tr2Matrix4Parameter_H
#define Tr2Matrix4Parameter_H


#include "include/ITriEffectParameter.h"
#include "ITriReroutable.h"

BLUE_DECLARE( Tr2Matrix4Parameter );
BLUE_CLASS_ALLOW_DELAYED_DELETE( Tr2Matrix4Parameter );

// -----------------------------------------------------------------------------------
// Description:
//   Support class for Tr2Effect PopulateParameters, that can map a hlsl float4x4
//   to a bluetype Matrix.
// SeeAlso:
//   Tr2Effect
// -----------------------------------------------------------------------------------
BLUE_CLASS( Tr2Matrix4Parameter ):
	public ITriEffectParameter,
	public ITriReroutable
{

public:
	EXPOSE_TO_BLUE();

	Tr2Matrix4Parameter(IRoot* lockobj = NULL);

	using ITriEffectParameter::Lock;
	using ITriEffectParameter::Unlock;

	// 4x4 matrix mapping hlsl float4x4
	Matrix  m_value;

	// hlsl attribute name
	BlueSharedString m_name;

	// flag set when this attribute is actually used by the current effect
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
	// ITriReroutable
	void SetDestination( void* dest, size_t size );
	void GetDestination( void*& dest, size_t& size );
	void RegisterBinding( TriValueBinding* vb );
	void UnregisterBinding( TriValueBinding* vb );
	bool IsRerouted() const;

	Matrix	GetValue();
	void	SetValue( const Matrix & );

private:
	// If this parameter is bound to a curve we have to inform the binding of the
	// destination value the effect sets when the parameters are mapped to shader
	// constants.
	typedef TrackableStdVector<TriValueBinding*> BindingVector_t;
	BindingVector_t m_bindings;
	float* m_reroutedValue;

};

TYPEDEF_BLUECLASS( Tr2Matrix4Parameter );

#endif	// Tr2Matrix4Parameter_H_

