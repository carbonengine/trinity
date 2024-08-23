#include "StdAfx.h"
#include "EveChildMesh.h"
#include "Tr2MeshBase.h"
#include "TriFrustum.h"
#include "Eve/SpaceObject/EveSpaceObject2.h"
#include "Eve/EveTransform.h"
#include "Utilities/BoundingSphere.h"
#include "Tr2InstancedMesh.h"
#include "Tr2GrannyAnimation.h"
#include "TriFrustumOrtho.h"
#include "Utilities/BoundingBox.h"

extern float g_eveSpaceSceneLODFactor;
extern float g_eveSpaceSceneVisibilityThreshold;

namespace
{
constexpr float s_instanceScreenSizeThreshold = 1.f;

}

EveChildMesh::EveChildMesh( IRoot* lockobj ):
	PARENTLOCK( m_transformModifiers ),
	PARENTLOCK( m_decals ),
	PARENTLOCK( m_attachments ),
	PARENTLOCK( m_lights ),
	m_display( true ),
	m_isVisible( false ),
	m_instancesVisible( false ),
	m_castShadow( false ),
	m_lowestLodVisible( TR2_LOD_LOW ),
	m_minScreenSize( 0.f ),
	m_currentScreenSize( -1.f ),
	m_currentInstanceScreenSize( -1.f ),
	m_sortValueOffset( 0 ),
	m_sortValueScale( 1 ),
	m_useSpaceObjectData( true ),
	m_activationStrength( 1.0f ),
	m_origin( SPACE ),
	m_reflectionMode( EntityComponents::REFLECT_NEVER ),
	EveChildTransform(),
	EveEntity( lockobj )
{
	// init per-object data with default values
	memset( &m_vsData, 0, sizeof( EveSpaceObjectVSData ) );
	memset( &m_psData, 0, sizeof( EveSpaceObjectPSData ) );
	m_vsData.shipData.y = 1.f;
	m_vsData.shipData.w = 1.f;
	m_psData.shipData.y = 1.f;
	m_psData.shipData.w = 1.f;
	m_psData.screenSize = Vector4( 0.5f, 0.5f, 0.5f, 1.f );
}

EveChildMesh::~EveChildMesh()
{
}

bool EveChildMesh::Initialize()
{
	if( m_staticTransform )
	{
		RebuildLocalTransform();
	}

	InitializeAnimation();

	return true;
}

bool EveChildMesh::OnModified( Be::Var* val )
{
	if( IsMatch( val, m_reflectionMode ) || IsMatch( val, m_display) || IsMatch( val, m_mesh) || IsMatch( val, m_castShadow ) )
	{
		ReRegister();
	}
	if( IsMatch( val, m_mesh ) || IsMatch( val, m_animationUpdater ) )
	{
		InitializeAnimation();
	}
	return true;
}

const char* EveChildMesh::GetName() const
{
	return m_name.c_str();
}

void EveChildMesh::SetName( const char* name )
{
	m_name = BlueSharedString( name );
}

void EveChildMesh::InitializeAnimation()
{
	if( m_animationUpdater && m_animationUpdater->GetResPath().empty() )
	{
		if( m_mesh )
		{
			if( auto geometry = m_mesh->GetGeometryResource() )
			{
				m_animationUpdater->SetUseMeshBinding( true );
				m_animationUpdater->SetSharedGeometryRes( geometry );
				return;
			}
		}
		m_animationUpdater->SetSharedGeometryRes( nullptr );
	}
}


// --------------------------------------------------------------------------------
// Description:
//    Registers itself and its children with the scene registration container.
//    This is so we don't have to traverse the tree every frame
// --------------------------------------------------------------------------------
void EveChildMesh::RegisterComponents()
{
	auto registry = this->GetComponentRegistry();
	if( registry && m_display && m_mesh != nullptr )
	{
		if( EntityComponents::ShouldReflect( m_reflectionMode ) )
		{
			registry->RegisterComponent<ITr2Renderable>( this );
		}
		if( m_castShadow )
		{
			registry->RegisterComponent<IEveShadowCaster>( this );
		}
	}
}

