////////////////////////////////////////////////////////////
//
//    Created:   Jan 2019
//    Copyright: CCP 2019
//

#include "StdAfx.h"
#include "EveChildLineSet.h"
#include "Eve/EveUpdateContext.h"
#include "TransformModifiers/EveChildModifierTransformCommon.h"
#include "Resources/TriGeometryRes.h"
#include "TriFrustum.h"
#include "Utilities/BoundingSphere.h"

namespace
{
class EveChildLineSetPerObjectData : public Tr2PerObjectData
{
public:
	virtual void SetPerObjectDataToDevice( Tr2ConstantBufferAL** buffers, unsigned constantTypeMask, Tr2RenderContext& renderContext ) const override
	{
		FillAndSetConstants(
			*buffers[Tr2RenderContextEnum::VERTEX_SHADER],
			m_vsData,
			sizeof( *m_vsData ),
			Tr2RenderContextEnum::VERTEX_SHADER,
			Tr2Renderer::GetPerObjectVSStartRegister(),
			renderContext );
		FillAndSetConstants(
			*buffers[Tr2RenderContextEnum::PIXEL_SHADER],
			m_psData,
			sizeof( *m_psData ),
			Tr2RenderContextEnum::PIXEL_SHADER,
			Tr2Renderer::GetPerObjectPSStartRegister(),
			renderContext );
	}
	EveSpaceObjectPSData* m_psData;
	EveSpaceObjectVSData* m_vsData;
};
}

// --------------------------------------------------------------------------------------
// Description
//   Render batch specialization for EveChildContainer-batches
// See Also
//   TriRenderBatch, ITr2GeometryBatch
// --------------------------------------------------------------------------------------
class ChildLineSetInstancingBatch : public TriGeometryBatch
{
public:
	void SetGeometryResource( TriGeometryRes* val )
	{
		m_geometryResource = val;
	}


	// Set the geometry provider
	void SetGeometryProvider( EveChildLineSet* val )
	{
		m_geom = val;
	}

	// Forward the SubmitGeometry call to the geometry provider
	void SubmitGeometry( Tr2RenderContext& renderContext )
	{
		if( m_geom )
		{
			if( m_geom->GetMesh()->GetDisplay() )
			{
				m_geom->Draw( this, renderContext );
			}
		}
	}

	// Gets the batch type name for PIX debugging
	virtual const std::string& GetBatchTypeName( void ) const
	{
		static const std::string name = "ChildLineSetInstancingBatch";
		return name;
	}

private:
	EveChildLineSetPtr m_geom;
};


EveChildLineSet::EveChildLineSet( IRoot* lockobj ) :
	EveChildTransform(),
	PARENTLOCK( m_lines ),
	m_vertexDeclarationHandle( Tr2EffectStateManager::UNINITIALIZED_DECLARATION ),
	m_worldVelocity( 0, 0, 0 ),
	m_display( true ),
	m_isAlwaysOn( false ),
	m_stride( 12 * sizeof( float ) ),
	m_ownerMaxSpeed( 0 ),
	m_scrollSpeed( 0 ),
	m_baseColor( 1, 1, 1, 1 ),
	m_animColor( 0, 0, 0, 1 ),
	m_boundingSphere( 0, 0, 0, 1 ),
	m_currentScreenSize( 1 ),
	m_brightness( 1 ),
	m_minScreenSize( -1 ),
	m_isVisible( true ),
	m_type( LINE_RENDER ),
	m_scaleSegmentsByCompleteness( false ),
	m_additiveBatch( false ),
	m_updateLineSet( true ),
	m_totalObjectCount( 0 ),
	m_vertexCount( 0 ),
	m_cachedSVD( 0 )
{
	Initialize();
}

EveChildLineSet::~EveChildLineSet()
{
}

