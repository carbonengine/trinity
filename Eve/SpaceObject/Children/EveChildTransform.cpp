#include "StdAfx.h"
#include "EveChildTransform.h"

EveChildTransform::EveChildTransform() :
	m_scaling( 1, 1, 1 ),
	m_rotation( 0, 0, 0, 1 ),
	m_translation( 0, 0, 0 ),
	m_useSRT( true ),
	m_staticTransform( false )
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
	Matrix temp;
	if( m_useSRT )
	{
		D3DXMatrixTransformation( &m_localTransform, nullptr, nullptr, &m_scaling, nullptr, &m_rotation, &m_translation );
	}
	D3DXMatrixTransformation( &temp, nullptr, nullptr, scale, nullptr, rotation, translation );
	D3DXMatrixMultiply( &m_localTransform, &m_localTransform, &temp );
	if( m_useSRT )
	{
		D3DXMatrixDecompose( &m_scaling, &m_rotation, &m_translation, &m_localTransform );
	}
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
		D3DXMatrixMultiply( &m_worldTransform, &m_localTransform, &parentTransform );
	}
}