// --------------------------------------------------------------------------------
// Description:
//   Check if the object is casting a shadow in the camera/shadow frustums
bool EveChildMesh::IsCastingShadow( const TriFrustum& cameraFrustum, const TriFrustumOrtho& shadowFrustum, const uint32_t shadowMapSize, const Vector3 sunDir, float& sizeInShadow ) const
{
	if( !m_display || !m_castShadow )
	{
		return false;
	}

	Vector4 bs;
	GetBoundingSphere( bs );
	sizeInShadow = 0;

	if( bs.w <= 0.0f )
	{
		return false;
	}

	if( EveShadowCaster::IsVisible( cameraFrustum, shadowFrustum, sunDir, bs ) )
	{
		if( Tr2InstancedMeshPtr instanced = BlueCastPtr( m_mesh ) )
		{
			if( auto instanceBounds = instanced->GetInstanceBoundsClosestToPoint( TransformCoord( shadowFrustum.GetEyePos(), Inverse( m_worldTransform ) ) ) )
			{
				instanceBounds.Transform( m_worldTransform );

				sizeInShadow = EveShadowCaster::GetSizeInShadow( shadowFrustum, shadowMapSize, Vector4( instanceBounds.center, instanceBounds.radius ) );
			}
			else
			{
				sizeInShadow = EveShadowCaster::GetSizeInShadow( shadowFrustum, shadowMapSize, bs );
			}
		}
		else
		{
			sizeInShadow = EveShadowCaster::GetSizeInShadow( shadowFrustum, shadowMapSize, bs );	
		}
	}
	return sizeInShadow > 5.f;
}

void EveChildMesh::UpdateVisibility( const TriFrustum& frustum, const Matrix& parentTransform, Tr2Lod parentLod )
{
	m_isVisible = false;
	m_currentScreenSize = -1;
	m_instancesVisible = false;
	m_currentInstanceScreenSize = -1.0f;

	if( m_mesh )
	{
		auto bounds = m_mesh->GetBounds();
		bounds.Transform( m_worldTransform );

		m_currentScreenSize = frustum.GetPixelSizeAccross( bounds );

		if( Tr2InstancedMeshPtr instanced = BlueCastPtr( m_mesh ) )
		{
			if( auto instanceBounds = instanced->GetInstanceBoundsClosestToPoint( TransformCoord( frustum.m_viewPos, Inverse( m_worldTransform ) ) ) )
			{
				instanceBounds.Transform( m_worldTransform );
				m_currentInstanceScreenSize = frustum.GetPixelSizeAccross( instanceBounds );
				m_mesh->UseWithScreenSize( m_currentInstanceScreenSize, instanceBounds.radius );
			}
			else
			{
				m_currentInstanceScreenSize = std::numeric_limits<float>::max();
				m_mesh->UseWithScreenSize( m_currentScreenSize, CcpMath::Sphere( bounds ).radius );
			}
		}
		else
		{
			m_currentInstanceScreenSize = std::numeric_limits<float>::max();
			m_mesh->UseWithScreenSize( m_currentScreenSize, CcpMath::Sphere( bounds ).radius );
		}

		m_currentScreenSize /= g_eveSpaceSceneLODFactor;
		m_currentInstanceScreenSize /= g_eveSpaceSceneLODFactor;

		if( frustum.IsBoxVisible( bounds ) )
		{
			m_isVisible = parentLod >= m_lowestLodVisible && m_currentScreenSize >= m_minScreenSize;
			m_instancesVisible = m_isVisible && m_currentInstanceScreenSize >= s_instanceScreenSizeThreshold;
		}
	}

	if( !m_attachments.empty() )
	{
		size_t boneCount = 0;
		const granny_matrix_3x4* bones = nullptr;
		Tr2GrannyAnimationUtils::GetBoneList( m_animationUpdater, bones, boneCount );

		for( auto it = begin( m_attachments ); it != end( m_attachments ); ++it )
		{
			( *it )->UpdateVisibility( frustum, m_worldTransform, bones, boneCount );
		}
	}

	if( m_isVisible )
	{
		for( auto it = m_decals.begin(); it != m_decals.end(); ++it )
		{
			if( m_animationUpdater && m_animationUpdater->GetMeshBoneCount() && m_animationUpdater->IsInitialized() )
			{
				( *it )->SetBoneMatrix( m_animationUpdater->GetMeshBoneMatrixList(), m_animationUpdater->GetMeshBoneCount() );
			}
			( *it )->UpdateVisibility( frustum, &m_parentData );
		}
	}
}

