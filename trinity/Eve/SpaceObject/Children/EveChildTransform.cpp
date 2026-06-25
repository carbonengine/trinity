// Copyright © 2023 CCP ehf.

#include "StdAfx.h"
#include "EveChildTransform.h"

EveChildTransform::EveChildTransform() :
	m_scaling( 1, 1, 1 ),
	m_rotation( 0, 0, 0, 1 ),
	m_translation( 0, 0, 0 ),
	m_useSRT( true ),
	m_staticTransform( false ),
	m_useStaticRotation( false ),
	m_useStaticScale( false ),
	m_localTransform( IdentityMatrix() ),
	m_worldTransform( IdentityMatrix() )
{
}

void EveChildTransform::RebuildLocalTransform()
{
	if( m_useSRT )
	{
		m_localTransform = TransformationMatrix( m_scaling, m_rotation, m_translation );
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
		m_localTransform = TransformationMatrix( m_scaling, m_rotation, m_translation );
	}
}

void EveChildTransform::SetupWithStaticRotation( const Vector3* scale, const Quaternion* rotation, const Vector3* translation, Tr2Lod lowestLodVisible )
{
	m_useStaticRotation = true;
	Setup( scale, rotation, translation, lowestLodVisible );
}

void EveChildTransform::SetupWithStaticTransform( const Vector3* scale, const Quaternion* rotation, const Vector3* translation, Tr2Lod lowestLodVisible )
{
	m_staticTransform = true;
	Setup( scale, rotation, translation, lowestLodVisible );
}

void EveChildTransform::UpdateTransform( const Matrix& parentTransform )
{
	if( m_staticTransform || !m_useSRT )
	{
		m_worldTransform = m_localTransform * parentTransform;
	}
	else
	{
		m_localTransform = TransformationMatrix( m_scaling, m_rotation, m_translation );

		if( !m_useStaticRotation && !m_useStaticScale )
		{
			m_worldTransform = m_localTransform * parentTransform;
			return;
		}

		Vector3 scale, translation;
		Quaternion rotation;
		Matrix modifiedParentTransform;

		Decompose( scale, rotation, translation, parentTransform );

		if( m_useStaticScale )
		{
			scale = Vector3( 1.f, 1.f, 1.f );
		}

		if( m_useStaticRotation )
		{
			// rotation is static and scale might be
			modifiedParentTransform = ScalingMatrix( scale ) * TranslationMatrix( translation );
			m_worldTransform = m_localTransform * modifiedParentTransform;
		}
		else
		{
			// rotation is not static but scale is
			modifiedParentTransform = RotationMatrix( rotation ) * TranslationMatrix( translation );
			m_worldTransform = m_localTransform * modifiedParentTransform;
		}
	}
}