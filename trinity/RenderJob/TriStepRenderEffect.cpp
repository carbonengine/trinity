#include "StdAfx.h"
#include "TriStepRenderEffect.h"
#include "Tr2Renderer.h"
#include "Shader/Tr2EffectStateManager.h"
#include "Shader/Tr2Effect.h"
#include "Shader/Tr2ShaderBuffer.h"

CCP_STATS_DECLARE( rsRenderEffectCount, "Trinity/RenderStep/RenderEffectCount", true, CST_COUNTER_LOW, "Calls to TriStepRenderEffect::Execute per frame" );

TriStepRenderEffect::TriStepRenderEffect( IRoot* lockobj )
	:m_tlTexCoord( 0.0f, 0.0f ),
	m_brTexCoord( 1.0f, 1.0f )
{
}

TriStepRenderEffect::~TriStepRenderEffect(void)
{
}

void TriStepRenderEffect::py__init__( Tr2Effect* effect, Tr2ShaderBuffer* shaderBuffer )
{
	m_effect = effect;
	m_shaderBuffer = shaderBuffer;
}

TriStepResult TriStepRenderEffect::Execute( Be::Time realTime, Be::Time simTime, Tr2RenderContext& renderContext )
{
	CCP_STATS_INC( rsRenderEffectCount );

	if( m_effect )
	{
		if( m_shaderBuffer )
		{
			m_shaderBuffer->ApplyBuffer( renderContext );
		}
		Tr2Renderer::DrawTexture( renderContext, m_effect, m_tlTexCoord, m_brTexCoord );
	}
	return RS_OK;
}