void EveChildMesh::GetRenderables( std::vector<ITr2Renderable*>& renderables )
{
	if( m_isVisible )
	{
		if( Tr2InstancedMeshPtr instanced = BlueCastPtr( m_mesh ) ) 
		{
			if( m_instancesVisible )
			{
				renderables.push_back( this );

				TriGeometryResPtr geometryRes = m_mesh->GetGeometryResource();

				if( geometryRes )
				{
					// runn over every decal and update it
					for( EveSpaceObjectDecalVector::const_iterator it = m_decals.begin(); it != m_decals.end(); ++it )
					{
						// now prep to get the renderables
						( *it )->GetInstancedRenderables( renderables, instanced, m_currentInstanceScreenSize );
					}
				}
			}
		}
		else
		{
			renderables.push_back( this );
			TriGeometryResPtr geometryRes = m_mesh->GetGeometryResource();

			if( geometryRes )
			{
				// runn over every decal and update it
				for( EveSpaceObjectDecalVector::const_iterator it = m_decals.begin(); it != m_decals.end(); ++it )
				{
					// now prep to get the renderables
					( *it )->GetRenderables( renderables, geometryRes, m_currentScreenSize );
				}
			}
		}
	}
}

bool EveChildMesh::GetBoundingSphere( Vector4& sphere, BoundingSphereQuery query ) const
{
	if( m_mesh )
	{
		if( auto aabb = m_mesh->GetBounds() )
		{
			auto s = CcpMath::Sphere( aabb );
			s.Transform( m_worldTransform );
			sphere = Vector4( s.center, s.radius );
			return true;
		}
	}

	return false;
}

bool EveChildMesh::HasTransparentBatches()
{
	if( m_display && m_mesh )
	{
		return !(m_mesh->GetAreas( TRIBATCHTYPE_TRANSPARENT )->empty());
	}

	return false;
}

bool EveChildMesh::IsVisible( const TriFrustum& frustum ) const
{
	Vector4 boundingSphere;

	if( GetBoundingSphere( boundingSphere ) && boundingSphere.w != 0)
	{
		if( frustum.IsSphereVisible( boundingSphere.GetXYZ(), boundingSphere.w ) )
		{
			return frustum.GetPixelSizeAccrossEst( boundingSphere.GetXYZ(), boundingSphere.w ) >= g_eveSpaceSceneVisibilityThreshold;
		}
	}
	return false;
}

void EveChildMesh::GetBatches( ITriRenderBatchAccumulator* batches, TriBatchType batchType, const Tr2PerObjectData* perObjectData, Tr2RenderReason reason )
{
	if( m_display )
	{
		if( m_mesh )
		{
			m_mesh->GetBatches( batches, m_mesh->GetAreas( batchType ), perObjectData, min( m_currentInstanceScreenSize, m_currentScreenSize ) );
		}
		
		if( m_activationStrength != 0.0 )
		{
			for( auto it = begin( m_attachments ); it != end( m_attachments ); ++it )
			{
				( *it )->GetBatches( batches, batchType, perObjectData, reason );
			}
		}	
	}
}

void EveChildMesh::GetShadowBatches( ITriRenderBatchAccumulator* batches, const Tr2PerObjectData* perObjectData, float shadowPixelSize )
{
	// TODO: Figure out what we want to do with shadows
	// Fix asap <Logi 27. aug 2015>
	if( m_display && m_mesh )
	{
		m_mesh->GetBatches( batches, m_mesh->GetAreas( TRIBATCHTYPE_OPAQUE ), perObjectData, shadowPixelSize );
	}
}

float EveChildMesh::GetSortValue()
{
	Vector3 d = Tr2Renderer::GetViewPosition() - m_worldTransform.GetTranslation();
	float distance = Length( d );
	return distance * m_sortValueScale + m_sortValueOffset;
}

