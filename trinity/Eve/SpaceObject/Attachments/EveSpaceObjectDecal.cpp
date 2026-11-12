// Copyright © 2023 CCP ehf.

#include "StdAfx.h"
#include "EveSpaceObjectDecal.h"
#include "Tr2Renderer.h"
#include "TriRenderBatch.h"
#include "TriPoolAllocator.h"
#include "Shader/Tr2Effect.h"
#include "Tr2Mesh.h"
#include "Resources/TriGeometryRes.h"
#include "Tr2RuntimeInstanceData.h"
#include "Utilities/BoundingBox.h"
#include "Utilities/MatrixUtils.h"
#include "TriFrustum.h"
#include "Tr2InstancedMesh.h"
#include "TriSettingsRegistrar.h"
#include "Eve/SpaceObject/Children/EveSpaceObjectChild.h"
#include "EveDecalTriangleCache.h"


CCP_STATS_DECLARED_ELSEWHERE( primitiveCount );
CCP_STATS_DECLARE( decalDPCount, "Trinity/EveSpaceObject2/DecalDPCount", true, CST_COUNTER_LOW, "Number of decals rendered" );

bool g_buildDecalBuffers = false;
TRI_REGISTER_SETTING( "buildDecalBuffers", g_buildDecalBuffers );

using namespace Tr2RenderContextEnum;

static BlueStructureDefinition s_eveSpaceObjectDecalIndexDef[] = {
	{ "index", Be::UINT32_1, 0 },
	{ 0 }
};

// ------------------------------------------------------------------------------------------------------
EveSpaceObjectDecal::EveSpaceObjectDecal( IRoot* lockobj ) :
	m_display( true ),
	m_position( 0.f, 0.f, 0.f ),
	m_rotation( 0.f, 0.f, 0.f, 1.f ),
	m_scaling( 1.f, 1.f, 1.f ),
	m_parentBoneIndex( -1 ),
	m_geometryLodIndex( -1 ),
	m_isVisible( 0 ),
	m_decalMatrix( IdentityMatrix() ),
	m_inverseDecalMatrix( IdentityMatrix() ),
	m_parentBoneMatrix( IdentityMatrix() ),
	m_minScreenSize( 0 ),
	m_instanceScreenSize( -1 ),
	m_batchType( TRIBATCHTYPE_DECAL ),
	m_priority( 0 ),
	m_vertexDeclarationOverride( Tr2EffectStateManager::UNINITIALIZED_DECLARATION ),
	m_instanceData( nullptr ),
	m_minBounds( 0, 0, 0 ),
	m_maxBounds( 0, 0, 0 ),
	m_isGeometryFrozen( false ),
	m_dynamicIndexBuffers( false ),
	m_usePartTargets( false )
{
	// init
	PrepareResources();
	memset( &m_parentData, 0, sizeof( IEveSpaceObject2::ParentData ) );
	m_parentData.transform = IdentityMatrix();
}

// ------------------------------------------------------------------------------------------------------
EveSpaceObjectDecal::~EveSpaceObjectDecal()
{
	ReleaseResources( TRISTORAGE_ALL );
}

// ------------------------------------------------------------------------------------------------------
void EveSpaceObjectDecal::UpdateDecalMatrix()
{
	// update decal matrix
	m_decalMatrix = TransformationMatrix( m_scaling, m_rotation, m_position );
	// and calc the inverse right away, is needed for intersection
	m_inverseDecalMatrix = Inverse( m_decalMatrix );
}

// ------------------------------------------------------------------------------------------------------
bool EveSpaceObjectDecal::Initialize()
{
	UpdateDecalMatrix();

	PrepareResources();

	return true;
}

// ------------------------------------------------------------------------------------------------------
bool EveSpaceObjectDecal::OnModified( Be::Var* value )
{
	if( IsMatch( value, m_position ) || IsMatch( value, m_rotation ) || IsMatch( value, m_scaling ) )
	{
		// update the decal matrix
		UpdateDecalMatrix();
		// and rebuild the index buffer, cause decal volume has changed size and might hit new triangles
		if( !HasStaticIndexBuffers() )
		{
			m_decalGeometry = nullptr;
		}
		// the part targets cache the decal volume in part space
		m_partTargets.clear();
	}
	return true;
}

// ------------------------------------------------------------------------------------------------------
void EveSpaceObjectDecal::ReleaseResources( TriStorage )
{
}

// ------------------------------------------------------------------------------------------------------
bool EveSpaceObjectDecal::OnPrepareResources()
{
	// create new index buffer
	if( m_decalGeometry && !m_decalGeometry->m_indexBuffer.IsValid() )
	{
		m_decalGeometry = nullptr;
	}
	for( auto& target : m_partTargets )
	{
		if( target.decalGeometry && !target.decalGeometry->m_indexBuffer.IsValid() )
		{
			target.decalGeometry = nullptr;
		}
	}
	return true;
}

