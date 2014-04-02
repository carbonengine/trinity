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

	size_t GetValueSize() const;
	void CopyValueToEffect(		Tr2RenderContextEnum::ShaderType inputType, 
								unsigned char* destHandle, 
								size_t size,
								Tr2RenderContext &renderContext ) const;
	const char* GetParameterName() const;
	virtual bool IsZeroOrNull( void ) const;
	void RebuildEffectHandles( ITr2ShaderState* effectRes );

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

	float m_value;
	BlueSharedString m_name;

protected:
	bool m_isUsedByEffect;
	bool m_allowRerouting;

	// If this parameter is bound to a curve we have to inform the binding of the
	// destination value the effect sets when the parameters are mapped to shader
	// constants.
	TriValueBinding* m_binding;
	float* m_reroutedValue;

public:
	float GetValue();
	void SetValue( float val );
};
TYPEDEF_BLUECLASS( Tr2FloatParameter );

#endif 