bool EveChildLineSet::Initialize()
{
	if( m_lineSet == nullptr )
	{
		if( !m_lineSet.CreateInstance() )
		{
			return true;
		}
	}

	GenerateManagedPoints();
	InitializeLineSet();

	return true;
}

bool EveChildLineSet::OnModified( Be::Var* value )
{
	if( IsMatch( value, m_additiveBatch ) )
	{
		m_lineSet->SetAdditiveFlag( m_additiveBatch );
	}

	m_updateLineSet = true;
	return true;
}

/////////////////////////////////////////////////////////////////////////////////////
// Tr2DeviceResource
bool EveChildLineSet::OnPrepareResources()
{
	USE_MAIN_THREAD_RENDER_CONTEXT();

	auto hr = m_vertexBuffer.Create(
		m_stride,
		128,
		Tr2GpuUsage::VERTEX_BUFFER,
		Tr2CpuUsage::WRITE_OFTEN,
		nullptr,
		renderContext );
	if( FAILED( hr ) )
	{
		return false;
	}

	return true;
}

void EveChildLineSet::SetShaderOption( const BlueSharedString& name, const BlueSharedString& value )
{
}

bool EveChildLineSet::IsAlwaysOn() const
{
	return m_isAlwaysOn;
}

Tr2MeshPtr EveChildLineSet::GetMesh() const
{
	return m_mesh;
}

void EveChildLineSet::GenerateManagedPoints()
{
	for( auto it = begin( m_lines ); it != end( m_lines ); ++it )
	{
		( *it )->GeneratePoints( m_worldTransform );
	}

	UpdateBoundingSphere();
}


void EveChildLineSet::InitializeLineSet()
{
	if( !m_lineSet || m_lines.empty() )
	{
		return;
	}

	m_lineSet->ClearLines();
	m_lineSet->SetDynamicFlag( true );

	for( auto it = begin( m_lines ); it != end( m_lines ); ++it )
	{
		( *it )->AddLinesToSet( *m_lineSet, m_baseColor * m_brightness, m_animColor * m_brightness, m_scrollSpeed );
	}

	m_lineSet->SubmitChanges();
}

void EveChildLineSet::UpdateBoundingSphere( bool reCalculateChildren )
{
	Vector4 sphere;
	float objectSizeBonus = 0.f;

	if( m_type != LINE_RENDER && m_mesh != nullptr )
	{
		if( m_mesh->GetGeometryResource() != nullptr )
		{
			if( ( m_mesh->GetGeometryResource() )->IsGood() )
			{
				TriGeometryResMeshData* meshData = m_mesh->GetGeometryResource()->GetMeshData( m_mesh->GetMeshIndex() );
				objectSizeBonus += meshData->m_boundingSphere.w;
			}
		}
	}

	CalculateBoundingSphereForLineSetPaths( sphere, m_lines, reCalculateChildren, objectSizeBonus );

	m_boundingSphere = sphere;
}

const char* EveChildLineSet::GetName() const
{
	return m_name.c_str();
}

void EveChildLineSet::SetName( const char* name )
{
	m_name = BlueSharedString( name );
}

void EveChildLineSet::UpdateVisibility( const TriFrustum& frustum, const Matrix& parentTransform, Tr2Lod parentLod )
{
	if( !m_display )
	{
		return;
	}

	m_isVisible = false;
	Vector4 sphere = m_boundingSphere;
	BoundingSphereTransform( m_worldTransform, sphere );

	if( frustum.IsSphereVisible( &( sphere ) ) )
	{
		m_currentScreenSize = frustum.GetPixelSizeAccross( &m_boundingSphere );

		if( m_currentScreenSize >= m_minScreenSize )
		{
			m_isVisible = true;
		}
	}

	if( m_lineSet )
	{
		m_lineSet->UpdateVisibility( frustum, m_worldTransform );
	}

	for( auto it = begin( m_lines ); it != end( m_lines ); ++it )
	{
		( *it )->UpdateVisibility( frustum, parentLod, m_worldTransform );
	}
}

