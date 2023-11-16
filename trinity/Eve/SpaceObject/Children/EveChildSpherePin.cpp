#include "StdAfx.h"
#include "EveChildSpherePin.h"
#include "TriRenderBatch.h"
#include "Eve/EveUpdateContext.h"
#include "Curves/TriCurveSet.h"

static const char* PIN_PICK_EFFECT_PATH = "res:/Graphics/Effect/Managed/Space/UI/SpherePinPicking.fx";

using namespace Tr2RenderContextEnum;

EveChildSpherePin::EveChildSpherePin( IRoot* lockobj ) :
	EveChildMesh( lockobj ),
	PARENTLOCK( m_curveSets ),
	m_centerNormal( .0f, .0f, .0f ),
	m_pinMaxRadius( .2f ),
	m_pinRadius( .0f ),
	m_pinRotation( .0f ),
	m_pinColor( 1.f, 1.f, 1.f, 1.f ),
	m_pinAlphaThreshold( .0f )
{
}


EveChildSpherePin::~EveChildSpherePin()
{
}

void EveChildSpherePin::UpdateAsyncronous( EveUpdateContext& updateContext, const EveChildUpdateParams& params )
{
	EveChildMesh::UpdateAsyncronous( updateContext, params );
	const auto time = updateContext.GetTime();
	for (auto it = m_curveSets.begin(); it != m_curveSets.end(); ++it)
	{
		(*it)->Update( time, time );
	}
}

Tr2PerObjectData* EveChildSpherePin::GetPerObjectData( ITriRenderBatchAccumulator* accumulator )
{
	
	// allocate only once
	auto perObjectData = accumulator->Allocate<EveChildSpherePinPerObjectData>();
	
	if ( nullptr == perObjectData )
	{
		return nullptr;
	}
	
	// set world matrix
	perObjectData->m_worldMatrix = Transpose( m_worldTransform );
	// set all other pin data
	perObjectData->m_pinPosition = Vector4( m_centerNormal, m_pinRadius );
	perObjectData->m_pinRotation = Vector4( m_pinRotation, 0.f, 0.f, 0.f );
	perObjectData->m_pinColor = Vector4( m_pinColor.r, m_pinColor.g, m_pinColor.b, m_pinColor.a );
	perObjectData->m_pinThreshold = Vector4( m_pinAlphaThreshold, 0.f, 0.f, 0.f );
	perObjectData->m_pinRadiusPrecalc = Vector4( sinf( m_pinRadius ), cosf( m_pinRadius ), sinf( m_pinRotation ), cosf( m_pinRotation ) );
	perObjectData->m_pinUV = Vector4(1.f, 1.f, .0f, .0f);

	return perObjectData;
}

// --------------------------------------------------------------------------------
// Description:
//   Copy all the matrices and other data to HW
// --------------------------------------------------------------------------------
void EveChildSpherePinPerObjectData::SetPerObjectDataToDevice( Tr2ConstantBufferAL** buffers, unsigned constantTypeMask, Tr2RenderContext& renderContext ) const
{
	CCP_STATS_ZONE( __FUNCTION__ );

	const int constantCount = sizeof( EveChildSpherePinPerObjectData ) - sizeof( Tr2PerObjectData );
	FillAndSetConstants( *buffers[VERTEX_SHADER], &m_worldMatrix, constantCount, VERTEX_SHADER, Tr2Renderer::GetPerObjectVSStartRegister(), renderContext );
	FillAndSetConstants( *buffers[PIXEL_SHADER], &m_worldMatrix, constantCount, PIXEL_SHADER, Tr2Renderer::GetPerObjectPSStartRegister(), renderContext );
}
