////////////////////////////////////////////////////////////////////////////////
//
// Created:		January 2011
// Copyright:	CCP 2011
//

#include "StdAfx.h"
#include "Tr2Sprite2dRenderJob.h"
#include "Tr2Sprite2dScene.h"
#include "Tr2Sprite2dPickingMask.h"


Tr2Sprite2dRenderJob::Tr2Sprite2dRenderJob( IRoot* lockobj /*= NULL */ )
{
}

Tr2Sprite2dRenderJob::~Tr2Sprite2dRenderJob()
{

}

void Tr2Sprite2dRenderJob::GatherSprites( Tr2Sprite2dScene* renderer )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	if( m_renderJob && m_display )
	{
		renderer->RunJob( m_renderJob );
	}
}

ITr2SpriteObject* Tr2Sprite2dRenderJob::PickPoint( float x, float y, Tr2Sprite2dScene* renderer )
{
	if( !m_display )
	{
		return NULL;
	}

	if( m_pickState == TR2_SPS_ON )
	{
		if( renderer->IsInside( Vector2( x, y ), m_translation, m_displayWidth, m_displayHeight, 0.0f ) )
		{
			if( !m_pickingMask || m_pickingMask->SampleMask( renderer->InverseTransformPoint( Vector2( x, y ) ), m_translation, m_displayWidth, m_displayHeight ) )
			{
				return this;
			}
		}
	}

	return NULL;
}

unsigned int Tr2Sprite2dRenderJob::GetVertexCount()
{
	return 0;
}