void EveSpaceObjectDecal::UpdateVisibility( const EveUpdateContext& updateContext, const IEveSpaceObject2::ParentData* parentData )
{
	m_isVisible = 0;
	m_lastFrustum = updateContext.GetFrustum();
	m_lastInvLodFactor = updateContext.GetInvLodFactor();
	m_hasLastFrustum = true;

	if( !m_display || !m_decalEffect )
	{
		return;
	}

	if( m_minScreenSize > 0 )
	{
		Matrix worldDecalMatrix = m_parentBoneMatrix * parentData->transform;

		Vector3 min( -1, -1, -1 );
		Vector3 max( 1, 1, 1 );
		bool isInstanced = m_instanceData != nullptr;
		auto& frustum = updateContext.GetFrustum();

		// are we using instance magic?
		if( isInstanced )
		{
			// then we need to use the geometry mesh bounding box as the sphere
			min = XMVector3Transform( m_minBounds, m_parentBoneMatrix * parentData->transform );
			max = XMVector3Transform( m_maxBounds, m_parentBoneMatrix * parentData->transform );

			if( BoundingBoxIsInside( min, max, frustum.m_viewPos ) )
			{
				m_isVisible = 1;
				m_parentData = *parentData;
				return;
			}
			else if( !frustum.IsBoxVisible( min, max ) )
			{
				m_isVisible = 0;
				return;
			}

			Vector3 closest = ClosestPointToBoundingBox( min, max, frustum.m_viewPos );
			worldDecalMatrix = TranslationMatrix( closest - frustum.m_viewPos ) * worldDecalMatrix;
		}

		worldDecalMatrix = m_decalMatrix * worldDecalMatrix;
		BoundingBoxTransform( min, max, worldDecalMatrix );
		Vector3 sphereCenter( ( min + max ) / 2 );
		float sphereRadius( Length( min - max ) / 2 );

		auto pixelSize = frustum.GetPixelSizeAccrossEst( sphereCenter, sphereRadius );

		float modifiedMinScreen = m_minScreenSize * updateContext.GetLodFactor();
		if( pixelSize < modifiedMinScreen )
		{
			m_isVisible = 0;
			return;
		}

		m_isVisible = std::min( ( pixelSize - modifiedMinScreen ) / ( modifiedMinScreen * 0.5f ), 1.f );
	}
	else
	{
		m_isVisible = 1;
	}
	m_parentData = *parentData;
}

// ------------------------------------------------------------------------------------------------------
void EveSpaceObjectDecal::GetRenderables( std::vector<ITr2Renderable*>& renderables, DecalMeshCache& meshCache, TriGeometryRes* geomRes, float screensize )
{
	if( m_usePartTargets )
	{
		m_usePartTargets = false;
		m_partTargets.clear();
	}

	if( m_isVisible <= 0 || !geomRes )
	{
		return;
	}

	// if we get a new mesh, we must re-build the index buffer. This should be avoided cause it is slow!
	if( geomRes != m_baseGeometryResource )
	{
		m_decalGeometry = nullptr;
		m_baseGeometryResource = geomRes;
	}

	auto mesh = geomRes->GetMeshData( 0 );

	if( !m_decalGeometry || ( mesh && mesh->m_lodMask != m_decalGeometry->m_lodMask ) )
	{
		if( HasStaticIndexBuffers() )
		{
			CreateStaticIndexBuffers( geomRes );
		}
		else if( m_dynamicIndexBuffers )
		{
			// runtime decal: owned here, never added to the mesh's shared decal cache (that cache only grows)
			m_decalGeometry = mesh && geomRes->IsGood() ? BuildDecalGeometryCached( mesh, m_decalMatrix, m_inverseDecalMatrix ) : nullptr;
		}
		else if( g_buildDecalBuffers )
		{
			CreateDecalIndexBuffers( geomRes, meshCache );
		}
	}

	if( !m_decalGeometry )
	{
		return;
	}

	m_geometryLodIndex = m_isGeometryFrozen ? 0 : geomRes->GetLodIndexForScreenSize( 0, screensize );

	if( m_geometryLodIndex >= m_decalGeometry->m_lods.size() ||
		!m_decalGeometry->m_indexBuffer.IsValid() ||
		!m_decalGeometry->m_lods[m_geometryLodIndex].m_primitiveCount )
	{
		return;
	}

	renderables.push_back( this );
}

namespace
{
// A part decal's world matrix is composed on the CPU, while the part itself is drawn through the
// instanced path (instance matrix, then the child's world matrix, on the GPU): the two positions
// differ by rounding, and with no depth write or bias the decal z-fights its own surface. Scaling
// the decal about the eye by this much leaves every pixel where it is and moves it this fraction of
// its distance towards the eye, which is well above that rounding and well below anything visible.
constexpr float PART_DEPTH_PULL = 4e-4f;

// The LOD the instanced mesh manager draws a part with (EveInstancedMeshManager::GetMeshLod)
int InstancedMeshLod( const TriGeometryResMeshData& meshData, float screenSize )
{
	const size_t lodCount = meshData.m_lods.size();
	for( size_t lod = 0; lod + 1 < lodCount; ++lod )
	{
		if( screenSize > meshData.m_lods[lod + 1]->m_maxScreenSize )
		{
			return int( lod );
		}
	}
	return int( lodCount ) - 1;
}

float MaxAxisScale( const Matrix& m )
{
	return std::sqrt( std::max( { LengthSq( Vector3( m._11, m._12, m._13 ) ), LengthSq( Vector3( m._21, m._22, m._23 ) ), LengthSq( Vector3( m._31, m._32, m._33 ) ) } ) );
}
}

