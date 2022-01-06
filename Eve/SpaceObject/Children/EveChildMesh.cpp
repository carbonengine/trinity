#include "StdAfx.h"
#include "EveChildMesh.h"
#include "Tr2MeshBase.h"
#include "TriFrustum.h"
#include "Eve/SpaceObject/EveSpaceObject2.h"
#include "Eve/EveTransform.h"
#include "Utilities/BoundingSphere.h"
#include "Tr2MeshLod.h"


extern float g_eveSpaceSceneLODFactor;
extern float g_eveSpaceSceneVisibilityThreshold;


EveChildMesh::EveChildMesh( IRoot* lockobj ):
	PARENTLOCK( m_transformModifiers ),
	m_display( true ),
	m_isVisible( false ),
	m_lowestLodVisible( TR2_LOD_LOW ),
	m_minScreenSize( 0.f ),
	m_currentScreenSize( -1.f ),
	m_sortValueOffset( 0 ),
	m_sortValueScale( 1 ),
	m_useSpaceObjectData( true ),
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
	return true;
}

bool EveChildMesh::OnModified( Be::Var* val )
{
	if( IsMatch( val, m_reflectionMode ) || IsMatch( val, m_display) || IsMatch( val, m_mesh) )
	{
		ReRegister();
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

// --------------------------------------------------------------------------------
// Description:
//    Registers itself and its children with the scene registration container.
//    This is so we don't have to traverse the tree every frame
// --------------------------------------------------------------------------------
void EveChildMesh::RegisterComponents()
{
	auto registry = this->GetComponentRegistry();
	if( registry && m_display && m_mesh != nullptr)
	{
		if( EntityComponents::ShouldReflect( m_reflectionMode ) )
		{
			registry->RegisterComponent( ComponentType::REFLECTION_RENDERABLE, this, this->m_state );
		}
	}
}


void EveChildMesh::UpdateVisibility( const TriFrustum& frustum, const Matrix& parentTransform, Tr2Lod parentLod )
{
	m_isVisible = false;
	if( parentLod < m_lowestLodVisible )
	{
		return;
	}
	Vector4 boundingSphere;

	if( GetBoundingSphere( boundingSphere ) )
	{
		m_currentScreenSize = frustum.GetPixelSizeAccross( &boundingSphere );
		m_isVisible = m_currentScreenSize >= m_minScreenSize * g_eveSpaceSceneLODFactor;
	}
	else
	{
		m_currentScreenSize = -1;
	}
}
// --------------------------------------------------------------------------------
// Description:
//   Only used by planets because they use a custom frustum and UpdateVisibility fails at calculating m_currentScreenSize
// --------------------------------------------------------------------------------
void EveChildMesh::ForceCurrentScreenSize( float screenSize )
{
	m_currentScreenSize = screenSize;
	m_isVisible = m_currentScreenSize >= m_minScreenSize * g_eveSpaceSceneLODFactor;
}

void EveChildMesh::GetRenderables( std::vector<ITr2Renderable*>& renderables )
{
	if( m_isVisible )
	{
		renderables.push_back( this );
	}
}

bool EveChildMesh::GetBoundingSphere( Vector4& sphere, BoundingSphereQuery query ) const
{

	if( m_mesh && m_mesh->GetBoundingSphere( sphere ) )
	{
		BoundingSphereTransform( m_worldTransform, sphere );
		return true;
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
	if( m_display && m_mesh )
	{
		m_mesh->GetBatches( batches, m_mesh->GetAreas( batchType ), perObjectData );
	}
}

void EveChildMesh::GetShadowBatches( ITriRenderBatchAccumulator* batches, const Tr2PerObjectData* perObjectData )
{
	// TODO: Figure out what we want to do with shadows
	// Fix asap <Logi 27. aug 2015>
	if( m_display && m_mesh )
	{
		m_mesh->GetBatches( batches, m_mesh->GetAreas( TRIBATCHTYPE_OPAQUE ), perObjectData );
	}
}

float EveChildMesh::GetSortValue()
{
	Vector3 d = Tr2Renderer::GetViewPosition() - m_worldTransform.GetTranslation();
	float distance = Length( d );
	return distance * m_sortValueScale + m_sortValueOffset;
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
		return sizeof( m_vsData );
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
	}
}

void EveChildMesh::UpdateAsyncronous( EveUpdateContext& updateContext, const EveChildUpdateParams& params )
{
	m_perObjectDataVs.InvalidateBufferData();
	m_perObjectDataPs.InvalidateBufferData();

	Matrix localToWorldTransform;
	
	if( nullptr != params.childParent )
	{
		params.childParent->GetLocalToWorldTransform( localToWorldTransform );
	}
	else if( nullptr != params.spaceObjectParent )
	{
		params.spaceObjectParent->GetLocalToWorldTransform( localToWorldTransform );
		params.spaceObjectParent->GetPerObjectStructs( m_vsData, m_psData );
	}
	else 
	{
		localToWorldTransform = params.localToWorldTransform;
	}
	m_vsData.worldTransformLast = Transpose( m_worldTransform );

	UpdateTransform( localToWorldTransform );
	for( auto it = m_transformModifiers.begin(); it != m_transformModifiers.end(); it++ )
	{
		m_worldTransform = (*it)->ApplyTransform( m_worldTransform, params.boneCount, params.bones );
	}

	m_vsData.worldTransform = Transpose( m_worldTransform );
	m_vsData.invWorldTransform = Inverse( m_worldTransform );

	// Normalize screenSize dimensions
	auto screen_width = Tr2Renderer::GetViewport().width;
	auto screen_height = Tr2Renderer::GetViewport().height;
	m_psData.screenSize.x = float( m_currentScreenSize / screen_width );
	m_psData.screenSize.y = float( m_currentScreenSize / screen_height );
}

void EveChildMesh::UpdateSyncronous( EveUpdateContext& updateContext, const EveChildUpdateParams& )
{
}

void EveChildMesh::GetLocalToWorldTransform( Matrix& transform ) const
{
	transform = m_worldTransform;
}

void EveChildMesh::ChangeLOD( Tr2Lod lod )
{
	if( m_mesh == nullptr )
	{
		return;
	}
	if ( Tr2MeshLodPtr meshLod = BlueCastPtr( m_mesh ) )
	{
		meshLod->SelectLod( lod );
	}
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
}

void EveChildMesh::RenderDebugInfo( ITr2DebugRenderer2& renderer )
{
	if( m_display && m_mesh )
	{
		m_mesh->RenderDebugInfo( m_worldTransform, renderer );
	}
}

void EveChildMesh::AddTransformModifier( IEveChildTransformModifier* modifier )
{
	m_transformModifiers.Append( modifier );
}
