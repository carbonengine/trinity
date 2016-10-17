#include "StdAfx.h"
#include "EveChildTransform.h"

EveChildTransform::EveChildTransform() :
	m_scaling( 1, 1, 1 ),
	m_rotation( 0, 0, 0, 1 ),
	m_translation( 0, 0, 0 ),
	m_useSRT( true ),
	m_staticTransform( false ),
	m_useStaticRotation( false )
{
	D3DXMatrixIdentity( &m_localTransform );
	D3DXMatrixIdentity( &m_worldTransform );
}

void EveChildTransform::RebuildLocalTransform()
{
	if( m_useSRT )
	{
		D3DXMatrixTransformation( &m_localTransform, nullptr, nullptr, &m_scaling, nullptr, &m_rotation, &m_translation );
	}
}

void EveChildTransform::Setup( const Vector3* scale, const Quaternion* rotation, const Vector3* translation, Tr2Lod lowestLodVisible )
{
	if( m_useSRT )
	{
		if( scale )
		{
			m_scaling = *scale;
		}
		if( rotation )
		{
			m_rotation = *rotation;
		}
		if( translation )
		{
			m_translation = *translation;
		}
		D3DXMatrixTransformation( &m_localTransform, nullptr, nullptr, &m_scaling, nullptr, &m_rotation, &m_translation);
	}
}

void EveChildTransform::SetupWithStaticRotation( const Vector3* scale, const Quaternion* rotation, const Vector3* translation, Tr2Lod lowestLodVisible )
{
	m_useStaticRotation = true;
	Setup( scale, rotation, translation, lowestLodVisible );
}

void EveChildTransform::UpdateTransform( const Matrix& parentTransform ) 
{
	if( m_staticTransform || !m_useSRT )
	{
		D3DXMatrixMultiply( &m_worldTransform, &m_localTransform, &parentTransform );
	}
	else
	{
		D3DXMatrixTransformation( &m_localTransform, nullptr, nullptr, &m_scaling, nullptr, &m_rotation, &m_translation );
		if( !m_useStaticRotation )
		{
			D3DXMatrixMultiply( &m_worldTransform, &m_localTransform, &parentTransform );
		}
		else
		{
			// Take out the rotation
			Vector3 scale, translation;
			Quaternion rotation;
			Matrix parentTransformWithoutRotation;

			Quaternion identityRotation = Quaternion( 0, 0, 0, 1 );
			Vector3 zero = Vector3( 0, 0, 0 );

			D3DXMatrixDecompose( &scale, &rotation, &translation, &parentTransform );
			D3DXMatrixTransformation( &parentTransformWithoutRotation, &zero, &identityRotation, &scale, &zero, &identityRotation, &translation );
						
			D3DXMatrixMultiply( &m_worldTransform, &m_localTransform, &parentTransformWithoutRotation );
		}
	}
}