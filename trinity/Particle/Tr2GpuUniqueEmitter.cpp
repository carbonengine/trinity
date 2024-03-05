////////////////////////////////////////////////////////////
//
//    Created:   October 2015
//    Copyright: CCP 2015
//

#include "StdAfx.h"
#include "Tr2GpuUniqueEmitter.h"

Tr2GpuUniqueEmitter::Tr2GpuUniqueEmitter( IRoot* lockObj )
	:Tr2GpuSharedEmitter( lockObj ),
	m_attractorPosition( 0.f, 0.f, 0.f ),
	m_scaledByParent( false )
{
	m_id = uintptr_t( this ) | uintptr_t( CcpGetTimestamp() ) | ( 1 << ( sizeof( uintptr_t ) - 1 ) );
}

void Tr2GpuUniqueEmitter::GenerateID()
{
}

void Tr2GpuUniqueEmitter::Update( const UpdateArguments& arguments )
{
	Tr2GpuParticleSystem::Emitter originalEmitter;
	Tr2GpuParticleSystem::EmitterParams originalParams;
	if( m_params.attractorStrength != 0.f || m_scaledByParent )
	{
		if( m_params.attractorStrength != 0.f )
		{
			m_params.attractorPosition = XMVector3TransformCoord( m_attractorPosition, arguments.parentTransform );
			m_params.attractorPosition -= arguments.originShift;
		}
		if( m_scaledByParent )
		{
			originalEmitter = m_emitter;
			originalParams = m_params;

			XMVECTOR scale, rotation, translation;
			XMMatrixDecompose( &scale, &rotation, &translation, arguments.parentTransform );
			float uniform = XMVectorGetX( XMVector3Dot( scale, XMVectorReplicate( 1.0f / 3.0f ) ) );
			m_params.sizes *= uniform;
			m_params.gravity *= uniform;
			m_params.turbulenceAmplitude *= uniform;
			m_params.attractorStrength *= uniform;
			m_emitter.radius *= uniform;
			m_emitter.maxSpeed *= uniform;
			m_emitter.minSpeed *= uniform;
		}
		UpdateHash();
	}
	Tr2GpuSharedEmitter::Update( arguments );
	if( m_scaledByParent )
	{
		m_emitter = originalEmitter;
		m_params = originalParams;
	}
}

void Tr2GpuUniqueEmitter::SpawnParticles( const UpdateArguments& arguments,
								const Vector3* position, 
								const Vector3* velocity, 
								float rateModifier )
{
	Tr2GpuParticleSystem::Emitter originalEmitter;
	Tr2GpuParticleSystem::EmitterParams originalParams;
	if( m_scaledByParent )
	{
		originalEmitter = m_emitter;
		originalParams = m_params;

		XMVECTOR scale, rotation, translation;
		XMMatrixDecompose( &scale, &rotation, &translation, arguments.parentTransform );
		float uniform = XMVectorGetX( XMVector3Dot( scale, XMVectorReplicate( 1.0f / 3.0f ) ) );
		m_params.sizes *= uniform;
		m_params.gravity *= uniform;
		m_params.turbulenceAmplitude *= uniform;
		m_params.attractorStrength *= uniform;
		m_emitter.radius *= uniform;
		m_emitter.maxSpeed *= uniform;
		m_emitter.minSpeed *= uniform;
		UpdateHash();
	}
	Tr2GpuSharedEmitter::SpawnParticles( arguments, position, velocity, rateModifier );
	if( m_scaledByParent )
	{
		m_emitter = originalEmitter;
		m_params = originalParams;
	}
}


void Tr2GpuUniqueEmitter::SpawnParticles( const UpdateArguments& arguments,
								const Vector3 *positionStart, const Vector3 *positionEnd,
								const Vector3 *velocityStart, const Vector3 *velocityEnd,
								float deltaTime )
{
	Tr2GpuParticleSystem::Emitter originalEmitter;
	Tr2GpuParticleSystem::EmitterParams originalParams;
	if( m_scaledByParent )
	{
		originalEmitter = m_emitter;
		originalParams = m_params;

		XMVECTOR scale, rotation, translation;
		XMMatrixDecompose( &scale, &rotation, &translation, arguments.parentTransform );
		float uniform = XMVectorGetX( XMVector3Dot( scale, XMVectorReplicate( 1.0f / 3.0f ) ) );
		m_params.sizes *= uniform;
		m_params.gravity *= uniform;
		m_params.turbulenceAmplitude *= uniform;
		m_params.attractorStrength *= uniform;
		m_emitter.radius *= uniform;
		m_emitter.maxSpeed *= uniform;
		m_emitter.minSpeed *= uniform;
		UpdateHash();
	}
	Tr2GpuSharedEmitter::SpawnParticles( arguments, positionStart, positionEnd, velocityStart, velocityEnd, deltaTime );
	if( m_scaledByParent )
	{
		m_emitter = originalEmitter;
		m_params = originalParams;
	}
}