void EveChildLineSet::GetRenderables( std::vector<ITr2Renderable*>& renderables )
{
	if( !m_display )
	{
		return;
	}

	if( !m_isVisible )
	{
		return;
	}


	if( m_updateLineSet )
	{
		GenerateManagedPoints();
		InitializeLineSet();
		m_updateLineSet = false;
	}

	if( LINE_RENDER != m_type )
	{
		renderables.push_back( this );
	}

	if( OBJECT_RENDER != m_type )
	{
		if( m_lineSet )
		{
			m_lineSet->GetRenderables( renderables );
		}
	}
}

bool EveChildLineSet::GetBoundingSphere( Vector4& sphere, BoundingSphereQuery query ) const
{
	sphere = m_boundingSphere;

	BoundingSphereTransform( m_worldTransform, sphere );

	return true;
}

void EveChildLineSet::UpdateSyncronous( EveUpdateContext& updateContext, const EveChildUpdateParams& params )
{
	CreateSpriteVertexDeclaration();

	m_ownerMaxSpeed = params.ownerMaxSpeed;

	bool updateBounds = false;

	for( auto it = begin( m_lines ); it != end( m_lines ); ++it )
	{
		const bool updateLocalBoundingSphere = ( *it )->Update( updateContext, params );
		updateBounds = updateBounds || updateLocalBoundingSphere;
	}

	if( updateBounds )
	{
		UpdateBoundingSphere( false );
	}

	if( m_type == OBJECT_RENDER || m_type == BOTH )
	{
		USE_MAIN_THREAD_RENDER_CONTEXT();
		UpdateBuffer( renderContext );
	}

	if( m_type == LINE_RENDER || m_type == BOTH )
	{
		InitializeLineSet();
	}
}

void EveChildLineSet::UpdateBuffer( Tr2RenderContext& renderContext )
{
	unsigned points, total = 0;
	for( auto it = begin( m_lines ); it != end( m_lines ); ++it )
	{
		( *it )->GetPointCount( points );
		total += points;
	}
	m_totalObjectCount = total;

	if( !m_vertexBuffer.IsValid() || m_vertexBuffer.GetDesc().count < m_totalObjectCount )
	{
		USE_MAIN_THREAD_RENDER_CONTEXT();
		m_vertexBuffer.Create(
			m_stride,
			uint32_t( m_totalObjectCount ),
			Tr2GpuUsage::VERTEX_BUFFER,
			Tr2CpuUsage::WRITE_OFTEN,
			nullptr,
			renderContext );
	}

	uint8_t* data;

	CR_RETURN( m_vertexBuffer.MapForWriting( data, renderContext ) );

	for( auto it = begin( m_lines ); it != end( m_lines ); ++it )
	{
		( *it )->UpdateBuffer( renderContext, data, m_worldTransform, m_stride );
	}

	m_vertexBuffer.UnmapForWriting( renderContext );
}

void EveChildLineSet::CreateSpriteVertexDeclaration()
{
	Tr2MeshPtr meshPtr = m_mesh;

	if( meshPtr )
	{
		if( nullptr == meshPtr->GetGeometryResource() )
		{
			return;
		}

		if( ( meshPtr->GetGeometryResource() )->IsGood() )
		{
			TriGeometryResMeshData* meshData = meshPtr->GetGeometryResource()->GetMeshData( meshPtr->GetMeshIndex() );
			if( meshData->m_vertexDeclaration != m_cachedSVD )
			{
				Tr2VertexDefinition s_InstancedVertex;
				Tr2EffectStateManager::GetVertexDeclarationElements( meshData->m_vertexDeclaration, s_InstancedVertex );

				Tr2VertexDefinition& def = s_InstancedVertex;
				def.Add( Tr2VertexDefinition::FLOAT32_4, Tr2VertexDefinition::TEXCOORD, 8, 1, 1 );
				def.Add( Tr2VertexDefinition::FLOAT32_4, Tr2VertexDefinition::TEXCOORD, 9, 1, 1 );
				def.Add( Tr2VertexDefinition::FLOAT32_4, Tr2VertexDefinition::TEXCOORD, 10, 1, 1 );

				// create vertex-declarartion
				m_vertexDeclarationHandle = Tr2EffectStateManager::GetVertexDeclarationHandle( s_InstancedVertex );
				m_cachedSVD = meshData->m_vertexDeclaration;
			}
			return;
		}
	}
	m_cachedSVD = -1;
	m_vertexDeclarationHandle = -1;
}

