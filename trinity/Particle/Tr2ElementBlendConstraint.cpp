////////////////////////////////////////////////////////////
//
//    Created:   October 2012
//    Copyright: CCP 2012
//

#include "StdAfx.h"
#include "Tr2ElementBlendConstraint.h"
#include "Tr2ParticleSystem.h"

// --------------------------------------------------------------------------------------
// Description:
//   Tr2ElementBlendConstraint default constructor
// --------------------------------------------------------------------------------------
Tr2ElementBlendConstraint::Tr2ElementBlendConstraint( IRoot* lockobj )
	:m_isValid( false ),
	m_name( Tr2ParticleElementDeclarationName::CUSTOM ),
	m_value( 0.f, 0.f, 0.f, 0.f ),
	m_originalFactor( 1.f ),
	m_element()
{
}

// --------------------------------------------------------------------------------------
// Description:
//   Implements ITr2GenericParticleConstraint interface. Applies the constraint to the
//   particle. Blends particle element value with the constant value.
// Arguments:
//   arguments - Child emitters update arguments
//   particles - Particle data buffers
//   strides - Sizes of particle data in each of "particles" arrays (in floats).
//   count - Number of particles.
//   dt - (unused) Frame time
// --------------------------------------------------------------------------------------
void Tr2ElementBlendConstraint::ApplyConstraint( const ITr2GenericEmitter::UpdateArguments& arguments, float** particles, unsigned* strides, unsigned count, float dt )
{
	if( !m_isValid )
	{
		return;
	}
	Tr2ParticleStreamIterator<float> data( particles, strides, m_element );
	XMVECTOR originalFactor = XMVectorReplicate( m_originalFactor );
	XMVECTOR value = m_value;
	if( m_element.m_dimension == 1 )
	{
		for( unsigned i = 0; i < count; ++i, ++data )
		{
			*data = *data * m_originalFactor + m_value.x;
		}
	}
	else if( data.Is16ByteAligned() )
	{
		switch( m_element.m_dimension )
		{
		case 2:
			for( unsigned i = 0; i < count; ++i, ++data )
			{
				XMStoreFloat2( 
					(XMFLOAT2*)data.Get(), 
					XMVectorMultiplyAdd( XMLoadFloat2( (XMFLOAT2*)data.Get() ), originalFactor, value ) );
			}
			break;
		case 3:
			for( unsigned i = 0; i < count; ++i, ++data )
			{
				XMStoreFloat3( 
					(XMFLOAT3*)data.Get(), 
					XMVectorMultiplyAdd( XMLoadFloat3( (XMFLOAT3*)data.Get() ), originalFactor, value ) );
			}
			break;
		case 4:
			for( unsigned i = 0; i < count; ++i, ++data )
			{
				XMStoreFloat4( 
					(XMFLOAT4*)data.Get(), 
					XMVectorMultiplyAdd( XMLoadFloat4( (XMFLOAT4*)data.Get() ), originalFactor, value ) );
			}
			break;
		}
	}
	else
	{
		switch( m_element.m_dimension )
		{
		case 2:
			for( unsigned i = 0; i < count; ++i, ++data )
			{
				XMStoreFloat2( 
					(XMFLOAT2*)data.Get(), 
					XMVectorMultiplyAdd( XMLoadFloat2( (XMFLOAT2*)data.Get() ), originalFactor, value ) );
			}
			break;
		case 3:
			for( unsigned i = 0; i < count; ++i, ++data )
			{
				XMStoreFloat3( 
					(XMFLOAT3*)data.Get(), 
					XMVectorMultiplyAdd( XMLoadFloat3( (XMFLOAT3*)data.Get() ), originalFactor, value ) );
			}
			break;
		case 4:
			for( unsigned i = 0; i < count; ++i, ++data )
			{
				XMStoreFloat4( 
					(XMFLOAT4*)data.Get(), 
					XMVectorMultiplyAdd( XMLoadFloat4( (XMFLOAT4*)data.Get() ), originalFactor, value ) );
			}
			break;
		}
	}
}

// --------------------------------------------------------------------------------------
// Description:
//   Implements ITr2GenericParticleConstraint interface. Binds the constraint with the
//   provided particle system. Looks up a particle element with the given type/name in
//   the particle system.
// Arguments:
//   system - Particle system that owns the constraint
// --------------------------------------------------------------------------------------
void Tr2ElementBlendConstraint::Bind( Tr2ParticleSystem* system )
{
	m_isValid = false;

	const Tr2ParticleElementDataMap &declaration = system->GetElementDeclaration();
	auto i = declaration.find( m_name );
	if( i == declaration.end() )
	{
		CCP_LOGERR( "Tr2BlendElementConstraint element \"%s\" not found in the particle system", m_name.GetName().c_str() );
	}
	else
	{
		m_element = i->second;
		m_isValid = true;
	}
}
