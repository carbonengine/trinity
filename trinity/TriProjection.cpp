// Copyright © 2023 CCP ehf.

#include "StdAfx.h"
#include "TriProjection.h"
#include "Tr2Renderer.h"

TriProjection::TriProjection( IRoot* lockobj ) :
	m_projectionType( 0 ),
	m_fov( 0 ),
	m_aspect( 0 ),
	m_zn( 0 ),
	m_zf( 0 ),
	m_left( 0 ),
	m_top( 0 ),
	m_right( 0 ),
	m_bottom( 0 )
{
}

TriProjection::~TriProjection( void )
{
}

void TriProjection::PerspectiveFov( float fov, float aspect, float zn, float zf )
{
	m_projectionType = TRIPROJECTION_FOV;
	m_fov = fov;
	m_aspect = aspect;
	m_zn = zn;
	m_zf = zf;
}

void TriProjection::PerspectiveOffCenter( float left, float right, float bottom, float top, float zn, float zf )
{
	m_projectionType = TRIPROJECTION_OFF_CENTER;
	m_left = left;
	m_right = right;
	m_top = top;
	m_bottom = bottom;
	m_zn = zn;
	m_zf = zf;
}

void TriProjection::PerspectiveOrthographic( float width, float height, float front, float back )
{
	m_projectionType = TRIPROJECTION_ORTHO;
	m_left = width;
	m_top = height;
	m_zn = front;
	m_zf = back;
}

// -------------------------------------------------------------
// Description:
//   Sets this projection to a custom projection from a matrix
// Arguments:
//   mat - the projection transform matrix to be used
// -------------------------------------------------------------
void TriProjection::CustomProjection( const Matrix& mat )
{
	m_projectionType = TRIPROJECTION_CUSTOM;
	m_customTransform = mat;
}

int TriProjection::GetProjectionType()
{
	// Casting to int for Python exposure
	return int( m_projectionType );
}

void TriProjection::SetProjection( Tr2RenderContext& renderContext )
{
	Matrix viewport2projectionAdjustment = IdentityMatrix();
	auto& viewport = renderContext.m_esm.GetViewport();
	auto& deviceViewport = renderContext.m_esm.GetDeviceViewport();

	// In case we shrunk the viewport we need to scale and/or offset the
	// projection matrix
	viewport2projectionAdjustment._11 = viewport.width / deviceViewport.m_width;
	viewport2projectionAdjustment._22 = viewport.height / deviceViewport.m_height;
	viewport2projectionAdjustment._31 = ( viewport.width - deviceViewport.m_width ) / deviceViewport.m_width;
	viewport2projectionAdjustment._32 = ( viewport.height - deviceViewport.m_height ) / deviceViewport.m_height;
	if( viewport.x < 0 )
	{
		viewport2projectionAdjustment._31 *= -1;
	}

	if( viewport.y + viewport.height > int( renderContext.m_esm.GetRenderTargetHeight() ) )
	{
		viewport2projectionAdjustment._32 *= -1;
	}


	switch( m_projectionType )
	{
	case TRIPROJECTION_FOV:
		Tr2Renderer::SetPerspectiveProjection( m_fov, m_zn, m_zf, m_aspect, viewport2projectionAdjustment );
		break;
	case TRIPROJECTION_OFF_CENTER:
		Tr2Renderer::SetPerspectiveProjection( m_left, m_right, m_bottom, m_top, m_zn, m_zf, viewport2projectionAdjustment );
		break;
	case TRIPROJECTION_ORTHO:
		Tr2Renderer::SetOrthoProjection( m_left, m_top, m_zn, m_zf, viewport2projectionAdjustment );
		break;
	case TRIPROJECTION_CUSTOM:
		Tr2Renderer::SetProjectionTransform( m_customTransform, viewport2projectionAdjustment );
		break;
	}
}

void TriProjection::SetProjection()
{
	switch( m_projectionType )
	{
	case TRIPROJECTION_FOV:
		Tr2Renderer::SetPerspectiveProjection( m_fov, m_zn, m_zf, m_aspect );
		break;
	case TRIPROJECTION_OFF_CENTER:
		Tr2Renderer::SetPerspectiveProjection( m_left, m_right, m_bottom, m_top, m_zn, m_zf );
		break;
	case TRIPROJECTION_ORTHO:
		Tr2Renderer::SetOrthoProjection( m_left, m_top, m_zn, m_zf );
		break;
	case TRIPROJECTION_CUSTOM:
		Tr2Renderer::SetProjectionTransform( m_customTransform );
		break;
	}
}

void TriProjection::GetMatrixWithoutViewAdjustment( Matrix& proj ) const
{
	// This constructs the projection matrix. Note that this is not necessarily the
	// same matrix that SetProjection would set to the device. That is because
	// Tr2Renderer::SetPerspectiveProjection may skew/offset the projection if the
	// viewport is partly off screen. <bjorns 2009-03-26>
	switch( m_projectionType )
	{
	case TRIPROJECTION_FOV:
		proj = PerspectiveFovMatrix( m_fov, m_aspect, m_zn, m_zf );
		break;
	case TRIPROJECTION_OFF_CENTER:
		proj = PerspectiveOffCenterMatrix( m_left, m_right, m_bottom, m_top, m_zn, m_zf );
		break;
	case TRIPROJECTION_ORTHO:
		proj = OrthoMatrix( m_left, m_top, m_zn, m_zf );
		break;
	case TRIPROJECTION_CUSTOM:
		proj = m_customTransform;
		break;
	}
}

Matrix TriProjection::GetTransform() const
{
	Matrix proj;
	GetMatrixWithoutViewAdjustment( proj );
	return proj;
}