// for validation and objects interacting with the shader attributes
std::vector<std::pair<int, int>> EveChildLineSet::GetVertexElementAddedThroughCode() const
{
	std::vector<std::pair<int, int>> out;
	out.emplace_back( std::pair<int, int>( Tr2VertexDefinition::TEXCOORD, 8 ) );
	out.emplace_back( std::pair<int, int>( Tr2VertexDefinition::TEXCOORD, 9 ) );
	out.emplace_back( std::pair<int, int>( Tr2VertexDefinition::TEXCOORD, 10 ) );
	return out;
}

void EveChildLineSet::UpdateAsyncronous( EveUpdateContext& updateContext, const EveChildUpdateParams& params )
{
	Matrix localToWorldTransform = params.localToWorldTransform;

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

	m_vsData.worldTransform = Transpose( m_worldTransform );
	m_vsData.invWorldTransform = Inverse( m_worldTransform );
}


Tr2PerObjectData* EveChildLineSet::GetPerObjectData( ITriRenderBatchAccumulator* accumulator )
{
	EveChildLineSetPerObjectData* perObjectData = accumulator->Allocate<EveChildLineSetPerObjectData>();

	if( !perObjectData )
	{
		return nullptr;
	}

	perObjectData->m_vsData = &m_vsData;
	perObjectData->m_psData = &m_psData;
	return perObjectData;
}

bool EveChildLineSet::HasTransparentBatches()
{
	if( m_display && m_mesh )
	{
		if( !( m_mesh->GetAreas( TRIBATCHTYPE_TRANSPARENT )->empty() ) )
		{
			return true;
		}
	}
	return false;
}

void EveChildLineSet::GetBatches( ITriRenderBatchAccumulator* batches, TriBatchType batchType, const Tr2PerObjectData* perObjectData, Tr2RenderReason reason )
{
	if( !m_display )
	{
		return;
	}

	if( m_cachedSVD == -1 || m_vertexDeclarationHandle == -1 )
	{
		return;
	}

	if( !m_vertexBuffer.IsValid() )
	{
		return;
	}

	if( m_type != OBJECT_RENDER && m_type != BOTH )
	{
		return;
	}

	if( m_mesh == nullptr )
	{
		return;
	}

	if( m_mesh->GetGeometryResource() == nullptr )
	{
		return;
	}

	if( !( m_mesh->GetGeometryResource()->IsGood() ) )
	{
		return;
	}

	if( m_mesh->GetGeometryResource()->GetMeshCount() < 1 )
	{
		return;
	}

	auto areaList = m_mesh->GetAreas( batchType );

	for( auto srcMeshArea = areaList->begin(); srcMeshArea != areaList->end(); ++srcMeshArea )
	{
		auto a = *srcMeshArea;

		ChildLineSetInstancingBatch* batch = batches->Allocate<ChildLineSetInstancingBatch>();

		if( nullptr == batch )
		{
			continue;
		}

		batch->SetPerObjectData( perObjectData );
		batch->SetShaderMaterial( a->GetMaterialInterface() );
		batch->SetMeshParameters( m_mesh->GetMeshIndex(), a->GetIndex(), a->GetCount(), a->IsReversed() );
		batch->SetGeometryResource( m_mesh->GetGeometryResource() );
		batch->SetGeometryProvider( this );
		batches->Commit( batch );
	}
}