Tr2PerObjectData* EveChildMesh::GetShadowPerObjectData( ITriRenderBatchAccumulator* accumulator )
{
	return GetPerObjectData( accumulator );
}

Tr2PerObjectData* EveChildMesh::GetPerObjectData( ITriRenderBatchAccumulator* accumulator )
{
	if( !m_useSpaceObjectData )
	{
		EveBasicPerObjectData* perObjectData = accumulator->Allocate<EveBasicPerObjectData>();

		if( !perObjectData )
		{
			return nullptr;
		}

		perObjectData->m_world = m_vsData.worldTransform;
		perObjectData->m_worldInverseTranspose = Inverse( m_worldTransform );
		return perObjectData;
	}

	Tr2PerObjectDataWithPersistentBuffers<EveChildMesh>* perObjectData = accumulator->Allocate<Tr2PerObjectDataWithPersistentBuffers<EveChildMesh>>();
	if( !perObjectData )
	{
		return nullptr;
	}
	perObjectData->Initialize( this, &m_perObjectDataVs, &m_perObjectDataPs );

	return perObjectData;
}

uint32_t EveChildMesh::GetPerObjectDataSize( Tr2RenderContextEnum::ShaderType shaderType ) const
{
	if( shaderType == Tr2RenderContextEnum::PIXEL_SHADER )
	{
		return sizeof( m_psData );
	}
	else
	{
		int boneCount = 0;
		if( m_animationUpdater && m_animationUpdater->IsInitialized() )
		{
			boneCount = m_animationUpdater->GetMeshBoneCount();
		}

		return sizeof( m_vsData ) + boneCount * 3 * 16; // m_vsBonesMatrix (3x4)
	}
}

void EveChildMesh::UpdatePerObjectBuffer( Tr2RenderContextEnum::ShaderType shaderType, uint32_t size, void* data )
{
	if( shaderType == Tr2RenderContextEnum::PIXEL_SHADER )
	{
		uint8_t* perObjectPS = (uint8_t*)data;
		memcpy( perObjectPS, &m_psData, sizeof( m_psData ) );
	}
	else
	{
		uint8_t* perObjectVS = (uint8_t*)data;
		memcpy( perObjectVS, &m_vsData, sizeof( m_vsData ) );
		perObjectVS += sizeof( m_vsData );

		size -= sizeof( m_vsData );
		if( size )
		{
			memcpy( perObjectVS, m_animationUpdater->GetMeshBoneMatrixList(), size );
		}
	}
}

void EveChildMesh::UpdateAsyncronous( EveUpdateContext& updateContext, const EveChildUpdateParams& params )
{
	m_perObjectDataVs.InvalidateBufferData();
	m_perObjectDataPs.InvalidateBufferData();

	Matrix localToWorldTransform;
	Matrix lastWorldTransform = m_worldTransform;
	
	if( nullptr != params.childParent )
	{
		params.childParent->GetLocalToWorldTransform( localToWorldTransform );
	}
	else if( nullptr != params.spaceObjectParent )
	{
		params.spaceObjectParent->GetLocalToWorldTransform( localToWorldTransform );
	}
	else 
	{
		localToWorldTransform = params.localToWorldTransform;
	}

	UpdateTransform( localToWorldTransform );
	for( auto it = m_transformModifiers.begin(); it != m_transformModifiers.end(); it++ )
	{
		m_worldTransform = (*it)->ApplyTransform( m_worldTransform, params.boneCount, params.bones );
	}

	// need to update the data we get from the parent to be relevant to us!
	if( nullptr != params.spaceObjectParent )
	{
		params.spaceObjectParent->GetPerObjectStructs( m_vsData, m_psData );
		params.spaceObjectParent->GetParentData( &m_parentData );

		// need to move the clipdata inversely of the translation of the childmesh
		m_vsData.clipData = Vector4( m_vsData.clipData.GetXYZ() - m_translation, m_vsData.clipData.w );
		m_psData.clipData = Vector4( m_psData.clipData.GetXYZ() - m_translation, m_psData.clipData.w );
		
		// update the world transform of the parent
		m_parentData.transform = m_worldTransform;
	}

	m_activationStrength = params.activationStrength;

	m_vsData.worldTransform = Transpose( m_worldTransform );
	m_vsData.invWorldTransform = Inverse( m_worldTransform );
	m_vsData.worldTransformLast = Transpose( lastWorldTransform );

	// Normalize screenSize dimensions
	auto screen_width = Tr2Renderer::GetViewport().width;
	auto screen_height = Tr2Renderer::GetViewport().height;
	m_psData.screenSize.x = min( float( m_currentScreenSize / screen_width ), 1.0f );
	m_psData.screenSize.y = min( float( m_currentScreenSize / screen_height ), 1.0f );
}