void EveSpaceObjectDecal::GetModularRenderables( std::vector<ITr2Renderable*>& renderables, const std::vector<EveChildGeometry>& parts, float objectScreenSize, float objectRadius, const TriGeometryRes* ownGeometry )
{
	if( !m_usePartTargets )
	{
		// switching from a single mesh parent: drop anything tied to that mesh
		m_usePartTargets = true;
		m_decalGeometry = nullptr;
		m_baseGeometryResource = nullptr;
		m_instanceData = nullptr;
		m_vertexDeclarationOverride = Tr2EffectStateManager::UNINITIALIZED_DECLARATION;
	}

	if( m_isVisible <= 0 )
	{
		return;
	}

	for( auto& target : m_partTargets )
	{
		target.seen = false;
	}

	for( const auto& part : parts )
	{
		TriGeometryRes* geometry = part.geometry;
		if( !geometry || !geometry->IsGood() || !geometry->GetMeshCount() )
		{
			continue;
		}
		TriGeometryResMeshData* meshData = geometry->GetMeshData( 0 );
		if( !meshData )
		{
			continue;
		}

		auto found = std::find_if( m_partTargets.begin(), m_partTargets.end(), [&]( const PartTarget& target ) {
			return !target.seen && target.geometry == geometry && target.childToObject == part.childToObject;
		} );
		if( found == m_partTargets.end() )
		{
			PartTarget target;
			target.geometry = geometry;
			target.childToObject = part.childToObject;
			// part vertices are in part space; the decal volume lives in object space
			target.decalMatrix = m_decalMatrix * Inverse( part.childToObject );
			target.inverseDecalMatrix = Inverse( target.decalMatrix );

			// cheap reject: does the decal volume touch the part's bounds at all?
			Vector3 decalMin( -1.f, -1.f, -1.f );
			Vector3 decalMax( 1.f, 1.f, 1.f );
			BoundingBoxTransform( decalMin, decalMax, target.decalMatrix );
			target.overlaps = decalMin.x <= meshData->m_maxBounds.x && decalMax.x >= meshData->m_minBounds.x &&
				decalMin.y <= meshData->m_maxBounds.y && decalMax.y >= meshData->m_minBounds.y &&
				decalMin.z <= meshData->m_maxBounds.z && decalMax.z >= meshData->m_minBounds.z;

			Vector3 boundsCenter = ( meshData->m_minBounds + meshData->m_maxBounds ) * 0.5f;
			target.lodRadius = Length( meshData->m_maxBounds - meshData->m_minBounds ) * 0.5f + Length( boundsCenter );
			// fallback before the first visibility update: scale the object's screen size by the part's size
			if( objectRadius > 0.f )
			{
				const Matrix& m = part.childToObject;
				float scale = std::max( { Length( Vector3( m._11, m._12, m._13 ) ),
										  Length( Vector3( m._21, m._22, m._23 ) ),
										  Length( Vector3( m._31, m._32, m._33 ) ) } );
				target.screenSizeScale = meshData->m_boundingSphere.w * scale / objectRadius;
			}
			m_partTargets.push_back( std::move( target ) );
			found = m_partTargets.end() - 1;
		}
		found->seen = true;

		if( !found->overlaps )
		{
			continue;
		}

		if( !found->decalGeometry || found->decalGeometry->m_lodMask != meshData->m_lodMask )
		{
			found->decalGeometry = BuildDecalGeometryCached( meshData, found->decalMatrix, found->inverseDecalMatrix );
		}
	}

	m_partTargets.erase( std::remove_if( m_partTargets.begin(), m_partTargets.end(), []( const PartTarget& target ) { return !target.seen; } ), m_partTargets.end() );

	bool anyVisible = false;
	for( auto& target : m_partTargets )
	{
		target.lodIndex = -1;
		if( !target.decalGeometry || !target.decalGeometry->m_indexBuffer.IsValid() )
		{
			continue;
		}
		int lodIndex = 0;
		if( m_isGeometryFrozen )
		{
			lodIndex = 0;
		}
		else if( target.geometry == ownGeometry )
		{
			// the object's own mesh draws at the object's screen size
			lodIndex = target.geometry->GetLodIndexForScreenSize( 0, objectScreenSize );
		}
		else if( m_hasLastFrustum )
		{
			// as EveChildInstancedMeshes sizes the instance's sphere and EveInstancedMeshManager picks its LOD
			Matrix world = target.childToObject * m_parentData.transform;
			float screenSize = m_lastFrustum.GetPixelSizeAccrossEst( world.GetTranslation(), target.lodRadius * MaxAxisScale( world ) ) * m_lastInvLodFactor;
			const TriGeometryResMeshData* meshData = target.geometry->GetMeshData( 0 );
			lodIndex = meshData ? InstancedMeshLod( *meshData, screenSize ) : -1;
		}
		else
		{
			lodIndex = target.geometry->GetLodIndexForScreenSize( 0, objectScreenSize * target.screenSizeScale );
		}
		if( lodIndex < 0 || lodIndex >= int( target.decalGeometry->m_lods.size() ) || !target.decalGeometry->m_lods[lodIndex].m_primitiveCount )
		{
			continue;
		}
		target.lodIndex = lodIndex;
		anyVisible = true;
	}

	if( anyVisible )
	{
		renderables.push_back( this );
	}
}

void EveSpaceObjectDecal::GetInstancedRenderables( std::vector<ITr2Renderable*>& renderables, DecalMeshCache& meshCache, const Tr2InstancedMesh* instancedMesh, float instanceScreenSize )
{
	GetRenderables( renderables, meshCache, instancedMesh->GetGeometryResource(), instanceScreenSize );
	m_instanceScreenSize = instanceScreenSize;
	m_instanceData = instancedMesh->GetInstanceGeometryResource();
	m_vertexDeclarationOverride = instancedMesh->GetVertexDeclaration();
	instancedMesh->GetBoundingBox( m_minBounds, m_maxBounds );
}


// --------------------------------------------------------------------------------
// Description:
//   decals are always transparent
// --------------------------------------------------------------------------------
bool EveSpaceObjectDecal::HasTransparentBatches()
{
	return true;
}

// --------------------------------------------------------------------------------
// Description:
//   Todo
// --------------------------------------------------------------------------------
void EveSpaceObjectDecal::GetBatches( ITriRenderBatchAccumulator* batches,
									  TriBatchType batchType,
									  const Tr2PerObjectData* perObjectData,
									  Tr2RenderReason reason )
{
	if( batchType != m_batchType )
	{
		return;
	}

	if( !m_decalEffect || !m_display )
	{
		return;
	}

	if( m_usePartTargets )
	{
		for( const auto& target : m_partTargets )
		{
			if( target.lodIndex < 0 || !target.decalGeometry || !target.decalGeometry->m_indexBuffer.IsValid() || !target.geometry->IsGood() )
			{
				continue;
			}
			const TriGeometryResLodData* lod = target.geometry->GetMeshLod( 0, target.lodIndex );
			if( !lod || !lod->m_allocationsValid )
			{
				continue;
			}
			auto partData = batches->Allocate<EveDecalPerObjectData>();
			if( !partData )
			{
				return;
			}
			// draw the part's vertices with the part's world transform and the decal volume in part space
			Matrix world = target.childToObject * m_parentData.transform;
			if( m_hasLastFrustum )
			{
				const Vector3& eye = m_lastFrustum.m_viewPos;
				world = world * TranslationMatrix( -eye ) * ScalingMatrix( 1.f - PART_DEPTH_PULL, 1.f - PART_DEPTH_PULL, 1.f - PART_DEPTH_PULL ) * TranslationMatrix( eye );
			}
			FillPerObjectData( *partData, world, target.decalMatrix, target.inverseDecalMatrix, IdentityMatrix() );
			CommitBatch( batches, lod, *target.decalGeometry, target.lodIndex, partData );
		}
		return;
	}

	if( !m_baseGeometryResource )
	{
		return;
	}

	if( !m_baseGeometryResource->IsGood() )
	{
		return;
	}

	if( m_baseGeometryResource->GetMeshCount() < 1 )
	{
		return;
	}
	if( !m_decalGeometry || !m_decalGeometry->m_indexBuffer.IsValid() )
	{
		return;
	}
	if( m_geometryLodIndex >= m_decalGeometry->m_lods.size() ||
		!m_decalGeometry->m_lods[m_geometryLodIndex].m_primitiveCount )
	{
		return;
	}

	const TriGeometryResLodData* lod = m_baseGeometryResource->GetMeshLod( 0, m_geometryLodIndex );
	if( !lod->m_allocationsValid )
	{
		return;
	}

	CommitBatch( batches, lod, *m_decalGeometry, m_geometryLodIndex, perObjectData );
}

