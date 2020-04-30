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
#include "include/TriMath.h"
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
				m_vsData, sizeof( *m_vsData ),
				Tr2RenderContextEnum::VERTEX_SHADER,
				Tr2Renderer::GetPerObjectVSStartRegister(),
				renderContext 
			);
			FillAndSetConstants( 
				*buffers[Tr2RenderContextEnum::PIXEL_SHADER],
				m_psData, sizeof( *m_psData ),
				Tr2RenderContextEnum::PIXEL_SHADER,
				Tr2Renderer::GetPerObjectPSStartRegister(),
				renderContext 
			);
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
				m_geom->Draw( this, renderContext);
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
	m_vertexDeclarationHandle( Tr2EffectStateManager::UNINITIALIZED_DECLARATION ),
	m_worldVelocity( 0, 0, 0 ),
	m_display( true ),
	m_billboardObject( true ),
	m_isAlwaysOn( false ),
	m_stride( 12 * sizeof( float ) ),
	m_ownerMaxSpeed( 0 ),
	m_scrollSpeed( 0 ),
	m_scrollSegmenting( 1 ),
	m_circleRadius( 500 ),
	m_circleDistort( 1, 0, 1, 0 ),
	m_objectScale( 1, 1, 1 ),
	m_numSegments( 64 ),
	m_exposedNumSegments( 64 ),
	m_completeness( 1 ),
	m_baseColor( 1, 1, 1, 1 ),
	m_animColor( 0, 0, 0, 1 ),
	m_animValue( 0 ),
	m_animSpeed( 0 ),
	m_lineWidth( 1 ),
	m_point1( 0, 0, 0 ),
	m_point2( 0, 0, 0 ),
	m_bezierPoint( 0, 0, 0 ),
	m_curveSegments( 24 ),
	m_brightness( 1 ),
	m_boundingSphere( 0, 0, 0, 1 ),
	m_currentScreenSize( 1 ),
	m_minScreenSize( -1 ),
	m_isVisible( true ),
	m_exposedCurveSegments( 24 ),
	m_type( LINE_RENDER ),
	m_objType( CIRCLE ),
	m_additiveBatch( false ),
	m_updateLineSet( true )
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

	m_numSegments = int( m_exposedNumSegments + 0.5f );
	m_curveSegments = int( m_exposedCurveSegments + 0.5f );
	
	GenerateManagedPoints();
	InitializeLineSet();
	UpdateBoundingSphere();
	
	return true;
}

bool EveChildLineSet::OnModified( Be::Var* value )
{
	if( IsMatch( value, m_completeness ) )
	{
		m_completeness = min( 2.f, max( 0.f, m_completeness ) );
	}

	if( IsMatch( value, m_exposedNumSegments ) )
	{
		m_numSegments = min( max( 1, int( m_exposedNumSegments + 0.5f ) ), 256 );
		// 256: enough for each 1/4th circle segment to have up to 64 segments each as a "smoothness limit" for our artists
	}

	if( IsMatch( value, m_exposedCurveSegments ) )
	{
		m_curveSegments = min( max( 1, int( m_exposedCurveSegments + 0.5f ) ), 128 );
		// 128: enough for each arc segment to have up to 64 segments each as a "smoothness limit" for our artists
	}

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
	if ( m_objType == BEZIER_CURVE )
	{
		return GenerateManagedPointsForCurve();
	}
		
	if( m_numSegments <= 0 )
	{
		return;
	}

	const float totalArc = ( 1.f - abs( m_completeness - 1.f ) ) * XM_2PI;
	const float startOffset = max( m_completeness - 1.f, 0.f ) * XM_2PI + totalArc / ( 2 * m_numSegments );

	m_managedPoints.clear();
	m_managedPoints.reserve( m_numSegments );
	
	for( int i = 0; i < m_numSegments; i++ )
	{
		const float locOnCircle = startOffset + totalArc * ( ( float( i ) / m_numSegments ) + m_animValue / m_numSegments );
		float X = cos( locOnCircle )*m_circleRadius * m_circleDistort.x;
		float Y = 0.f;
		if( m_circleDistort.y != 0 || m_circleDistort.w != 0 )
		{
			Y = pow( sin( locOnCircle ), 2.f ) * m_circleRadius * m_circleDistort.y + pow( cos( locOnCircle ), 2.f ) * m_circleRadius * m_circleDistort.w;
		}
		float Z = sin( locOnCircle )*m_circleRadius * m_circleDistort.z;
		m_managedPoints.emplace_back( Vector3( X, Y, Z) );
	}
}