void EveChildMesh::UpdateSyncronous( EveUpdateContext& updateContext, const EveChildUpdateParams& )
{
	if( m_animationUpdater )
	{
		if( m_mesh && m_animationUpdater->GetResPath().empty() )
		{
			if( m_mesh->GetGeometryResource() != m_animationUpdater->GetSharedGeometryRes() )
			{
				InitializeAnimation();
			}
		}

		m_animationUpdater->PrePhysicsAnimation( 0, IdentityMatrix() );
	}
}

void EveChildMesh::GetLocalToWorldTransform( Matrix& transform ) const
{
	transform = m_worldTransform;
}

void EveChildMesh::ChangeLOD( Tr2Lod )
{
}

void EveChildMesh::SetMesh( Tr2MeshBase* mesh )
{
	m_mesh = mesh;
}

void EveChildMesh::SetOrigin( Origin origin )
{
	m_origin = origin;
}

// --------------------------------------------------------------------------------
// Description:
//   Setup function to set data from outside, in this case just pass it to
//   function of base class
// --------------------------------------------------------------------------------
void EveChildMesh::Setup( const Vector3* scale, const Quaternion* rotation, const Vector3* translation, Tr2Lod lowestLodVisible )
{
	// call base class's setup
	EveChildTransform::Setup( scale, rotation, translation, lowestLodVisible );

	// and remember lodding!
	m_lowestLodVisible = lowestLodVisible;
}

bool EveChildMesh::IsAlwaysOn() const
{
	return true;
}

void EveChildMesh::SetShaderOption( const BlueSharedString& name, const BlueSharedString& value )
{
	if ( nullptr != m_mesh )
	{
		m_mesh->SetShaderOption( name, value );
	}

	for( EveSpaceObjectDecalVector::iterator it = m_decals.begin(); it != m_decals.end(); ++it )
	{
		( *it )->SetShaderOption( name, value );
	}

	for( auto it = m_attachments.begin(); it != m_attachments.end(); ++it )
	{
		IEveSpaceObjectAttachment* attachment = *it;
		attachment->SetShaderOption( name, value );
	}
}

void EveChildMesh::SetScale( const Vector3& scale )
{
	m_scaling = scale;
}

void EveChildMesh::GetDebugOptions( Tr2DebugRendererOptions& options )
{
	if( m_mesh )
	{
		m_mesh->GetDebugOptions( options );
	}
	options.insert( "Bones" );
	options.insert( "Decals" );
	options.insert( "Lights" );

	for( auto it = begin( m_attachments ); it != end( m_attachments ); ++it )
	{
		( *it )->GetDebugOptions( options );
	}
}

void EveChildMesh::RenderDebugInfo( ITr2DebugRenderer2& renderer )
{
	if( !m_display )
	{
		return;
	}
	if( m_mesh )
	{
		m_mesh->RenderDebugInfo( m_worldTransform, renderer );
	}
	if( m_animationUpdater && renderer.HasOption( GetRawRoot(), "Bones" ) )
	{
		m_animationUpdater->RenderBones( m_worldTransform );
	}

	if( renderer.HasOption( GetRawRoot(), "Decals" ) )
	{
		for( EveSpaceObjectDecalVector::iterator it = m_decals.begin(); it != m_decals.end(); ++it )
		{
			( *it )->RenderDebugInfo( renderer, m_worldTransform );
		}
	}

	if( !m_attachments.empty() )
	{
		size_t boneCount = 0;
		const granny_matrix_3x4* bones = nullptr;
		Tr2GrannyAnimationUtils::GetBoneList( m_animationUpdater, bones, boneCount );

		for( auto it = begin( m_attachments ); it != end( m_attachments ); ++it )
		{
			( *it )->RenderDebugInfo( renderer, m_worldTransform, bones, boneCount );
		}
	}

	if( renderer.HasOption( GetRawRoot(), "Lights" ) )
	{
		size_t boneCount = 0;
		const granny_matrix_3x4* bones = nullptr;
		Tr2GrannyAnimationUtils::GetBoneList( m_animationUpdater, bones, boneCount );
		for( auto it = m_lights.begin(); it != m_lights.end(); ++it )
		{
			( *it )->RenderDebugInfo( renderer, m_worldTransform, bones, boneCount );
		}
	}
}