void EveSpaceObjectDecal::CommitBatch( ITriRenderBatchAccumulator* batches, const TriGeometryResLodData* lod, const MeshDecalData& decalGeometry, int lodIndex, const Tr2PerObjectData* perObjectData )
{
	Tr2RenderBatch batch;
	batch.SetPriority( m_priority );
	batch.SetMaterial( m_decalEffect );
	auto declaration = m_vertexDeclarationOverride != Tr2EffectStateManager::UNINITIALIZED_DECLARATION ? m_vertexDeclarationOverride : lod->m_mesh->m_vertexDeclarationHandle;
	batch.SetGeometry( declaration, lod->m_vertexAllocation, decalGeometry.m_indexBuffer );

	batch.SetPerObjectData( perObjectData );

	if( m_instanceData )
	{
		ITr2InstanceData::InstanceData data = m_instanceData->GetInstanceData( 0, std::numeric_limits<float>::max() );
		batch.SetDrawIndexedInstanced(
			decalGeometry.m_lods[lodIndex].m_primitiveCount * 3,
			data.count,
			decalGeometry.m_indexBuffer.GetStartIndex() + decalGeometry.m_lods[lodIndex].m_startIndex,
			lod->m_vertexAllocation.GetOffset() / lod->m_vertexAllocation.GetStride(),
			data.offset / data.stride );
		batch.m_vertexStreams[1] = &data.buffer;
		batch.m_stride[1] = data.stride;
	}
	else
	{
		batch.SetDrawIndexedInstanced(
			decalGeometry.m_lods[lodIndex].m_primitiveCount * 3,
			1,
			decalGeometry.m_indexBuffer.GetStartIndex() + decalGeometry.m_lods[lodIndex].m_startIndex,
			lod->m_vertexAllocation.GetOffset() / lod->m_vertexAllocation.GetStride(),
			0 );
	}

	if( batch )
	{
		CCP_STATS_INC( decalDPCount );

		batches->Commit( batch );
	}
}

// --------------------------------------------------------------------------------
// Description:
//   ToDo
// --------------------------------------------------------------------------------
float EveSpaceObjectDecal::GetSortValue()
{
	return 1.f;
}

// --------------------------------------------------------------------------------
// Description:
//   ToDo
// --------------------------------------------------------------------------------
Tr2PerObjectData* EveSpaceObjectDecal::GetPerObjectData( ITriRenderBatchAccumulator* accumulator )
{
	// allocate only once
	auto perObjectData = accumulator->Allocate<EveDecalPerObjectData>();
	if( !perObjectData )
	{
		return NULL;
	}

	FillPerObjectData( *perObjectData, m_parentData.transform, m_decalMatrix, m_inverseDecalMatrix, m_parentBoneMatrix );
	return perObjectData;
}

void EveSpaceObjectDecal::FillPerObjectData( EveDecalPerObjectData& data, const Matrix& worldMatrix, const Matrix& decalMatrix, const Matrix& inverseDecalMatrix, const Matrix& parentBoneMatrix ) const
{
	// world matrix
	data.m_vsData.m_worldMatrix = Transpose( worldMatrix );
	// inv world matrix
	data.m_vsData.m_invWorldMatrix = Inverse( data.m_vsData.m_worldMatrix );

	// decal matrix (both nrm and inv)
	data.m_vsData.m_decalMatrix = Transpose( decalMatrix );
	data.m_vsData.m_inverseDecalMatrix = Transpose( inverseDecalMatrix );

	// matrix from possible bone animation of parent
	data.m_vsData.m_parentBoneMatrix = Transpose( parentBoneMatrix );
	data.m_vsData.m_invParentBoneMatrix = Inverse( Transpose( parentBoneMatrix ) );

	// clip sphere data from parent
	data.m_psData.m_shipData = m_parentData.shipData;
	data.m_psData.m_clipData = Vector4( m_parentData.clipSphereCenter, m_parentData.clipRadiusSq );
	data.m_psData.m_clipRadius2Sq = m_parentData.clipRadius2Sq;

	// display data
	data.m_psData.m_displayData = Vector4( (float)m_parentData.killCount, m_isVisible, 0.f, 0.f );

	if( m_parentData.shLighting )
	{
		memcpy( data.m_psData.m_shLightingCoefficients, m_parentData.shLighting, sizeof( data.m_psData.m_shLightingCoefficients ) );
	}
	else
	{
		memset( data.m_psData.m_shLightingCoefficients, 0, sizeof( data.m_psData.m_shLightingCoefficients ) );
	}
}

