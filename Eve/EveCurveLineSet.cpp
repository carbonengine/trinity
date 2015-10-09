#include "StdAfx.h"
#include "EveCurveLineSet.h"
#include "Eve/EveConstantBufferFormats.h"
#include "TriFrustum.h"
#include "Tr2Effect.h"
#include "Utilities/BoundingSphere.h"
#include "Tr2PerObjectData.h"
#include "TriPoolAllocator.h"
#include "TriRenderBatch.h"

static const char* CURVE_LINE_EFFECT_PATH = "res:/Graphics/Effect/Managed/Space/SpecialFX/Lines3D.fx";
static const char* CURVE_PICK_EFFECT_PATH = "res:/Graphics/Effect/Managed/Space/SpecialFX/Lines3DPicking.fx";

CCP_STATS_DECLARED_ELSEWHERE( primitiveCount );

// ------------------------------------------------------------------------------------------------------
EveCurveLineSet::EveCurveLineSet( IRoot* lockobj /*= NULL*/ ):
	Tr2CurveLineSet( lockobj )
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
	D3DXMatrixIdentity( &m_worldTransform );
	BoundingSphereInitialize( m_boundingSphere );
}

// ------------------------------------------------------------------------------------------------------
EveCurveLineSet::~EveCurveLineSet()
{
	ReleaseResources( TRISTORAGE_ALL );
}

// ------------------------------------------------------------------------------------------------------
void EveCurveLineSet::UpdateSyncronous( EveUpdateContext& updateContext )
{
}

// ------------------------------------------------------------------------------------------------------
void EveCurveLineSet::UpdateAsyncronous( EveUpdateContext& updateContext )
{
}

// ------------------------------------------------------------------------------------------------------
void EveCurveLineSet::Update( EveUpdateContext& updateContext )
{
}

// ------------------------------------------------------------------------------------------------------
void EveCurveLineSet::RenderDebugInfo( Tr2RenderContext& renderContext )
{
}

// ------------------------------------------------------------------------------------------------------
void EveCurveLineSet::UpdateViewDependentData( const Matrix& parentTransform )
{
	// local transform
	Matrix localTransform;
	D3DXMatrixTransformation( &localTransform, NULL, NULL, &m_scaling, NULL, &m_rotation, &m_translation );

	// store final world transform
	D3DXMatrixMultiply( &m_worldTransform, &localTransform, &parentTransform );
}

// ------------------------------------------------------------------------------------------------------
void EveCurveLineSet::GetRenderables( const TriFrustum& frustum, std::vector<ITr2Renderable*>& renderables, const Matrix& parentTransform )
{
	if( !m_display )
	{
		return;
	}

	// position the lines with parent transform
	UpdateViewDependentData( parentTransform );
	Vector4 boundingSphere = m_boundingSphere;
	BoundingSphereTransform( m_worldTransform, boundingSphere );

	// cull!
	if( frustum.IsSphereVisible( &boundingSphere ) )
	{
		renderables.push_back( this );
	}
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
	D3DXMatrixTranspose( &perObjectVSBuffer.WorldMat, &m_worldTransform );
	D3DXMatrixTranspose( &perObjectPSBuffer.WorldMat, &m_worldTransform );

	data->CopyToVSFloatBuffer( perObjectVSBuffer );
	data->CopyToPSFloatBuffer( perObjectPSBuffer );

	return data;
}