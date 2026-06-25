// Copyright © 2023 CCP ehf.

#include "StdAfx.h"
#include "EveCameraFxAttributes.h"
#include "Tr2Renderer.h"

EveCameraFxAttributes::EveCameraFxAttributes( IRoot* lockobj ) :
	m_distanceToCamera( 0 ),
	m_lookAngleToObject( 0 ),
	m_objectRotation( 0, 0, 0 ),
	m_rotationWithChildTransform( 0, 0, 0 ),
	m_cameraRotation( 0, 0, 0 )
{
}

EveCameraFxAttributes::~EveCameraFxAttributes()
{
}

void EveCameraFxAttributes::UpdateAsyncronous( const EveUpdateContext& updateContext, const EveChildUpdateParams& params )
{
	// gather attributes
	Vector3 objPos( 0.0, 0.0, 0.0 );

	if( nullptr != params.spaceObjectParent )
	{
		params.spaceObjectParent->GetModelCenterWorldPosition( objPos );
	}

	if( nullptr != params.childParent )
	{
		Matrix localToWorldTransform;
		params.childParent->GetLocalToWorldTransform( localToWorldTransform );
		m_rotationWithChildTransform = Vector3( localToWorldTransform._13, localToWorldTransform._23, localToWorldTransform._33 );
		objPos = localToWorldTransform.GetTranslation();
	}

	const Vector3 camPos = Tr2Renderer::GetViewPosition();
	const Vector3 vec2obj = objPos - camPos;
	const Matrix view = Tr2Renderer::GetViewTransform();

	m_distanceToCamera = Length( vec2obj );
	m_lookAngleToObject = -( Dot( Tr2Renderer::GetViewLookAt(), vec2obj ) / m_distanceToCamera );



	m_objectRotation = Vector3( params.localToWorldTransform._13, params.localToWorldTransform._23, params.localToWorldTransform._33 );
	m_cameraRotation = Vector3( view._13, view._23, view._33 );
}