void EveSpaceObjectDecal::CopyFrom( EveSpaceObjectDecal* object )
{
	m_display = object->m_display;
	m_position = object->m_position;
	m_rotation = object->m_rotation;
	m_scaling = object->m_scaling;
	m_parentBoneIndex = object->m_parentBoneIndex;
	m_geometryLodIndex = object->m_geometryLodIndex;
	m_decalMatrix = object->m_decalMatrix;
	m_inverseDecalMatrix = object->m_inverseDecalMatrix;
	m_decalEffect = object->m_decalEffect;
	m_minScreenSize = object->m_minScreenSize;
}

// ------------------------------------------------------------------------------------------------------
void EveSpaceObjectDecal::RenderDebugInfo( ITr2DebugRenderer2& renderer, const Matrix& worldMatrix ) const
{
	Matrix worldDecalMatrix = m_parentBoneMatrix * worldMatrix;
	if( m_instanceData != nullptr )
	{
		if( Tr2RuntimeInstanceDataPtr runtimeInstanceData = BlueCastPtr( m_instanceData ) )
		{
			// do some magic for instanced meshes that have decals!
			const char* data = reinterpret_cast<const char*>( runtimeInstanceData->GetData() );
			if( data == 0 )
			{
				return;
			}
			unsigned int stride = runtimeInstanceData->GetStride();

			// need vertex declaration to get offset of position element in the vertex
			Tr2VertexDefinition decl;
			if( Tr2EffectStateManager::GetVertexDeclarationElements( runtimeInstanceData->GetInstanceBufferVertexDeclaration( 0 ), decl ) )
			{
				auto transform0 = decl.Find( decl.TEXCOORD, 0 );
				auto transform1 = decl.Find( decl.TEXCOORD, 1 );
				auto transform2 = decl.Find( decl.TEXCOORD, 2 );

				for( uint32_t p = 0; p < runtimeInstanceData->GetCount(); ++p )
				{
					// get the transposed instanced values and detranspose them!
					const Vector4* transform0Value = (const Vector4*)&data[p * stride + transform0->m_offset];
					const Vector4* transform1Value = (const Vector4*)&data[p * stride + transform1->m_offset];
					const Vector4* transform2Value = (const Vector4*)&data[p * stride + transform2->m_offset];
					Matrix m = {
						transform0Value->x,
						transform1Value->x,
						transform2Value->x,
						0.0,
						transform0Value->y,
						transform1Value->y,
						transform2Value->y,
						0.0,
						transform0Value->z,
						transform1Value->z,
						transform2Value->z,
						0.0,
						transform0Value->w,
						transform1Value->w,
						transform2Value->w,
						1.0,
					};

					auto instancedDecalMatrix = m_decalMatrix * m * worldDecalMatrix;

					renderer.DrawBox( this, instancedDecalMatrix, Vector3( -1, -1, -1 ), Vector3( 1, 1, 1 ), Tr2DebugRenderer::Wireframe, Tr2DebugColor( 0xff00ffff, 0x2200ffff ) );
					renderer.DrawBox( this, instancedDecalMatrix, Vector3( -1, -1, -1 ), Vector3( 1, 1, 1 ), Tr2DebugRenderer::Solid, 0 );
				}
			}
		}
	}
	else
	{
		worldDecalMatrix = m_decalMatrix * worldDecalMatrix;
		renderer.DrawBox( this, worldDecalMatrix, Vector3( -1, -1, -1 ), Vector3( 1, 1, 1 ), Tr2DebugRenderer::Wireframe, Tr2DebugColor( 0xff00ffff, 0x2200ffff ) );
		renderer.DrawBox( this, worldDecalMatrix, Vector3( -1, -1, -1 ), Vector3( 1, 1, 1 ), Tr2DebugRenderer::Solid, 0 );
	}
}

// --------------------------------------------------------------------------------------
// Description:
//   Here the parent has it's chance to give in a bone matrix, if the parent is
//   animated.
// Arguments:
//   bonesMatrices - a pointer to the bone-matrix array of the parent
//   bonesMatricesCount - the max numbers of bones in this array
// --------------------------------------------------------------------------------------
void EveSpaceObjectDecal::SetBoneMatrix( const Float4x3* bonesMatrices, int bonesMatricesCount )
{
	// sanity
	if( m_parentBoneIndex >= bonesMatricesCount )
	{
		return;
	}

	// -1 mean disabled
	if( m_parentBoneIndex == -1 )
	{
		return;
	}

	// keep it
	TriMatrixCopyFrom3x4( &m_parentBoneMatrix, &bonesMatrices[m_parentBoneIndex] );
}

// --------------------------------------------------------------------------------------
// Description:
//  Sets the main shader of this decal
// --------------------------------------------------------------------------------------
void EveSpaceObjectDecal::SetEffect( Tr2EffectPtr newEffect )
{
	m_decalEffect = newEffect;
}

void EveSpaceObjectDecal::SetShaderOption( const BlueSharedString& name, const BlueSharedString& value )
{
	if( nullptr != m_decalEffect )
	{
		m_decalEffect->SetOption( name, value );
	}
}

// --------------------------------------------------------------------------------------
// Description:
//  Sets the state for the geometry of this decal
// --------------------------------------------------------------------------------------
void EveSpaceObjectDecal::SetHighDetailDecalState( bool isFrozen )
{
	m_isGeometryFrozen = isFrozen;
}

// --------------------------------------------------------------------------------------
// Description:
//   Returns the center position of this decal
// --------------------------------------------------------------------------------------
const Vector3& EveSpaceObjectDecal::GetPosition() const
{
	return m_position;
}

// --------------------------------------------------------------------------------------
// Description:
//  Sets the center position of this decal
// --------------------------------------------------------------------------------------
void EveSpaceObjectDecal::SetPosition( const Vector3& pos )
{
	m_position = pos;
}

// --------------------------------------------------------------------------------------
// Description:
//   Returns the rotation of this decal
// --------------------------------------------------------------------------------------
const Quaternion& EveSpaceObjectDecal::GetRotation() const
{
	return m_rotation;
}

// --------------------------------------------------------------------------------------
// Description:
//  Sets the rotation of this decal
// --------------------------------------------------------------------------------------
void EveSpaceObjectDecal::SetRotation( const Quaternion& rot )
{
	m_rotation = rot;
}

