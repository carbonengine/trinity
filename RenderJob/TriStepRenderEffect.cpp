#include "StdAfx.h"
#include "TriStepRenderEffect.h"
#include "Tr2Renderer.h"
#include "Tr2EffectStateManager.h"
#include "Tr2Effect.h"

CCP_STATS_DECLARE( rsRenderEffectCount, "Trinity/RenderStep/RenderEffectCount", true, CST_COUNTER_LOW, "Calls to TriStepRenderEffect::Execute per frame" );

TriStepRenderEffect::TriStepRenderEffect( IRoot* lockobj )
{
	m_tlTexCoord = Vector2( 0.0f, 0.0f );
	m_brTexCoord = Vector2( 1.0f, 1.0f );
}

TriStepRenderEffect::~TriStepRenderEffect(void)
{
}

void TriStepRenderEffect::py__init__( Tr2Effect* effect )
{
	m_effect = effect;
}

TriStepResult TriStepRenderEffect::Execute( Be::Time realTime, Be::Time simTime, Tr2RenderContext& renderContext )
{
	CCP_STATS_INC( rsRenderEffectCount );

	if( m_effect )
	{
		Tr2Renderer::DrawTexture( m_effect, m_tlTexCoord, m_brTexCoord );
	}
	return RS_OK;
}