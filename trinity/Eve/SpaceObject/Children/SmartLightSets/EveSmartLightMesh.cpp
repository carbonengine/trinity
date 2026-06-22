// Copyright © 2025 CCP ehf.

#include "StdAfx.h"
#include "EveSmartLightMesh.h"
#include "Shader/Tr2Effect.h"
#include "Tr2InstancedMesh.h"
#include "trimath.h"

EveSmartLightMesh::EveSmartLightMesh( IRoot* lockobj ) :
	EveChildInstanceMeshRenderer( lockobj ),
	m_shaderParamColorName( "" ),
	m_lastAreaColor( 0.f, 0.f, 0.f, 1.f )
{
}

uint32_t EveSmartLightMesh::GetNumberOfEntities() const
{
	return m_lastEntityCount;
}

void EveSmartLightMesh::UpdateSyncronous( const EveUpdateContext& updateContext, const EveChildUpdateParams& params, IEveDistributionMethod* distribution )
{
	// we use EveChildMesh instead of the direct parent as SmartLightMesh handles the distribution class differently and doesn't use it as a member variable
	EveChildMesh::UpdateSyncronous( updateContext, params );

	if( distribution != nullptr && m_display )
	{
		uint32_t numPlacements = uint32_t( distribution->GetNumberOfPlacements() );
		bool updateCount = m_lastEntityCount != numPlacements;
		m_lastEntityCount = numPlacements;

		if( numPlacements > 0 )
		{
			const PlacementDataWithIdentifierStructureList& placements = *distribution->GetPlacementData();
			Vector4 currentColor = this->GetGroupColor() * m_activationStrength;

			if( !m_attributeModifiers.empty() )
			{
				PlacementDataWithIdentifier firstPlacement = placements[0];
				Vector3 firstRotation( 0.f, 1.f, 0.f );
				// use first placement as a ref for modifiers as we can't set the shader params per instance

				Quaternion firstRotationQuad = firstPlacement.initialRotation * firstPlacement.additionalRotation;
				TriVectorRotateQuaternion( &firstRotation, &firstRotation, &firstRotationQuad );
				TriVectorRotateMatrix( &firstRotation, &firstRotation, &m_worldTransform );
				Vector3 Center = distribution->GetPlacementDataCenter();

				for( auto attributeModifier : m_attributeModifiers )
				{
					attributeModifier->ProcessAttributeModifier( currentColor.GetXYZ(), firstPlacement, Center, firstRotation, params.activationStrength );
				}
			}

			SetMeshColorParameter( currentColor );

			if( Tr2InstancedMeshPtr mesh = BlueCastPtr( m_mesh ) )
			{
				if( !mesh->GetInstanceGeometryResource() )
				{
					// happens once
					ConfigureInstanceData();
					UpdateGeometryResource( placements, numPlacements );
					UpdateBoundingSphere( placements, distribution );
				}
				else
				{
					bool alwaysUpdate = distribution->GetHasDynamicMovement() || m_rotationConstraint != EveChildInstanceMeshRenderer::NONE;
					if( updateCount || alwaysUpdate )
					{
						UpdateGeometryResource( placements, numPlacements );
						UpdateBoundingSphere( placements, distribution );
					}
				}
			}
		}
	}
}

void EveSmartLightMesh::SetMeshColorParameter( Vector4 meshColor )
{
	if( m_shaderParamColorName.empty() )
	{
		return;
	}

	if( !m_display )
	{
		return;
	}

	if( m_lastAreaColor == meshColor )
	{
		return;
	}

	if( m_mesh == nullptr )
	{
		return;
	}

	auto geomRes = m_mesh->GetGeometryResource();

	if( geomRes == nullptr )
	{
		return;
	}

	if( !( geomRes->IsGood() ) )
	{
		return;
	}

	auto grannyMesh = geomRes->GetMeshData( m_mesh->GetMeshIndex() );
	if( !grannyMesh )
	{
		return;
	}

	if( !( m_mesh->GetDisplay() ) )
	{
		return;
	}

	auto areaList = m_mesh->GetAllAreas();

	for( auto area : areaList )
	{
		auto materialInterface = area->GetMaterialInterface();

		if( Tr2EffectPtr effect = BlueCastPtr( materialInterface ) )
		{
			effect->SetParameter( m_shaderParamColorName, meshColor );
		}
	}

	m_lastAreaColor = meshColor;
}

void EveSmartLightMesh::UpdateAsyncronous( const EveUpdateContext& updateContext, const EveChildUpdateParams& params, IEveDistributionMethod* distribution )
{
	EveChildInstanceMeshRenderer::UpdateAsyncronous( updateContext, params );
}

void EveSmartLightMesh::GetRenderables( std::vector<ITr2Renderable*>& renderables )
{
	EveChildInstanceMeshRenderer::GetRenderables( renderables );
}

void EveSmartLightMesh::UpdateVisibility( const EveUpdateContext& updateContext, const Matrix& parentTransform, Tr2Lod parentLod )
{
	EveChildInstanceMeshRenderer::UpdateVisibility( updateContext, parentTransform, parentLod );
}