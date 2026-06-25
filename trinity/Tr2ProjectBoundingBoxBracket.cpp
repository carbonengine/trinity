// Copyright © 2011 CCP ehf.

#include "StdAfx.h"

#include "Tr2ProjectBoundingBoxBracket.h"
#include "include/ITr2BoundingBox.h"
#include "Tr2Renderer.h"
#include "TriViewport.h"
#include "Sprite2d/Tr2Sprite2dContainer.h"
#include "Utilities/BoundingBox.h"
#include "include/ITr2DebugRenderer.h"

extern ITr2DebugRendererPtr g_debugRenderer;


Tr2ProjectBoundingBoxBracket::Tr2ProjectBoundingBoxBracket( IRoot* lockobj /*= NULL */ ) :
	m_minProjectedWidth( 0.0f ),
	m_minProjectedHeight( 0.0f ),
	m_maxProjectedWidth( 0.0f ),
	m_maxProjectedHeight( 0.0f ),
	m_projectedX( 0.0f ),
	m_projectedY( 0.0f ),
	m_projectedZ( 0.0f ),
	m_projectedWidth( 0.0f ),
	m_projectedHeight( 0.0f ),
	m_integerCoordinates( true ),
	m_screenMargin( 32.0f ),
	m_cameraDistance( 0 )
{
}