// --------------------------------------------------------------------------------------
// Description:
//   Returns the scaling of this decal
// --------------------------------------------------------------------------------------
const Vector3& EveSpaceObjectDecal::GetScaling() const
{
	return m_scaling;
}

// --------------------------------------------------------------------------------------
// Description:
//  Sets the scaling of this decal
// --------------------------------------------------------------------------------------
void EveSpaceObjectDecal::SetScaling( const Vector3& sc )
{
	m_scaling = sc;
}

// --------------------------------------------------------------------------------------
// Description:
//   Returns the boneindex of this decal
// --------------------------------------------------------------------------------------
int EveSpaceObjectDecal::GetBoneIndex() const
{
	return m_parentBoneIndex;
}

// --------------------------------------------------------------------------------------
// Description:
//  Sets the boneindex of this decal
// --------------------------------------------------------------------------------------
void EveSpaceObjectDecal::SetBoneIndex( int idx )
{
	m_parentBoneIndex = idx;
}

// ------------------------------------------------------------------------------------------------------
void EveSpaceObjectDecal::CreateDecalIndexBuffers( TriGeometryResPtr geomRes, DecalMeshCache& meshCache )
{
	TRINITY_STATS_ZONE( __FUNCTION__ );

	// is loaded and good?
	if( !geomRes )
	{
		return;
	}

	if( !geomRes->IsGood() )
	{
		return;
	}

	if( !geomRes->GetMeshCount() )
	{
		return;
	}

	m_decalGeometry = nullptr;

	TriGeometryResMeshData* meshData = geomRes->GetMeshData( 0 );

	if( !meshData )
	{
		return;
	}

	for( auto& existingDecal : meshData->m_decals )
	{
		if( existingDecal->m_inverseDecalMatrix == m_inverseDecalMatrix )
		{
			m_decalGeometry = existingDecal;
			return;
		}
	}

	auto newDecal = BuildDecalGeometry( meshData, meshCache, m_decalMatrix, m_inverseDecalMatrix );
	if( !newDecal )
	{
		return;
	}

	meshData->m_decals.push_back( newDecal );
	m_decalGeometry = newDecal;
}

// ------------------------------------------------------------------------------------------------------
std::shared_ptr<MeshDecalData> EveSpaceObjectDecal::BuildDecalGeometry( TriGeometryResMeshData* meshData, DecalMeshCache& meshCache, const Matrix& decalMatrix, const Matrix& inverseDecalMatrix )
{
	TRINITY_STATS_ZONE( __FUNCTION__ );

	USE_MAIN_THREAD_RENDER_CONTEXT();

	std::vector<uint32_t> allIndices;

	std::shared_ptr<MeshDecalData> newDecal = std::make_shared<MeshDecalData>();
	newDecal->m_inverseDecalMatrix = inverseDecalMatrix;
	newDecal->m_lodMask = meshData->m_lodMask;

	Vector3 aabbMin( -1.f, -1.f, -1.f );
	Vector3 aabbMax( 1.f, 1.f, 1.f );
	BoundingBoxTransform( aabbMin, aabbMax, decalMatrix );

	// a decal volume outside the mesh bounds hits nothing: skip reading the buffers back
	if( aabbMin.x > meshData->m_maxBounds.x || aabbMax.x < meshData->m_minBounds.x ||
		aabbMin.y > meshData->m_maxBounds.y || aabbMax.y < meshData->m_minBounds.y ||
		aabbMin.z > meshData->m_maxBounds.z || aabbMax.z < meshData->m_minBounds.z )
	{
		newDecal->m_lods.resize( meshData->m_lods.size() );
		return newDecal;
	}

	// is vertex compressed to 16bit floats?
	bool vertexPositionCompressed = false;
	Tr2VertexDefinition vd;
	if( Tr2EffectStateManager::GetVertexDeclarationElements( meshData->m_vertexDeclarationHandle, vd ) )
	{
		if( auto pos = vd.Find( vd.POSITION ) )
		{
			vertexPositionCompressed = pos->m_dataType == vd.FLOAT16_4;
		}
	}

	for( int i = 0; i < static_cast<int>( meshData->m_lods.size() ); i++ )
	{
		TriGeometryResLodData* lod = meshData->m_lods[i].get();

		if( !lod->m_allocationsValid )
		{
			return nullptr;
		}

		// lock existing buffers, we do this only once and it is managed, so this call is ok
		const unsigned char* originalVertices = NULL;
		const unsigned char* originalIndices = NULL;

		bool readingBuffers = true;
		if( i < meshCache.buffers.size() )
		{
			originalVertices = meshCache.buffers[i].vertexBuffer.get();
			originalIndices = meshCache.buffers[i].indexBuffer.get();
			readingBuffers = false;
		}

		if( readingBuffers )
		{
			if( FAILED( lod->m_vertexAllocation.MapForReading( originalVertices, renderContext ) ) )
			{
				return nullptr;
			}
		}
		ON_BLOCK_EXIT( [&] { if( readingBuffers ) lod->m_vertexAllocation.UnmapForReading( renderContext ); } );

		if( readingBuffers )
		{
			if( FAILED( lod->m_indexAllocation.MapForReading( originalIndices, renderContext ) ) )
			{
				return nullptr;
			}
		}
		ON_BLOCK_EXIT( [&] { if( readingBuffers ) lod->m_indexAllocation.UnmapForReading( renderContext ); } );

		if( i == meshCache.buffers.size() )
		{
			DecalMeshCache::MeshBuffers buffers;
			buffers.vertexBuffer.reset( new uint8_t[lod->m_vertexAllocation.GetSize()] );
			memcpy( buffers.vertexBuffer.get(), originalVertices, lod->m_vertexAllocation.GetSize() );
			buffers.indexBuffer.reset( new uint8_t[lod->m_indexAllocation.GetSize()] );
			memcpy( buffers.indexBuffer.get(), originalIndices, lod->m_indexAllocation.GetSize() );
			meshCache.buffers.push_back( std::move( buffers ) );
		}

		// correct source pointers
		const unsigned int* indices32 = reinterpret_cast<const unsigned int*>( originalIndices );
		const unsigned short* indices16 = reinterpret_cast<const unsigned short*>( originalIndices );
		bool is16bit = lod->m_indexAllocation.GetStride() == 2;

		MeshDecalLodData buffer;
		buffer.m_startIndex = uint32_t( allIndices.size() );

		for( unsigned int t = 0; t < lod->m_primitiveCount; ++t )
		{
			// get triangle indices
			unsigned int index0 = is16bit ? indices16[0] : indices32[0];
			unsigned int index1 = is16bit ? indices16[1] : indices32[1];
			unsigned int index2 = is16bit ? indices16[2] : indices32[2];
			// next!
			indices16 += 3;
			indices32 += 3;

			// do real collision detection here
			Vector3 pos[3];
			if( vertexPositionCompressed )
			{
				pos[0] = *reinterpret_cast<const Vector3_16*>( originalVertices + index0 * meshData->m_bytesPerVertex );
				pos[1] = *reinterpret_cast<const Vector3_16*>( originalVertices + index1 * meshData->m_bytesPerVertex );
				pos[2] = *reinterpret_cast<const Vector3_16*>( originalVertices + index2 * meshData->m_bytesPerVertex );
			}
			else
			{
				pos[0] = *reinterpret_cast<const Vector3*>( originalVertices + index0 * meshData->m_bytesPerVertex );
				pos[1] = *reinterpret_cast<const Vector3*>( originalVertices + index1 * meshData->m_bytesPerVertex );
				pos[2] = *reinterpret_cast<const Vector3*>( originalVertices + index2 * meshData->m_bytesPerVertex );
			}
			if( IntersectTriangleAABB( pos, pos + 1, pos + 2, aabbMin, aabbMax ) &&
				IntersectTriangleOrientedBox( pos, pos + 1, pos + 2, inverseDecalMatrix ) )
			{
				allIndices.push_back( index0 );
				allIndices.push_back( index1 );
				allIndices.push_back( index2 );
			}
		}

		// primitive count; the decal index buffer is always 32 bit
		buffer.m_primitiveCount = ( uint32_t( allIndices.size() ) - buffer.m_startIndex ) / 3;
		newDecal->m_lods.push_back( buffer );
	}

	// dont create empty buffer
	if( !allIndices.empty() )
	{
		if( !SUCCEEDED( g_sharedBuffer.Allocate( 4, uint32_t( allIndices.size() ), allIndices.data(), renderContext, newDecal->m_indexBuffer ) ) )
		{
			return nullptr;
		}
	}

	return newDecal;
}