void EveChildLineSet::GenerateManagedPointsForCurve()
{
	if( m_curveSegments <= 0 )
	{
		return;
	}

	m_managedPoints.clear();
	m_managedPoints.reserve( m_curveSegments );

	for( int i = 0; i < m_curveSegments; i++ )
	{
		const float LoC = ( float( i ) / m_curveSegments ) + m_animValue / m_curveSegments; // LoC = Location on Curve
		float X = ( 1 - LoC ) * ( 1 - LoC ) * m_point1.x + 2 * ( 1 - LoC ) * LoC * m_bezierPoint.x + LoC * LoC * m_point2.x;
		float Y = ( 1 - LoC ) * ( 1 - LoC ) * m_point1.y + 2 * ( 1 - LoC ) * LoC * m_bezierPoint.y + LoC * LoC * m_point2.y;
		float Z = ( 1 - LoC ) * ( 1 - LoC ) * m_point1.z + 2 * ( 1 - LoC ) * LoC * m_bezierPoint.z + LoC * LoC * m_point2.z;

		m_managedPoints.emplace_back( Vector3( X, Y, Z ) );
	}
}


void EveChildLineSet::InitializeLineSet()
{
	if( m_objType == BEZIER_CURVE )
	{
		return InitializeLineSetForCurves();
	}
	
	if( !m_lineSet || m_numSegments <= 1 )
	{
		return;
	}

	m_lineSet->ClearLines();

	int lines = m_numSegments;

	if( m_completeness != 1 )
	{
		lines--;
	}

	float b = m_brightness;
	
	for( int i = 0; i < lines; i++ )
	{
		int nextPoint = ( i + 1 ) % m_numSegments;
		
		int id = m_lineSet->AddStraightLine(m_managedPoints[i], m_baseColor * b,m_managedPoints[nextPoint], m_baseColor * b, m_lineWidth );
		
		m_lineSet->ChangeLineAnimation( id, (Vector4) m_animColor * b, m_scrollSpeed, m_scrollSegmenting );
	}
	
	m_lineSet->SubmitChanges();
}


void EveChildLineSet::InitializeLineSetForCurves()
{
	if( !m_lineSet || m_curveSegments <= 0 )
	{
		return;
	}

	m_lineSet->ClearLines();

	float b = m_brightness;
	
	for( int i = 0; i < m_curveSegments; i++ )
	{
		int nextPoint = ( i + 1 ) % m_curveSegments;

		int id;
		
		if (nextPoint == 0)
		{
			id = m_lineSet->AddStraightLine( m_managedPoints[i], m_baseColor * b, m_point2, m_baseColor * b, m_lineWidth );
		}
		else
		{
			id = m_lineSet->AddStraightLine( m_managedPoints[i], m_baseColor * b, m_managedPoints[nextPoint], m_baseColor * b, m_lineWidth );
		}

		if( m_scrollSpeed != 0 )
		{
			m_lineSet->ChangeLineAnimation( id, (Vector4) m_animColor * b, m_scrollSpeed, m_scrollSegmenting );
		}
	}

	m_lineSet->SubmitChanges();
}