void Tr2ProjectBoundingBoxBracket::UpdateValue( double time )
{
	if( !m_object )
	{
		return;
	}

	if( !m_object->IsBoundingBoxReady() )
	{
		return;
	}

	Vector3 bbMin, bbMax;
	if( !m_object->GetWorldBoundingBox( bbMin, bbMax ) )
	{
		return;
	}

	Vector3 expansion( 0.5f, 0.5f, 0.5f );
	Vector3 expandedMin = bbMin - expansion;
	Vector3 expandedMax = bbMax + expansion;
	if( BoundingBoxIsInside( expandedMin, expandedMax, Tr2Renderer::GetViewPosition() ) )
	{
		// Camera is inside bounding box - can't do any sensible projection
		SetEmptyProjection();
		return;
	}

	Vector3 center = ( bbMax + bbMin ) * 0.5f;
	Vector3 projectedCenter;
	Matrix viewProj;
	projectedCenter = TransformCoord( center, Tr2Renderer::GetViewTransform() );
	projectedCenter = TransformCoord( projectedCenter, Tr2Renderer::GetProjectionTransform() );
	Vec3TransformByViewport( projectedCenter, Tr2Renderer::GetViewport() );
	if( projectedCenter.z <= 0.0f || projectedCenter.z >= 1.0f )
	{
		SetEmptyProjection();
		return;
	}

	Vector3 d = Tr2Renderer::GetViewPosition() - center;
	m_cameraDistance = Length( d );

	BoundingBoxProject(
		bbMin,
		bbMax,
		Tr2Renderer::GetViewTransform(),
		Tr2Renderer::GetProjectionTransform(),
		Tr2Renderer::GetViewport() );

	if( bbMin.z <= 0.0f || bbMax.z >= 1.0f )
	{
		SetEmptyProjection();
		return;
	}

	m_projectedZ = std::min( bbMin.z, bbMax.z );

	unsigned int screenWidth;
	unsigned int screenHeight;
	Tr2Renderer::GetBackBufferDimensions( screenWidth, screenHeight );

	if( ( bbMin.x > screenWidth ) || ( bbMax.x < 0.0f ) || ( bbMin.y > screenHeight ) || ( bbMax.y < 0.0f ) )
	{
		SetEmptyProjection();
		return;
	}

	bbMin.x = std::max( bbMin.x, m_screenMargin );
	bbMax.x = std::min( bbMax.x, (float)screenWidth - m_screenMargin );

	bbMin.y = std::max( bbMin.y, m_screenMargin );
	bbMax.y = std::min( bbMax.y, (float)screenHeight - m_screenMargin );

	m_projectedWidth = bbMax.x - bbMin.x;
	m_projectedHeight = bbMax.y - bbMin.y;

	bool useCenter3d = false;
	if( m_maxProjectedWidth > 0.0f || m_maxProjectedHeight > 0.0f )
	{
		useCenter3d = true;
	}

	float maxWidth = m_maxProjectedWidth;
	if( maxWidth == 0.0f )
	{
		maxWidth = 1e6f;
	}

	if( m_projectedWidth < m_minProjectedWidth )
	{
		m_projectedWidth = m_minProjectedWidth;
	}
	else if( m_projectedWidth > maxWidth )
	{
		m_projectedWidth = maxWidth;
	}

	float maxHeight = m_maxProjectedHeight;
	if( maxHeight == 0.0f )
	{
		maxHeight = 1e6f;
	}

	if( m_projectedHeight < m_minProjectedHeight )
	{
		m_projectedHeight = m_minProjectedHeight;
	}
	else if( m_projectedHeight > maxHeight )
	{
		m_projectedHeight = maxHeight;
	}

	float centerX;
	float centerY;
	if( useCenter3d )
	{
		// Bounded brackets are centered around the center of the 3d bounding box
		centerX = projectedCenter.x;
		centerY = projectedCenter.y;
	}
	else
	{
		// Unbounded brackets are centered around the center of the projected bounding box
		centerX = ( bbMin.x + bbMax.x ) * 0.5f;
		centerY = ( bbMin.y + bbMax.y ) * 0.5f;
	}
	m_projectedX = centerX - m_projectedWidth * 0.5f;
	m_projectedY = centerY - m_projectedHeight * 0.5f;

	if( m_integerCoordinates )
	{
		m_projectedX = floor( m_projectedX + 0.5f );
		m_projectedY = floor( m_projectedY + 0.5f );
		m_projectedWidth = floor( m_projectedWidth + 0.5f );
		m_projectedHeight = floor( m_projectedHeight + 0.5f );
	}

	if( m_projectedX < m_screenMargin )
	{
		float d = m_screenMargin - m_projectedX;
		if( d < m_projectedWidth - m_screenMargin )
		{
			m_projectedX = m_screenMargin;
		}
		else
		{
			SetEmptyProjection();
			return;
		}
	}

	if( m_projectedY < m_screenMargin )
	{
		float d = m_screenMargin - m_projectedY;
		if( d < m_projectedHeight - m_screenMargin )
		{
			m_projectedY = m_screenMargin;
		}
		else
		{
			SetEmptyProjection();
			return;
		}
		m_projectedY = m_screenMargin;
	}

	if( m_projectedX + m_projectedWidth > screenWidth - m_screenMargin )
	{
		m_projectedWidth = screenWidth - m_screenMargin - m_projectedX;
		if( m_projectedWidth < m_screenMargin )
		{
			SetEmptyProjection();
			return;
		}
	}

	if( m_projectedY + m_projectedHeight > screenHeight - m_screenMargin )
	{
		m_projectedHeight = screenHeight - m_screenMargin - m_projectedY;
		if( m_projectedHeight < m_screenMargin )
		{
			SetEmptyProjection();
			return;
		}
	}

	if( m_bracket )
	{
		m_bracket->SetDisplayX( m_projectedX );
		m_bracket->SetDisplayY( m_projectedY );
		m_bracket->SetDisplayWidth( m_projectedWidth );
		m_bracket->SetDisplayHeight( m_projectedHeight );
	}

	if( g_debugRenderer )
	{
		int x = (int)m_projectedX;
		int y = (int)m_projectedY;
		g_debugRenderer->Printf( x, y, 0xffffffff, "%S", m_name.c_str() );
		y += 16;

		g_debugRenderer->Printf( x, y, 0xffffffff, "(%5.2f, %5.2f, %5.2f)", bbMin.x, bbMin.y, bbMin.z );
		y += 16;

		g_debugRenderer->Printf( x, y, 0xffffffff, "(%5.2f, %5.2f, %5.2f)", bbMax.x, bbMax.y, bbMax.z );
		y += 16;

		g_debugRenderer->Printf( x, y, 0xffffffff, "(%5.2f, %5.2f)", m_projectedWidth, m_projectedHeight );
	}
}

void Tr2ProjectBoundingBoxBracket::SetEmptyProjection()
{
	m_projectedX = 0.0f;
	m_projectedY = 0.0f;
	m_projectedZ = 0.0f;
	m_projectedWidth = 0.0f;
	m_projectedHeight = 0.0f;

	if( m_bracket )
	{
		m_bracket->SetDisplayX( m_projectedX );
		m_bracket->SetDisplayY( m_projectedY );
		m_bracket->SetDisplayWidth( m_projectedWidth );
		m_bracket->SetDisplayHeight( m_projectedHeight );
	}
}