// ------------------------------------------------------------------------------------------------------
std::shared_ptr<MeshDecalData> EveSpaceObjectDecal::BuildDecalGeometryCached( TriGeometryResMeshData* meshData, const Matrix& decalMatrix, const Matrix& inverseDecalMatrix )
{
	TRINITY_STATS_ZONE( __FUNCTION__ );

	std::shared_ptr<MeshDecalData> newDecal = std::make_shared<MeshDecalData>();
	newDecal->m_inverseDecalMatrix = inverseDecalMatrix;
	newDecal->m_lodMask = meshData->m_lodMask;

	Vector3 aabbMin( -1.f, -1.f, -1.f );
	Vector3 aabbMax( 1.f, 1.f, 1.f );
	BoundingBoxTransform( aabbMin, aabbMax, decalMatrix );

	// a decal volume outside the mesh bounds hits nothing: don't build the cache for it
	if( aabbMin.x > meshData->m_maxBounds.x || aabbMax.x < meshData->m_minBounds.x ||
		aabbMin.y > meshData->m_maxBounds.y || aabbMax.y < meshData->m_minBounds.y ||
		aabbMin.z > meshData->m_maxBounds.z || aabbMax.z < meshData->m_minBounds.z )
	{
		newDecal->m_lods.resize( meshData->m_lods.size() );
		return newDecal;
	}

	const DecalTriangleCache* cache = DecalTriangleCache::Get( meshData );
	if( !cache )
	{
		return nullptr;
	}

	// faces more than ~66 degrees off the projection axis would smear the decal's texture across
	// them; leaving them out still lets a decal on an edge (45 degrees to each face) wrap it
	constexpr float MIN_FACING = 0.4f;
	Vector3 projectionAxis( decalMatrix._11, decalMatrix._12, decalMatrix._13 );
	float axisLength = Length( projectionAxis );
	projectionAxis = axisLength > 0.f ? projectionAxis / axisLength : Vector3( 1.f, 0.f, 0.f );

	std::vector<uint32_t> allIndices;
	for( size_t i = 0; i < meshData->m_lods.size(); ++i )
	{
		MeshDecalLodData lod;
		lod.m_startIndex = uint32_t( allIndices.size() );
		cache->Query( i, aabbMin, aabbMax, inverseDecalMatrix, projectionAxis, MIN_FACING, allIndices );
		lod.m_primitiveCount = ( uint32_t( allIndices.size() ) - lod.m_startIndex ) / 3;
		newDecal->m_lods.push_back( lod );
	}

	if( !allIndices.empty() )
	{
		USE_MAIN_THREAD_RENDER_CONTEXT();
		if( !SUCCEEDED( g_sharedBuffer.Allocate( 4, uint32_t( allIndices.size() ), allIndices.data(), renderContext, newDecal->m_indexBuffer ) ) )
		{
			return nullptr;
		}
	}
	return newDecal;
}

void EveSpaceObjectDecal::SetIndices( const std::vector<std::vector<uint32_t>>& indices )
{
	m_decalGeometry = nullptr;
	m_staticIndexBuffers = indices;
}

std::vector<std::vector<uint32_t>> EveSpaceObjectDecal::GetStaticIndexBuffers() const
{
	return m_staticIndexBuffers;
}

void EveSpaceObjectDecal::SetMinScreenSize( float minScreenSize )
{
	m_minScreenSize = minScreenSize;
}

