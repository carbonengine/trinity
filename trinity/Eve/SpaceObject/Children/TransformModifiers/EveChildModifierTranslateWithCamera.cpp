////////////////////////////////////////////////////////////
//
//    Created:   2018
//    Copyright: CCP 2018
//
#include "StdAfx.h"
#include "EveChildModifierTranslateWithCamera.h"
#include "Tr2Renderer.h"

EveChildModifierTranslateWithCamera::EveChildModifierTranslateWithCamera( IRoot* lockobj ):
	m_attachedToCamera( false )
{
}

EveChildModifierTranslateWithCamera::~EveChildModifierTranslateWithCamera()
{
}

Matrix EveChildModifierTranslateWithCamera::ApplyTransform( const Matrix& transform, size_t, const granny_matrix_3x4* ) const
{
	Matrix result = transform;
	if( m_attachedToCamera ) 
	{
		result.GetTranslation() = Tr2Renderer::GetViewPosition();
	}
	else
	{
		result.GetTranslation() += Tr2Renderer::GetViewPosition();
	}
	return result;
}