// Copyright © 2023 CCP ehf.

#include "StdAfx.h"
#include "EveCurveLineSet.h"
#include "Eve/EveConstantBufferFormats.h"
#include "TriFrustum.h"
#include "Shader/Tr2Effect.h"
#include "Utilities/BoundingSphere.h"
#include "Tr2PerObjectData.h"
#include "TriPoolAllocator.h"
#include "TriRenderBatch.h"

static const char* CURVE_LINE_EFFECT_PATH = "res:/Graphics/Effect/Managed/Space/SpecialFX/Lines3D.fx";
static const char* CURVE_PICK_EFFECT_PATH = "res:/Graphics/Effect/Managed/Space/SpecialFX/Lines3DPicking.fx";

CCP_STATS_DECLARED_ELSEWHERE( primitiveCount );

// ------------------------------------------------------------------------------------------------------
EveCurveLineSet::EveCurveLineSet( IRoot* lockobj /*= NULL*/ ) :
	Tr2CurveLineSet( lockobj ),
	m_isVisible( false )
{
	// create line draw effect
	Tr2EffectPtr lineEffect;
	lineEffect.CreateInstance();
	lineEffect->SetEffectPathName( CURVE_LINE_EFFECT_PATH );
	m_lineEffect = lineEffect;

	Tr2EffectPtr pickEffect;
	pickEffect.CreateInstance();
	pickEffect->SetEffectPathName( CURVE_PICK_EFFECT_PATH );
	m_pickEffect = pickEffect;

	// init
	m_worldTransform = IdentityMatrix();
	BoundingSphereInitialize( m_boundingSphere );
}

// ------------------------------------------------------------------------------------------------------
EveCurveLineSet::~EveCurveLineSet()
{
	ReleaseResources( TRISTORAGE_ALL );
}

// ------------------------------------------------------------------------------------------------------
void EveCurveLineSet::UpdateSyncronous( const EveUpdateContext& updateContext )
{
}

// ------------------------------------------------------------------------------------------------------
void EveCurveLineSet::UpdateAsyncronous( const EveUpdateContext& updateContext )
{
}

// ------------------------------------------------------------------------------------------------------
void EveCurveLineSet::Update( const EveUpdateContext& updateContext )
{
}

void EveCurveLineSet::UpdateVisibility( const EveUpdateContext& updateContext, const Matrix& parentTransform )
{
	m_isVisible = false;

	if( !m_display )
	{
		return;
	}

	// local transform
	Matrix localTransform = TransformationMatrix( m_scaling, m_rotation, m_translation );

	// store final world transform
	m_worldTransform = localTransform * parentTransform;
	Vector4 boundingSphere = m_boundingSphere;
	BoundingSphereTransform( m_worldTransform, boundingSphere );

	// cull!
	if( updateContext.GetFrustum().IsSphereVisible( &boundingSphere ) )
	{
		m_isVisible = true;
	}
}

// ------------------------------------------------------------------------------------------------------
void EveCurveLineSet::GetRenderables( std::vector<ITr2Renderable*>& renderables, Tr2ImpostorManager* impostors )
{
	if( m_isVisible )
	{
		renderables.push_back( this );
	}
}

void EveCurveLineSet::GetRenderables( std::vector<ITr2Renderable*>& renderables )
{
	GetRenderables( renderables, nullptr );
}

// ------------------------------------------------------------------------------------------------------
bool EveCurveLineSet::GetBoundingSphere( Vector4& sphere, BoundingSphereQuery query ) const
{
	sphere = m_boundingSphere;
	return true;
}

// ------------------------------------------------------------------------------------------------------
Tr2PerObjectData* EveCurveLineSet::GetPerObjectData( ITriRenderBatchAccumulator* accumulator )
{
	Tr2PerObjectDataStandard* data = accumulator->Allocate<Tr2PerObjectDataStandard>();

	if( !data )
	{
		return nullptr;
	}

	EvePerObjectPSData perObjectPSBuffer;
	EvePerObjectVSData perObjectVSBuffer;

	// column_major for shaders
	perObjectVSBuffer.WorldMat = Transpose( m_worldTransform );
	perObjectPSBuffer.WorldMat = Transpose( m_worldTransform );

	data->CopyToVSFloatBuffer( perObjectVSBuffer );
	data->CopyToPSFloatBuffer( perObjectPSBuffer );

	return data;
}