void EveSpaceObjectDecal::CreateStaticIndexBuffers( TriGeometryResPtr geomRes )
{
	TRINITY_STATS_ZONE( __FUNCTION__ );

	USE_MAIN_THREAD_RENDER_CONTEXT();

	// is loaded and good?
	if( !geomRes )
	{
		return;
	}

	if( !geomRes->IsGood() )
	{
		return;
	}

	if( !geomRes->GetMeshCount() )
	{
		return;
	}

	TriGeometryResMeshData* originalMeshData = geomRes->GetMeshData( 0 );
	for( auto& existingDecal : originalMeshData->m_decals )
	{
		if( existingDecal->m_inverseDecalMatrix == m_inverseDecalMatrix )
		{
			m_decalGeometry = existingDecal;
			return;
		}
	}

	std::shared_ptr<MeshDecalData> newDecal = std::make_shared<MeshDecalData>();
	newDecal->m_inverseDecalMatrix = m_inverseDecalMatrix;
	newDecal->m_lodMask = originalMeshData->m_lodMask;

	uint32_t total = 0;
	for( auto& lod : originalMeshData->m_lods )
	{
		if( lod->m_originalLodIndex >= m_staticIndexBuffers.size() )
		{
			// LOD generation may completely remove some parts of the model.
			// If we have a decal on that part, it will suddenly have an empty index buffer.
			// The precomputed decal LODs will omit LODs with an empty index buffer.
			// Therefore, this case is a possibility and can be safely ignored.
			break;
		}
		auto& buffer = m_staticIndexBuffers[lod->m_originalLodIndex];

		MeshDecalLodData decalLod;
		decalLod.m_startIndex = total;
		decalLod.m_primitiveCount = uint32_t( buffer.size() / 3 );

		newDecal->m_lods.push_back( decalLod );

		total += uint32_t( buffer.size() );
	}

	std::vector<uint32_t> indices;
	indices.reserve( total );

	for( auto& lod : originalMeshData->m_lods )
	{
		if( lod->m_originalLodIndex >= m_staticIndexBuffers.size() )
		{
			break;
		}
		auto& buffer = m_staticIndexBuffers[lod->m_originalLodIndex];

		indices.insert( end( indices ), begin( buffer ), end( buffer ) );
	}
	if( !SUCCEEDED( g_sharedBuffer.Allocate( 4, uint32_t( indices.size() ), indices.data(), renderContext, newDecal->m_indexBuffer ) ) )
	{
		return;
	}
	originalMeshData->m_decals.push_back( newDecal );
	m_decalGeometry = newDecal;
}

bool EveSpaceObjectDecal::HasStaticIndexBuffers() const
{
	for( const auto& buffer : m_staticIndexBuffers )
	{
		if( !buffer.empty() )
		{
			return true;
		}
	}

	return false;
}

void EveSpaceObjectDecal::GetPickingBatches( ITriRenderBatchAccumulator* batches, Tr2PickTypes pickTypes, const Tr2PerObjectData* perObjectData )
{
	if( ( pickTypes & PICK_TYPE_ATTACHMENTS ) == 0 )
	{
		return;
	}
	GetBatches( batches, m_batchType, perObjectData );
}

std::vector<uint32_t> EveSpaceObjectDecal::GetDecalPrimitiveCounts() const
{
	std::vector<uint32_t> counts;
	if( m_usePartTargets )
	{
		// per LOD, summed over all the parts the decal projects onto
		for( const auto& target : m_partTargets )
		{
			if( !target.decalGeometry )
			{
				continue;
			}
			const auto& lods = target.decalGeometry->m_lods;
			if( counts.size() < lods.size() )
			{
				counts.resize( lods.size(), 0 );
			}
			for( size_t i = 0; i < lods.size(); ++i )
			{
				counts[i] += lods[i].m_primitiveCount;
			}
		}
	}
	else if( m_decalGeometry )
	{
		for( auto& buffer : m_decalGeometry->m_lods )
		{
			counts.push_back( buffer.m_primitiveCount );
		}
	}
	else if( m_baseGeometryResource )
	{
		// no geometry yet: report what the static index buffers hold
		TriGeometryResMeshData* meshData = m_baseGeometryResource->GetMeshData( 0 );

		if( meshData )
		{
			for( auto& lod : meshData->m_lods )
			{
				if( lod->m_originalLodIndex >= m_staticIndexBuffers.size() )
				{
					break;
				}
				auto& buffer = m_staticIndexBuffers[lod->m_originalLodIndex];
				counts.push_back( uint32_t( buffer.size() / 3 ) );
			}
		}
	}
	return counts;
}

void EveSpaceObjectDecal::SetBatchType( TriBatchType batchType )
{
	m_batchType = batchType;
}

void EveSpaceObjectDecal::SetPriority( uint32_t priority )
{
	m_priority = priority;
}

// --------------------------------------------------------------------------------
// Description:
//   Copy all the matrices to HW
// --------------------------------------------------------------------------------
void EveDecalPerObjectData::SetPerObjectDataToDevice( Tr2ConstantBufferAL** buffers, unsigned constantTypeMask, Tr2RenderContext& renderContext ) const
{
	// add up constant count, see EveDecalPerObjectData
	FillAndSetConstants( *buffers[VERTEX_SHADER], &m_vsData, sizeof( DecalVSPerObjectData ), VERTEX_SHADER, Tr2Renderer::GetPerObjectVSStartRegister(), renderContext );
	FillAndSetConstants( *buffers[PIXEL_SHADER], &m_psData, sizeof( DecalPSPerObjectData ), PIXEL_SHADER, Tr2Renderer::GetPerObjectPSStartRegister(), renderContext );
}

void EveDecalPerObjectData::ApplyConstantBuffers( Tr2IndirectDrawBufferWriter& writer, Tr2RenderContext& renderContext ) const
{
	writer.SetPerObjectData( VERTEX_SHADER, &m_vsData, sizeof( DecalVSPerObjectData ) );
	writer.SetPerObjectData( PIXEL_SHADER, &m_psData, sizeof( DecalPSPerObjectData ) );
}