// --------------------------------------------------------------------------------
// Description:
//   Setup instanced rendering and call DIP
// --------------------------------------------------------------------------------
void EveChildLineSet::Draw( ChildLineSetInstancingBatch* batch, Tr2RenderContext& renderContext )
{
	auto geometry = batch->GetGeometryResource();

	if( geometry == nullptr )
	{
		return;
	}
	if( !( geometry->IsGood() ) )
	{
		return;
	}
	if( geometry->GetMeshCount() < 1 )
	{
		return;
	}
	if( !m_vertexBuffer.IsValid() )
	{
		return;
	}

	const TriGeometryResMeshData* meshData = geometry->GetMeshData( 0 );
	auto areaIx = batch->GetAreaIndex();
	auto areaCount = batch->GetAreaCount();

	if( areaIx >= meshData->m_areas.size() )
	{
		return;
	}

	if( areaIx + areaCount > meshData->m_areas.size() )
	{
		areaCount = static_cast<unsigned int>( meshData->m_areas.size() ) - areaIx;
	}

	const TriGeometryResAreaData& area = meshData->m_areas[areaIx];

	unsigned int primCount = area.m_primitiveCount;
	for( unsigned int i = 1; i < areaCount; ++i )
	{
		const TriGeometryResAreaData& curArea = meshData->m_areas[areaIx + i];
		primCount += curArea.m_primitiveCount;
	}


	renderContext.m_esm.ApplyVertexDeclaration( m_vertexDeclarationHandle );
	renderContext.m_esm.ApplyIndexBuffer( meshData->m_indexBuffer );
	// Stream 0: "geometry": here: our object's geometry
	renderContext.m_esm.ApplyStreamSource( 0, meshData->m_vertexBuffer, 0, meshData->m_bytesPerVertex );
	// Stream 1: instance", here: instance index
	renderContext.m_esm.ApplyStreamSource( 1, m_vertexBuffer, 0, m_stride );

	renderContext.SetTopology( Tr2RenderContextEnum::TOP_TRIANGLES );
	renderContext.DrawIndexedInstanced( meshData->m_vertexCount, area.m_firstIndex, primCount, m_totalObjectCount );
}


void EveChildLineSet::GetLocalToWorldTransform( Matrix& transform ) const
{
	transform = m_worldTransform;
}

void EveChildLineSet::ChangeLOD( Tr2Lod lod )
{
}

void EveChildLineSet::Setup( const Vector3* scale, const Quaternion* rotation, const Vector3* translation, Tr2Lod lowestLodVisible )
{
	EveChildTransform::Setup( scale, rotation, translation, lowestLodVisible );
}

void EveChildLineSet::GetDebugOptions( Tr2DebugRendererOptions& options )
{
	options.insert( "LineSets" );
}

void EveChildLineSet::RenderDebugInfo( ITr2DebugRenderer2& renderer )
{
	if( !m_display )
	{
		return;
	}

	for( auto it = begin( m_lines ); it != end( m_lines ); ++it )
	{
		( *it )->RenderDebugInfo( renderer, m_worldTransform );
	}

	if( renderer.HasOption( this, "Bounding Sphere" ) )
	{
		renderer.DrawSphere( this, TranslationMatrix( m_boundingSphere.GetXYZ() ) * m_worldTransform, m_boundingSphere.w, 12, Tr2DebugRenderer::Wireframe, 0xaaffffaa );
	}
}

void EveChildLineSet::GetWorldVelocity( Vector3& velocity ) const
{
	velocity = m_worldVelocity;
}


void EveChildLineSet::ReleaseResources( TriStorage s )
{
	m_cachedSVD = -1;
	m_vertexDeclarationHandle = -1;
}

float EveChildLineSet::GetOwnerMaxSpeed() const
{
	return m_ownerMaxSpeed;
}