void EveChildMesh::AddTransformModifier( IEveChildTransformModifier* modifier )
{
	m_transformModifiers.Append( modifier );
}

Tr2GrannyAnimation* EveChildMesh::GetAnimationController() const
{
	return m_animationUpdater;
}

void EveChildMesh::SetAnimationController( Tr2GrannyAnimation* animation )
{
	m_animationUpdater = animation;
	InitializeAnimation();
}

void EveChildMesh::AddDecal( EveSpaceObjectDecalPtr newDecal )
{
	m_decals.Append( newDecal->GetRawRoot() );
}

void EveChildMesh::AddAttachment( IEveSpaceObjectAttachment* attachment )
{
	m_attachments.Append( attachment );
}

void EveChildMesh::ClearAttachments()
{
	m_attachments.Clear();
}

void EveChildMesh::RegisterWithQuadRenderer( Tr2QuadRenderer& quadRenderer )
{
	for( auto it = begin( m_attachments ); it != end( m_attachments ); ++it )
	{
		( *it )->RegisterWithQuadRenderer( quadRenderer );
	}
}

void EveChildMesh::AddQuadsToQuadRenderer( const TriFrustum& frustum, Tr2QuadRenderer& quadRenderer ) const
{
	if( m_attachments.empty() || !m_isVisible || !m_display )
	{
		return;
	}
	size_t boneCount = 0;
	const granny_matrix_3x4* bones = nullptr;
	Tr2GrannyAnimationUtils::GetBoneList( m_animationUpdater, bones, boneCount );
	for( auto it = begin( m_attachments ); it != end( m_attachments ); ++it )
	{
		// If we need boostergain then we will need to get it from the parent (the 0.0 in the param list)
		( *it )->AddToQuadRenderer( quadRenderer, m_worldTransform, m_activationStrength, 0.0, bones, boneCount );
	}
}

void EveChildMesh::GetLights( Tr2LightManager& lightManager ) const
{
	if( ( m_lights.empty() && m_attachments.empty() ) || !m_display )
	{
		return;
	}

	size_t boneCount = 0;
	const granny_matrix_3x4* bones = nullptr;
	Tr2GrannyAnimationUtils::GetBoneList( m_animationUpdater, bones, boneCount );

	for( auto it = std::begin( m_lights ); it != std::end( m_lights ); ++it )
	{
		( *it )->AddLight( lightManager, m_worldTransform, 1.0f, bones, boneCount );
		( *it )->SetBrightnessMultiplier( m_activationStrength );
	}

	for( auto it = std::begin( m_attachments ); it != std::end( m_attachments ); ++it )
	{
		( *it )->GetLights( lightManager, m_worldTransform );
	}
}

void EveChildMesh::AddLight( Tr2Light* newLight )
{
	m_lights.Append( newLight->GetRawRoot() );
}

void EveChildMesh::ClearLights()
{
	m_lights.Clear();
}

void EveChildMesh::SetReflectionMode( EntityComponents::ReflectionMode mode )
{
	m_reflectionMode = mode;
	if( GetComponentRegistry() )
	{
		ReRegister();
	}
}

void EveChildMesh::SetCastShadow( bool castShadow )
{
	m_castShadow = castShadow;
}

void EveChildMesh::SetMinScreenSize( float minScreenSize )
{
	m_minScreenSize = minScreenSize;
}
