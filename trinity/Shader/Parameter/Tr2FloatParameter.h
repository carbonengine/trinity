#ifndef TR2FLOATPARAMETER_H
#define TR2FLOATPARAMETER_H

#include "include/ITriEffectParameter.h"
#include "ITriReroutable.h"

BLUE_DECLARE( Tr2FloatParameter );
BLUE_CLASS_ALLOW_DELAYED_DELETE( Tr2FloatParameter );

BLUE_CLASS( Tr2FloatParameter ):
	public ITriEffectParameter,
	public ITriReroutable,
	public IInitialize
{

public:
	EXPOSE_TO_BLUE();

	Tr2FloatParameter(IRoot* lockobj = NULL);
	~Tr2FloatParameter();

	using ITriEffectParameter::Lock;
	using ITriEffectParameter::Unlock;


	/////////////////////////////////////////////////////////////////////////////////////
	// ITriEffectParameter

	void CopyValueToEffect(		Tr2RenderContextEnum::ShaderType inputType, 
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

	//////////////////////////////////////////////////////////////////////////
	// IInitialize
	bool Initialize();

	BlueSharedString m_name;
	float m_value;
protected:
	// If this parameter is bound to a curve we have to inform the binding of the
	// destination value the effect sets when the parameters are mapped to shader
	// constants.
	typedef TrackableStdVector<TriValueBinding*> BindingVector_t;
	BindingVector_t m_bindings;
	float* m_reroutedValue;

	bool m_isUsedByEffect;
public:
	float GetValue();
	void SetValue( float val );
};
TYPEDEF_BLUECLASS( Tr2FloatParameter );

#endif 
