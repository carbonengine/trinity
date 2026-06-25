// Copyright © 2010 CCP ehf.

#include "StdAfx.h"
#include "EveProjectBracket.h"
#include "EveSprite2dBracket.h"
#include "Sprite2d/Tr2Sprite2dContainer.h"
#include "Tr2Renderer.h"
#include "TriViewport.h"
#include "include/ITr2DebugRenderer.h"

extern ITr2DebugRendererPtr g_debugRenderer;

EveProjectBracket::EveProjectBracket( IRoot* lockobj /*= NULL */ ) :
	m_trackPosition( 0.0f, 0.0f, 0.0f ),
	m_ballTrackingScaling( 1.0f ),
	m_dock( false ),
	m_isVisible( true ),
	m_isInFront( true ),
	m_isVisibleStateSet( false ),
	m_integerCoordinates( true ),
	m_marginLeft( 0.0f ),
	m_marginRight( 0.0f ),
	m_marginTop( 0.0f ),
	m_marginBottom( 0.0f ),
	m_minDispRange( 0.0f ),
	m_maxDispRange( FLT_MAX ),
	m_cameraDistance( 0.0f ),
	m_offsetX( 0 ),
	m_offsetY( 0 ),
	m_projectedPosition( 0.0f, 0.0f )
{
}

//Cylindric projection.  projects all points legally by rotating through the vertical and horizontal
//camera axii, landing them on a vertical or horizontal plane through the camera's origin.
//These two points are then projected regularly to get a x and y projected coordinate.
static Vector3 BicylindricProjection( const Vector3& pos, const TriViewport& viewport )
{
	bool behind = pos.z >= 0.0f;

	//horizontal rotation to center.  We use vector arithmetic for speed, by just projecting
	//the vector on the vertical view plane, and then correcting the z coordinate to get the
	//correct vector length.
	float zSq = pos.z * pos.z;
	Vector3 v[2] = {
		//rotate onto vertical plane about the vertical axis
		Vector3( 0.0f, -pos.y, sqrtf( pos.x * pos.x + zSq ) ),
		//Same for horizontal
		Vector3( pos.x, 0.0f, sqrtf( pos.y * pos.y + zSq ) )
	};

	//now, project both vectors  (view already done)
	for( int i = 0; i < 2; ++i )
	{
		v[i] = TransformCoord( v[i], Tr2Renderer::GetProjectionTransform() );
		Vec3TransformByViewport( v[i], viewport );
	}

	Vector3 res;
	res.x = v[1].x;
	res.y = v[0].y;
	res.z = ( behind ? -0.5f : 0.5f ) * ( v[0].z + v[1].z );
	return res;
}


void EveProjectBracket::UpdateValue( double time )
{
	Vector3 pos;
	if( m_trackBall )
	{
		m_trackBall->GetValueAt( &pos, BeOS->GetCurrentFrameTime() );

		pos *= m_ballTrackingScaling;
	}
	else if( m_trackTransform )
	{
		pos = m_trackTransform->GetWorldPosition();
	}
	else
	{
		pos = m_trackPosition;
	}

	pos = TransformCoord( pos, Tr2Renderer::GetViewTransform() );

	bool isInFront = ( pos.z <= 0.0f );
	m_isInFront = isInFront;

	m_cameraDistance = Length( pos );

	Vector3 projectedPosition;
	const TriViewport& vp = Tr2Renderer::GetViewport();

	projectedPosition = TransformCoord( pos, Tr2Renderer::GetProjectionTransform() );
	Vec3TransformByViewport( projectedPosition, vp );

	float x = projectedPosition.x;
	float y = projectedPosition.y;

	m_rawProjectedPosition.x = x;
	m_rawProjectedPosition.y = y;

	// Brackets behind the camera with the 'dock' flag cleared are hidden
	if( ( !isInFront && !m_dock ) )
	{
		SetBracketDisplayState( false );
		return;
	}

	// Brackets outside the display range are hidden
	if( ( m_cameraDistance < m_minDispRange ) || ( m_cameraDistance > m_maxDispRange ) )
	{
		SetBracketDisplayState( false );
		return;
	}

	// Turn brackets on again, if needed
	SetBracketDisplayState( true );

	float left = vp.x + m_marginLeft;
	float right = vp.x + vp.width - m_marginRight;
	float top = vp.y + m_marginTop;
	float bottom = vp.y + vp.height - m_marginBottom;

	if( m_parent )
	{
		x -= m_parent->GetDisplayX();
		y -= m_parent->GetDisplayY();

		// Keep in mind that the bracket is rendered relative to the parent.
		// Left/top are therefore zero based.
		left = m_marginLeft;
		top = m_marginTop;
		right = left + m_parent->GetDisplayWidth() - m_marginRight;
		bottom = top + m_parent->GetDisplayHeight() - m_marginBottom;
	}

	if( m_bracket )
	{
		x -= m_bracket->GetDisplayWidth() / 2.0f;
		y -= m_bracket->GetDisplayHeight() / 2.0f;
	}

	uint32_t debugColor = 0xffffffff;

	if( m_dock )
	{
		if( !isInFront || x < left || x >= right || y < top || y >= bottom )
		{
			Vector3 cylindricalProj = BicylindricProjection( pos, vp );
			y = cylindricalProj.y;

			float halfViewportHeight = (float)( vp.height ) * 0.5f;
			if( y >= halfViewportHeight )
			{
				y = ( y - halfViewportHeight ) * 1.5f + halfViewportHeight;
			}
			else
			{
				y = halfViewportHeight - ( halfViewportHeight - y ) * 1.5f;
			}

			if( isInFront )
			{
				debugColor = 0xff00ff00;
			}
			else
			{
				debugColor = 0xffff0000;
			}
		}

		if( isInFront )
		{
			if( x < left )
			{
				x = left;
			}
			else if( x > right )
			{
				x = right;
			}
		}
		else
		{
			if( x > ( (float)vp.width ) * 0.5f )
			{
				x = left;
			}
			else
			{
				x = right;
			}
		}

		if( y < top )
		{
			y = top;
		}
		else if( y > bottom )
		{
			y = bottom;
		}
	}

	x += m_offsetX;
	y += m_offsetY;

	if( m_integerCoordinates )
	{
		x = floor( x + 0.5f );
		y = floor( y + 0.5f );
	}

	if( g_debugRenderer )
	{
		g_debugRenderer->Printf( (int)x, (int)y, debugColor, "%S", m_name.c_str() );
	}

	if( m_bracket )
	{
		m_bracket->SetDisplayX( x );
		m_bracket->SetDisplayY( y );
	}

	m_projectedPosition.x = x;
	m_projectedPosition.y = y;

	if( m_bracketUpdateCallback )
	{
		m_bracketUpdateCallback.CallVoid( this );
	}

	if( m_bracketIcon )
	{
		m_bracketIcon->SetTranslation( m_projectedPosition );
	}
}

void EveProjectBracket::SetBracketDisplayState( bool state )
{
	if( ( state == m_isVisible ) && m_isVisibleStateSet )
	{
		return;
	}

	m_isVisible = state;
	m_isVisibleStateSet = true;

	if( m_bracket )
	{
		m_bracket->SetDisplay( state );
	}
	if( m_bracketIcon )
	{
		m_bracketIcon->SetDisplay( state );
	}

	if( m_displayChangeCallback )
	{
		m_displayChangeCallback.CallVoid( this, state );
	}
}