void EveChildLineSet::UpdateBoundingSphere()
{
	float objectSizeBonus = 0;
	
	if( m_type != LINE_RENDER )
	{
		if( m_mesh != nullptr)
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
	}
		
	if (m_objType == CIRCLE)
	{
		m_boundingSphere = Vector4( (TranslationMatrix( m_translation ) * m_worldTransform ).GetTranslation(), m_circleRadius + m_lineWidth + objectSizeBonus );
	}
	
	if( m_objType == BEZIER_CURVE )
	{
		Vector3 center( 0.f, 0.f, 0.f );
		center += m_point1;
		center += m_point2;
		center += m_bezierPoint;
		center /= 3.f;
		float rad = max( max( LengthSq( m_bezierPoint - center ), LengthSq( m_point2 - center ) ), LengthSq( m_point1 - center ) );
		m_boundingSphere = Vector4(  (TranslationMatrix(center) * m_worldTransform).GetTranslation(), sqrt(rad) + m_lineWidth + objectSizeBonus );
	}
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

	if( frustum.IsSphereVisible( &m_boundingSphere ) )
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
		UpdateBoundingSphere();
		m_updateLineSet = false;
	}
	
	
	if( LINE_RENDER != m_type )
	{
		renderables.push_back( this );
	}
	
	if ( OBJECT_RENDER != m_type )
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
	
	if ( m_animSpeed != 0 && m_type != LINE_RENDER )
	{
		float dt = updateContext.GetDeltaT();

		m_animValue += m_animSpeed * dt;
		m_animValue = fmod(m_animValue,1.f );
		GenerateManagedPoints();
	}

	if ( m_type == OBJECT_RENDER || m_type == BOTH )
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
	if( !m_vertexBuffer.IsValid() || m_vertexBuffer.GetDesc().count < m_managedPoints.size() )
	{
		USE_MAIN_THREAD_RENDER_CONTEXT();
		m_vertexBuffer.Create(
			m_stride,
			uint32_t( m_managedPoints.size() ),	
			Tr2GpuUsage::VERTEX_BUFFER, // VERTEX_BUFFER
			Tr2CpuUsage::WRITE_OFTEN,
			nullptr,
			renderContext );
	}
	
	uint8_t *data;
	Matrix WT = m_worldTransform;
	
	Vector3 scale, translation;
	Quaternion objRot, worldRot;
	Decompose( scale, worldRot, translation, Inverse( WT ) );

	CR_RETURN( m_vertexBuffer.MapForWriting( data, renderContext ) );
	
	for( int i = 0; i < int( m_managedPoints.size() ); i++ )
	{
		if( m_billboardObject )
		{
			Matrix m = TransformationMatrix( m_objectScale, m_rotation, m_managedPoints[i] ) * WT;

			m = Billboard2D( m );
		
			Decompose( scale, objRot, translation, m );

			m = TransformationMatrix( m_objectScale, objRot * worldRot, m_managedPoints[i] ) ;
			m = Transpose( m );
			
			memcpy( data, &m, m_stride );
			
			data += m_stride;
		}
		else
		{
			Vector3 dirToNextPoint( 0.f, 1.f, 0.f );
			int nextPoint = ( i + 1 ) % int(m_managedPoints.size());
			
			if ( nextPoint == 0 )
			{
				float blender = 1.f - pow( 1.f - abs(  m_completeness - 1.f ), 15.f );
				dirToNextPoint = Lerp(	m_managedPoints[nextPoint] - m_managedPoints[i],
										m_managedPoints[i] - m_managedPoints[i - 1],
										blender );
			}
			else
			{
				dirToNextPoint = m_managedPoints[nextPoint] - m_managedPoints[i];
			}
			
			TriQuaternionArcFromForward( &objRot, &dirToNextPoint );
			Matrix matrix = TransformationMatrix( m_objectScale, objRot, m_managedPoints[i] );
			Matrix m = Transpose( matrix );
			memcpy( data, &m, m_stride );
			data += m_stride;
		}
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
	out.emplace_back(std::pair<int,int>(Tr2VertexDefinition::TEXCOORD, 8));
	out.emplace_back(std::pair<int, int>( Tr2VertexDefinition::TEXCOORD, 9));
	out.emplace_back(std::pair<int, int>( Tr2VertexDefinition::TEXCOORD, 10));
	return out;
}

void EveChildLineSet::UpdateAsyncronous( EveUpdateContext& updateContext, const EveChildUpdateParams& params )
{
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

void EveChildLineSet::GetBatches( ITriRenderBatchAccumulator* batches, TriBatchType batchType,
	const Tr2PerObjectData* perObjectData )
{
	if( !m_display )
	{
		return;
	}

	if( m_cachedSVD == -1 || m_vertexDeclarationHandle == -1)
	{
		return;
	}
	
	if( !m_vertexBuffer.IsValid() )
	{
		return;
	}

	if ( m_type != OBJECT_RENDER && m_type != BOTH )
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
void EveChildLineSet::Draw( ChildLineSetInstancingBatch* batch, Tr2RenderContext& renderContext)
{
	auto geometry = batch->GetGeometryResource();

	if( geometry == nullptr ) { return; }
	if( !( geometry->IsGood() ) ) { return; }
	if( geometry->GetMeshCount() < 1 ) { return; }
	if( !m_vertexBuffer.IsValid() ) { return; }

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
	renderContext.DrawIndexedInstanced( meshData->m_vertexCount, area.m_firstIndex, primCount, int( m_managedPoints.size() ) );
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
	options.insert( "EveChildLineSetPoints" );
}

void EveChildLineSet::RenderDebugInfo( ITr2DebugRenderer2& renderer )
{
	if( !m_display )
	{
		return;
	}
	
	if( renderer.HasOption( this, "EveChildLineSetPoints" ) )
	{
		for( auto point = m_managedPoints.begin(); point != m_managedPoints.end(); ++point )
		{
			renderer.DrawSphere( this, TranslationMatrix( *point ) * m_worldTransform, 25, 8, Tr2DebugRenderer::Wireframe, 0xffffffff );
		}

		renderer.DrawSphere( this, TranslationMatrix( m_boundingSphere.GetXYZ() ) * m_worldTransform, m_boundingSphere.w, 10, Tr2DebugRenderer::Wireframe, 0xffffffaa );
		
		if (m_objType == BEZIER_CURVE)
		{
			renderer.DrawSphere( this, TranslationMatrix( m_bezierPoint ) * m_worldTransform, 30, 10, Tr2DebugRenderer::Wireframe, 0xbbbbffff );
			renderer.DrawSphere( this, TranslationMatrix( m_point1 ) * m_worldTransform, 30, 10, Tr2DebugRenderer::Wireframe, 0xbbffbbff );
			renderer.DrawSphere( this, TranslationMatrix( m_point2 ) * m_worldTransform, 30, 10, Tr2DebugRenderer::Wireframe, 0xbbffbbff );
		}
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