// Copyright © 2023 CCP ehf.

#include "StdAfx.h"
#include "TriDebugResourceHelper.h"
#include "Shader/Tr2Effect.h"

static const char* DEBUG_EFFECT_PATH = "res:/Graphics/Effect/Managed/Utility/Debug.fx";
static const char* DEBUG_LINE_EFFECT_PATH = "res:/Graphics/Effect/Managed/Utility/DebugLine.fx";

TriDebugResourceHelper g_debugResourceHelper;

TriDebugResourceHelper::TriDebugResourceHelper() :
	m_vertexPosColorDecl( Tr2EffectStateManager::UNINITIALIZED_DECLARATION ),
	m_vertexPosDecl( Tr2EffectStateManager::UNINITIALIZED_DECLARATION )
{
}

TriDebugResourceHelper::~TriDebugResourceHelper()
{
	m_effect = 0;
}

bool TriDebugResourceHelper::OnPrepareResources()
{
	{
		Tr2VertexDefinition vd;
		vd.Add( vd.FLOAT32_3, vd.POSITION );
		m_vertexPosDecl = Tr2EffectStateManager::GetVertexDeclarationHandle( vd );
	}

	{
		Tr2VertexDefinition vd;
		vd.Add( vd.FLOAT32_3, vd.POSITION );
		vd.Add( vd.UBYTE_4_NORM, vd.COLOR );
		m_vertexPosColorDecl = Tr2EffectStateManager::GetVertexDeclarationHandle( vd );
	}

	return true;
}

void TriDebugResourceHelper::ReleaseResources( TriStorage s )
{
	m_vertexPosDecl = 0;
	m_vertexPosColorDecl = 0;
}

Tr2Effect* TriDebugResourceHelper::GetEffect()
{
	if( !m_effect )
	{
		m_effect.CreateInstance();
		m_effect->SetEffectPathName( DEBUG_EFFECT_PATH );
	}

	return m_effect;
}

Tr2Effect* TriDebugResourceHelper::GetLineEffect()
{
	if( !m_lineEffect )
	{
		m_lineEffect.CreateInstance();
		m_lineEffect->SetEffectPathName( DEBUG_LINE_EFFECT_PATH );
	}

	return m_lineEffect;
}

unsigned int TriDebugResourceHelper::GetVertexPosDecl()
{
	return m_vertexPosDecl;
}

unsigned int TriDebugResourceHelper::GetVertexPosColorDecl()
{
	return m_vertexPosColorDecl;
}