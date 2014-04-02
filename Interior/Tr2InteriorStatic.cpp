#include "StdAfx.h"

#if INTERIORS_ENABLED

#include "Tr2InteriorStatic.h"

#include "umbraTypes.h"

#include "TriLineSet.h"
#include "Resources/TriGeometryRes.h"
#include "Resources/TriGrannyRes.h"

#include "Tr2EnlightenArea.h"
#include "Tr2Mesh.h"
#include "Tr2InteriorEnlightenSystem.h"
#include "Tr2InteriorCell.h"
#include "Tr2LitPerObjectData.h"
#include "ITr2UmbraUserData.h"
#include "Tr2ShaderMaterial.h"
#include "Curves/TriCurveSet.h"


using namespace Tr2RenderContextEnum;

// ---------------------------------------------------------------------------------------
Tr2InteriorStatic::Tr2InteriorStatic( IRoot* lockobj ):
	m_umbraObject( NULL ),
	m_umbraModel( NULL ),
	PARENTLOCK( m_enlightenAreas ),
	PARENTLOCK( m_detailMeshes ),
	m_display( true ),
	m_displayTargetMesh( true ),
	m_displayDetailMeshes( true ),
	m_position( 0.0f, 0.0f, 0.0f ),
	m_rotation( 0.0f, 0.0f, 0.0f, 1.0f ),
	m_drawLightSpider( false ),
	m_parentSystem( 0 ),
	m_shSolver( NULL ),
	PARENTLOCK( m_curveSets ),
	m_depthOffset( 0.f )
{
	// 0
	m_transform = Tr2Renderer::GetIdentityTransform();

	m_uvLinearTransform.x = 1.0f;
	m_uvLinearTransform.y = 0.0f;
	m_uvLinearTransform.z = 0.0f;
	m_uvLinearTransform.w = 1.0f;

	m_uvTranslation.x = 0.0f;
	m_uvTranslation.y = 0.0f;

	// Initializing to 0
	m_instanceInSystemIdx = 0;

	// Initial visibility
	m_isVisible = false;

	// Set the dirty flag
	m_isDirty = true;

	// Initial visualize method
	m_visualizeMethod = VM_NONE;

	// Initialize light counts
	m_visibleLightCount = 0;
	m_totalLightCount = 0;

	// Occlusion
	m_isOcclusionGeometryReady = false;
	m_useOcclusionGeometry = true;

	m_enlightenAreaMaterial.CreateInstance();
	m_enlightenAreaMaterial->SetHighLevelShaderName( "EnlightenArea" );
	
	D3DXMatrixIdentity( &m_mirrorToWorldMatrix );
}

// ---------------------------------------------------------------------------------------
Tr2InteriorStatic::~Tr2InteriorStatic()
{

	// get out of cell and free umbra stuff
	RemoveFromCell();
	DestroyOccluders();

	if( m_occlusionResource )
	{
		m_occlusionResource->RemoveNotifyTarget( this );
		m_occlusionResource.Unlock();
	}

	if( m_geometryResource )
	{
		m_geometryResource->RemoveNotifyTarget( this );
		m_geometryResource.Unlock();
	}
}

void Tr2InteriorStatic::ReleaseResources( TriStorage s )
{
	if( ( s & TRISTORAGE_ALL ) == TRISTORAGE_ALL )
	{
		for( unsigned i = 0; i != SHADER_TYPE_COUNT; ++i )
		{
			m_perObjectConstantBuffers[i].Destroy();
		}
	}
}

bool Tr2InteriorStatic::OnPrepareResources()
{
	return true;
}

// ---------------------------------------------------------------------------------------
void Tr2InteriorStatic::SetInstanceData( const Vector4 &linearTransform, 
										 const Vector2 &translation, 
										 unsigned int instanceInSystemIdx )
{
	m_uvLinearTransform = linearTransform;
	m_uvTranslation = translation;
	m_instanceInSystemIdx = instanceInSystemIdx;
}

// ---------------------------------------------------------------------------------------
bool Tr2InteriorStatic::GetBoundingBox( Vector3& minBounds, Vector3& maxBounds ) const
{
	Vector3 min( FLT_MAX, FLT_MAX, FLT_MAX );
	Vector3 max( -FLT_MAX, -FLT_MAX, -FLT_MAX );

	if( m_geometryResource && m_geometryResource->IsGood() )
	{
		for( unsigned int i = 0; i < m_geometryResource->GetMeshCount(); ++i )
		{
			Vector3 meshMin, meshMax;
			if( !m_geometryResource->GetBoundingBox( i, meshMin, meshMax ) )
			{
				return false;
			}
			BoundingBoxUpdate( min, max, meshMin, meshMax );
		}

		minBounds = min;
		maxBounds = max;
		return true;
	}
	else
	{
		return false;
	}
}

// ---------------------------------------------------------------------------------------
// Description:
//   Gets the bounding box of the static in world space.
// Arguments:
//   minBounds - [output] the minimum world-space extents of the bounding box
//   maxBounds - [output] the maximum world-space extents of the bounding box
// Return Value:
//   true, if the bounding box is ready
//   false, if the bounding box is not ready (i.e. the static is not fully loaded)
// ---------------------------------------------------------------------------------------
bool Tr2InteriorStatic::GetWorldBoundingBox( Vector3& minBounds, Vector3& maxBounds ) const
{
	// Get the local bounding box min & max
	if( !GetBoundingBox( minBounds, maxBounds ) )
	{
		return false;
	}
	
	// Transform into world coordinates
	BoundingBoxTransform( minBounds, maxBounds, m_transform );

	return true;
}

const Matrix& Tr2InteriorStatic::GetParentTransform() const
{
	if( m_parentSystem != NULL && m_parentSystem->GetParentCell() != NULL )
	{
		return m_parentSystem->GetParentCell()->GetWorldTransform();
	}
	else
	{
		return Tr2Renderer::GetIdentityTransform();
	}
}

Matrix Tr2InteriorStatic::GetWorldTransform() const
{
	return m_transform * GetParentTransform();
}

// ------------------------------------------------------------------------------------------------------
bool Tr2InteriorStatic::Initialize()
{
	// calc world transform once on init
	D3DXMatrixTransformation( &m_transform, 
							  NULL, 
							  NULL, 
							  NULL, 
							  NULL, 
							  &m_rotation, 
							  &m_position );

	InitializeGeometryResource();
	InitializeOcclusionResource();
	m_isDirty = true;
	return true;
}

// ---------------------------------------------------------------------------------------
bool Tr2InteriorStatic::OnModified( Be::Var* value )
{
	if( IsMatch( value, m_position ) || IsMatch( value, m_rotation ) )
	{
		// position & orientation modify: calc new transform matrix...
		D3DXMatrixTransformation( &m_transform, 
								  NULL, 
								  NULL, 
								  NULL, 
								  NULL, 
								  &m_rotation, 
								  &m_position );

		// Mark our dirty flag
		m_isDirty = true;

		// ...and tell umbra!
		if( m_umbraObject )
		{
			m_umbraObject->setObjectToCellMatrix( AS_UMBRA_MATRIX( m_transform ) );
		}

		for( std::vector<Occluder>::iterator it = m_occluders.begin();
			 it != m_occluders.end(); ++it )
		{
			if( it->m_object )
			{
				it->m_object->setObjectToCellMatrix( AS_UMBRA_MATRIX( m_transform ) );
			}
		}

		if( m_parentSystem )
		{
			m_parentSystem->RebuildBoundingBox();
		}
	}
	else if( IsMatch( value, m_geometryResPath ) )
	{
		InitializeGeometryResource();
	}
	else if( IsMatch( value, m_occlusionResPath ) )
	{
		InitializeOcclusionResource();
	}
	else if( IsMatch( value, m_useOcclusionGeometry ) )
	{
		if( !m_useOcclusionGeometry )
		{
			for( std::vector<Occluder>::iterator it = m_occluders.begin(); 
				 it != m_occluders.end(); ++it )
			{
				if( ( *it ).m_object )
				{
					( *it ).m_object->set( Umbra::Object::ENABLED, false );
				}
			}
		}
		else
		{
			for( std::vector<Occluder>::iterator it = m_occluders.begin(); 
				it != m_occluders.end(); ++it )
			{
				if( ( *it ).m_object )
				{
					( *it ).m_object->set( Umbra::Object::ENABLED, true );
				}
			}
		}
	}

	return true;
}

// ---------------------------------------------------------------------------------------
void Tr2InteriorStatic::Update( Be::Time time )
{
	for( TriCurveSetVector::const_iterator it = m_curveSets.begin(); it != m_curveSets.end(); ++it )
	{
		( *it )->Update( TimeAsDouble( time ) );
	}
}

// ---------------------------------------------------------------------------------------
void Tr2InteriorStatic::SetVisibleLightCount( int visibleLightCount )
{
	m_visibleLightCount = visibleLightCount;
}

// ---------------------------------------------------------------------------------------
void Tr2InteriorStatic::SetVisibleLightSet( const Tr2InteriorLightSet& visibleLightSet )
{
	if( m_drawLightSpider )
	{
		visibleLightSet.GetLightPositions( m_visibleLights );
	}
}

// -------------------------------------------------------------
// Description:
//   Rebinds low level shaders for all meshes of the static. 
//   Uses "StaticObject" and "Interior" situations for detail
//   meshes and "StaticObject", "Interior" and "EnlightenMesh"
//   situation for Enlighten areas.
// -------------------------------------------------------------
void Tr2InteriorStatic::BindLowLevelShaders()
{
	Tr2VariableStore* variableStore = NULL;
	Tr2InteriorEnlightenSystem::EnlightenQuality quality = Tr2InteriorEnlightenSystem::DIRECTIONAL_IRRADIANCE;
	if( m_parentSystem )
	{
		variableStore = &m_parentSystem->GetVariableStore();
		quality = m_parentSystem->GetEnlightenQuality();
	}

	std::vector<unsigned int> localFlags;
	unsigned int h = CcpHashFNV1( "StaticObject", strlen( "StaticObject" ) );
	localFlags.push_back( h );
	h = CcpHashFNV1( "Interior", strlen( "Interior" ) );
	localFlags.push_back( h );

	switch( quality )
	{
	case Tr2InteriorEnlightenSystem::SPHERICAL_HARMONICS:
		h = CcpHashFNV1( "EnlightenSH", strlen( "EnlightenSH" ) );
		localFlags.push_back( h );
		break;
	case Tr2InteriorEnlightenSystem::DIRECTIONAL_IRRADIANCE:
		h = CcpHashFNV1( "EnlightenDirectionalIrradiance", strlen( "EnlightenDirectionalIrradiance" ) );
		localFlags.push_back( h );
		break;
	}
	
	for( PTr2MeshVector::iterator it = m_detailMeshes.begin(); it != m_detailMeshes.end(); ++it )
	{
		( *it )->BindLowLevelShaders( localFlags, false, variableStore );
	}

	Tr2ShaderSituation currentSituation( localFlags );
	m_enlightenAreaMaterial->ApplyMaterialToSituation( currentSituation, false );
	m_enlightenAreaMaterial->SetVariableStore( variableStore );
	m_enlightenAreaMaterial->BindLowLevelShader( currentSituation );
}



// --------------------------------------------------------------------------------------
// Description:
//   Finds an area list to be used for enlighten albedo or emissive rendering.
// Return value:
//   //  returns the primary or fallback area list.
//	 
// See also:
//   Tr2InteriorEnlightenSystem::RenderEnlightenMaterialTextures, Tr2InteriorEnlightenSystem::CanRenderEnlightenMaterialTextures
// --------------------------------------------------------------------------------------

static Tr2MeshAreaVector* GetEnlightenRenderAreas(Tr2Mesh* mesh)
{
	Tr2MeshAreaVector* areas = mesh->GetAreas( TRIBATCHTYPE_OPAQUE_PREPASS );

	if( areas == NULL || areas->empty() )
	{
		areas = mesh->GetAreas( TRIBATCHTYPE_OPAQUE );
	}

	return areas;
}

// --------------------------------------------------------------------------------------
// Description:
//   Checks if the static is ready to render Enlighten textures.
// Return value:
//   true If the static is ready for rendering (all meshes are loaded, all meshes have
//        opaque prepass areas and each area has a shader material)
//	 false Otherwise
// See also:
//   Tr2InteriorEnlightenSystem::RenderEnlightenMaterialTextures
// --------------------------------------------------------------------------------------
bool Tr2InteriorStatic::CanRenderEnlightenMaterialTextures() const
{
	if( m_detailMeshes.empty() )
	{
		return false;
	}

	int enlightenMeshCount = 0 ;

	for( PTr2MeshVector::const_iterator it = m_detailMeshes.begin(); it != m_detailMeshes.end(); ++it )
	{
		if( !( *it )->GetGeometryResource() || !( *it )->GetGeometryResource()->IsPrepared() )
		{
			return false;
		}

		Tr2MeshAreaVector* areas = GetEnlightenRenderAreas( *it );
	
		// still empty! transparent or otherwise not for enlighten to render
		if( areas == NULL || areas->empty() )
		{
			continue; // its ok to have one transparent area admist others.
		}

		for( Tr2MeshAreaVector::iterator area = areas->begin(); area != areas->end(); ++area )
		{
			Tr2ShaderMaterial* material = dynamic_cast<Tr2ShaderMaterial*>( ( *area )->GetMaterialInterface() );
			if( material == NULL || !material->AreParametersPrepared() )
			{
				return false;
			}

		}

		enlightenMeshCount++;

	}
	
	if ( enlightenMeshCount )
	{
		return true;
	}
	
	return false;
}

// --------------------------------------------------------------------------------------
// Description:
//   Renders static into Enlighten albedo/emissive texture.
// Arguments:
//   situationName - name of the material situation to render static with (either albedo
//		or emissive)
// See also:
//   Tr2InteriorEnlightenSystem::RenderEnlightenMaterialTextures
// --------------------------------------------------------------------------------------
void Tr2InteriorStatic::RenderEnlightenMaterialTexture( const char* situationName, Tr2RenderContext &renderContext )
{
	if( !CanRenderEnlightenMaterialTextures() )
	{
		return;
	}

	// Assign new situations to materials.
	std::string name = " ";
	name += situationName;

	for( PTr2MeshVector::const_iterator it = m_detailMeshes.begin(); it != m_detailMeshes.end(); ++it )
	{
		Tr2MeshAreaVector* areas = GetEnlightenRenderAreas(*it);

		// not for enlighten, keep looking.
		if( areas == NULL || areas->empty() )
		{
			continue;
		}

		for( Tr2MeshAreaVector::iterator area = areas->begin(); area != areas->end(); ++area )
		{
			Tr2ShaderMaterial* material = dynamic_cast<Tr2ShaderMaterial*>( ( *area )->GetMaterialInterface() );

			std::string situation = material->GetDefaultSituationString();
			material->SetDefaultSituationString( ( situation + name ).c_str() );
		}
	}

	BindLowLevelShaders();

	// Render the object.
	TriPoolAllocator* allocator = Tr2Renderer::GetPoolAllocator();
	ITriRenderBatchAccumulator* enlightenBatches = CCP_NEW( "Tr2InteriorStatic/enlightenBatches" ) TriRenderBatchAccumulator<>( allocator );

	Tr2PerObjectData* perObjectData = GetPerObjectData( enlightenBatches );

	for( PTr2MeshVector::iterator it = m_detailMeshes.begin(); it != m_detailMeshes.end(); ++it )
	{
		Tr2Mesh* mesh = *it;
		Tr2MeshAreaVector* areas = GetEnlightenRenderAreas(*it);

		// not for enlighten rendering, continue on.
		if( areas == NULL || areas->empty() )
		{
			continue;
		}

		mesh->GetBatches( enlightenBatches, areas, perObjectData );
	}

	renderContext.m_esm.RenderBatches( enlightenBatches );

	enlightenBatches->Clear();
	CCP_DELETE enlightenBatches;

	// Restore original material situations.
	size_t len = name.length();
	for( PTr2MeshVector::const_iterator it = m_detailMeshes.begin(); it != m_detailMeshes.end(); ++it )
	{
		Tr2MeshAreaVector* areas = GetEnlightenRenderAreas(*it);

		// nothing important to enlighten, keep going.
		if( areas == NULL || areas->empty() )
		{
			continue;
		}

		for( Tr2MeshAreaVector::iterator area = areas->begin(); area != areas->end(); ++area )
		{
			Tr2ShaderMaterial* material = dynamic_cast<Tr2ShaderMaterial*>( ( *area )->GetMaterialInterface() );

			std::string situation = material->GetDefaultSituationString();
			material->SetDefaultSituationString( situation.substr( 0, situation.length() - len ).c_str() );
		}
	}

	BindLowLevelShaders();
}

// --------------------------------------------------------------------------------------
// Description:
//   Renders static's occlusion geometry. Assumes that the shader and all render states
//   are already set.
// --------------------------------------------------------------------------------------
void Tr2InteriorStatic::RenderOcclusionGeometry() 
{
	Tr2ConstantBufferAL*	perObjectConstantBuffers[CBUFFER_COUNT];
	for( unsigned i = 0; i != CBUFFER_COUNT; ++i )
	{
		perObjectConstantBuffers[i] = &m_perObjectConstantBuffers[i];
	}

	if( m_occlusionResource && m_occlusionResource->IsGood() )
	{
		if( m_occlusionDebugRenderResource == NULL )
		{
			m_occlusionDebugRenderResource.CreateInstance();

			if( !m_occlusionDebugRenderResource )
			{
				return;
			}

			m_occlusionDebugRenderResource->PrepareFromGrannyRes( m_occlusionResource );
		}
		if( m_occlusionDebugRenderResource->IsGood() )
		{
			Tr2LitPerObjectData data;

			Tr2InteriorPerObjectPSData perObjectPSBuffer;
			Tr2InteriorPerObjectVSData perObjectVSBuffer;

			// 0
			memset( &perObjectPSBuffer, 0, sizeof( perObjectPSBuffer ) );
			memset( &perObjectVSBuffer, 0, sizeof( perObjectVSBuffer ) );

			// column_major for shaders
			D3DXMatrixTranspose( &perObjectVSBuffer.WorldMat, &GetWorldTransform() );

			// Set the mirror-to-world matrix
			D3DXMatrixIdentity( &perObjectPSBuffer.mirrorToWorldMatrix );

			// Do the copy
			data.CopyToPSFloatBuffer( perObjectPSBuffer );
			data.CopyToVSFloatBuffer( perObjectVSBuffer );

			USE_MAIN_THREAD_RENDER_CONTEXT();
			data.SetPerObjectDataToDevice( perObjectConstantBuffers, 0xffFFffFFu, renderContext );

			for( unsigned mesh = 0;
				mesh < m_occlusionDebugRenderResource->GetMeshCount();
				++mesh )
			{
				m_occlusionDebugRenderResource->RenderAsOneArea( mesh );
			}
		}
	}
}

void Tr2InteriorStatic::RenderDebugInfo( TriLineSetPtr lines ) const
{
	if( m_drawLightSpider )
	{
		Color color;
		if( m_visibleLights.size() > MAX_INTERIOR_LIGHTS_PER_OBJECT )
		{
			color = Color( 1.0f, 0.5f, 0.0f, 1.0f );
		}
		else
		{
			color = Color( 0.2f, 1.0f, 0.2f, 1.0f );
		}
		Vector3 center( 0.0f, 0.0f, 0.0f );
		if( m_geometryResource && m_geometryResource->IsGood() && m_geometryResource->GetMeshCount() > 0 )
		{
			Vector3 minBounds, maxBounds;
			m_geometryResource->GetBoundingBox( 0, minBounds, maxBounds );
			for( unsigned int i = 1; i < m_geometryResource->GetMeshCount(); ++i )
			{
				Vector3 meshMinBounds, meshMaxBounds;
				m_geometryResource->GetBoundingBox( i, meshMinBounds, meshMaxBounds );
				if( minBounds.x > meshMinBounds.x )
				{
					minBounds.x = meshMinBounds.x;
				}
				if( minBounds.y > meshMinBounds.y )
				{
					minBounds.y = meshMinBounds.y;
				}
				if( minBounds.z > meshMinBounds.z )
				{
					minBounds.z = meshMinBounds.z;
				}
				if( maxBounds.x < meshMaxBounds.x )
				{
					maxBounds.x = meshMaxBounds.x;
				}
				if( maxBounds.y < meshMaxBounds.y )
				{
					maxBounds.y = meshMaxBounds.y;
				}
				if( maxBounds.z < meshMaxBounds.z )
				{
					maxBounds.z = meshMaxBounds.z;
				}
			}
			center = ( minBounds + maxBounds ) / 2.0f;
		}
		D3DXVec3TransformCoord( &center, &center, &GetWorldTransform() );
		for( std::vector<Vector3>::const_iterator it = m_visibleLights.begin(); it != m_visibleLights.end(); ++it )
		{
			lines->Add( center, color, *it, color );
		}
	}
}

// ---------------------------------------------------------------------------------------
void Tr2InteriorStatic::SetTotalLightCount( int totalLightCount )
{
	m_totalLightCount = totalLightCount;
}

//  Description:
//    Helper function to get per-object data for this renderable using an arbitrary 
//	  light-set and object-to-world matrix.  Both GetPerObjectData and 
//    GetPerObjectDataWithPerInstanceLighting are thin wrappers around this function.
//  See Also:
//    GetPerObjectData, GetPerObjectDataWithPerInstanceLighting
//  Arguments:
//    accumulator -         The batch accumulator used to allocate memory for per-object data
//    lightSet -            The set of lights illuminating this object
//    objectToWorldMatrix - The transformation matrix used to position this object in 
//							world coordinates
//  Return Value:
//    The allocated per-object data, or NULL if the memory allocation failed.
// ---------------------------------------------------------------------------------------
Tr2PerObjectData* Tr2InteriorStatic::GetPerObjectDataWithLightSet( 
										ITriRenderBatchAccumulator* accumulator,
										Tr2InteriorLightSet* lightSet,
										const Matrix& objectToWorldMatrix,
										const Matrix& mirrorToWorldMatrix )
{
	Tr2LitPerObjectData* data = accumulator->Allocate<Tr2LitPerObjectData>();

	if( !data )
	{
		return NULL;
	}

	Tr2InteriorPerObjectPSData perObjectPSBuffer;
	Tr2InteriorPerObjectVSData perObjectVSBuffer;

	// 0
	memset( &perObjectPSBuffer, 0, sizeof( perObjectPSBuffer ) );
	memset( &perObjectVSBuffer, 0, sizeof( perObjectVSBuffer ) );

	// column_major for shaders
	D3DXMatrixTranspose( &perObjectVSBuffer.WorldMat, &objectToWorldMatrix );

	// Copy static object uv transforms
	if( m_visualizeMethod == VM_EN_OBJECT_TEXCOORD )
	{
		perObjectVSBuffer.uvLinearTransform = Vector4( 1.0f, 0.0f, 0.0f, 1.0f );
		perObjectVSBuffer.uvTranslation = Vector2( 0.0f, 0.0f );
	}
	else
	{
		perObjectVSBuffer.uvLinearTransform = m_uvLinearTransform;
		perObjectVSBuffer.uvTranslation = m_uvTranslation;
	}

	// put pointlights in perobject data
	if( lightSet )
	{
		lightSet->PopulateLightData( &perObjectPSBuffer );
		data->SetLightsActive( lightSet->GetNumOfActiveLights(), lightSet->GetNumOfActiveLights() );
	}

	// Set the mirror-to-world matrix
	perObjectPSBuffer.mirrorToWorldMatrix = mirrorToWorldMatrix;

	// Hijack some per-object parameters, depending on the visualize method
	if( m_visualizeMethod == VM_EN_TARGET_DETAIL )
	{
		perObjectPSBuffer.shadowCaster0 = Vector4( 0.1f, 0.1f, 0.5f, 0.4f );
	}
	else if( m_visualizeMethod == VM_EN_ONLY || m_visualizeMethod == VM_ALL_LIGHTING )
	{
		Tr2InteriorEnlightenSystem::EnlightenQuality quality = Tr2InteriorEnlightenSystem::DIRECTIONAL_IRRADIANCE;
		if( m_parentSystem )
		{
			quality = m_parentSystem->GetEnlightenQuality();
		}

		switch( quality )
		{
		case Tr2InteriorEnlightenSystem::SPHERICAL_HARMONICS:
			perObjectPSBuffer.shadowCaster0 = Vector4( 1.0f, 0.0f, 0.0f, 1.0f );
			break;
		case Tr2InteriorEnlightenSystem::DIRECTIONAL_IRRADIANCE:
			perObjectPSBuffer.shadowCaster0 = Vector4( 0.0f, 1.0f, 0.0f, 1.0f );
			break;
		case Tr2InteriorEnlightenSystem::IRRADIANCE:
			perObjectPSBuffer.shadowCaster0 = Vector4( 0.0f, 0.0f, 0.0f, 1.0f );
			break;
		}
	}

	// Do the copy
	data->CopyToPSFloatBuffer( perObjectPSBuffer );
	data->CopyToVSFloatBuffer( perObjectVSBuffer );

	D3DXMatrixInverse( &m_mirrorToWorldMatrix, NULL, D3DXMatrixTranspose( &m_mirrorToWorldMatrix, &mirrorToWorldMatrix ) );

	return data;
}

// ---------------------------------------------------------------------------------------
Tr2PerObjectData* Tr2InteriorStatic::GetPerObjectData( 
										ITriRenderBatchAccumulator* accumulator )
{
	return GetPerObjectDataWithLightSet( accumulator, 
										 &m_lightSet, 
										 GetWorldTransform(), 
										 Tr2Renderer::GetIdentityTransform() );
}

// ---------------------------------------------------------------------------------------
bool Tr2InteriorStatic::HasTransparentBatches()
{
	for( PTr2MeshVector::iterator it = m_detailMeshes.begin(); 
		it != m_detailMeshes.end(); ++it )
	{
		Tr2Mesh* mesh = *it;

		if( mesh->GetAreas( TRIBATCHTYPE_TRANSPARENT ) && 
			!mesh->GetAreas( TRIBATCHTYPE_TRANSPARENT )->empty() )
		{
			return true;
		}
	}

	return false;
}


// ---------------------------------------------------------------------------------------
// Description:
//   Gets the batches for rendering the Tr2InteriorStatic, depending on various settings
//   including if the target / detail meshes are being rendered, and if we're visualizing
//   the albedo
// SeeAlso:
//   GetEnlightenTargetMeshOpaqueBatches
//   GetEnlightenDetailMeshOpaqueBatches
// ---------------------------------------------------------------------------------------
void Tr2InteriorStatic::GetBatches( ITriRenderBatchAccumulator* batches,
									TriBatchType batchType,
								    const Tr2PerObjectData* perObjectData )
{
	// If display is off, nothing gets displayed
	if( !m_display )
	{
		return;
	}

	// For albedo visualization we shouldn't render prepass as this might
	// occlude target meshes redered during lit material pass
	if( batchType == TRIBATCHTYPE_DEPTHNORMAL && ( m_visualizeMethod == VM_EN_ALBEDO || m_visualizeMethod == VM_EN_TARGET_CHARTS ) )
	{
		return;
	}

	if( batchType == TRIBATCHTYPE_OPAQUE || batchType == TRIBATCHTYPE_OPAQUE_PREPASS )
	{
		// Gather batches for the Enlighten areas
		// These areas are always and only opaque
		if( m_displayTargetMesh 
			|| m_visualizeMethod == VM_EN_ALBEDO 
			|| m_visualizeMethod == VM_EN_TARGET_DETAIL
			|| m_visualizeMethod == VM_EN_TARGET_CHARTS )
		{
			//When getting batches for target/detail vis, temporarily override
			// the rendering mode (and set it back when we're done)
			const Tr2EffectStateManager::RenderingMode oldRenderingMode = batches->GetRenderingMode();
			if( m_visualizeMethod == VM_EN_TARGET_DETAIL )
			{
				batches->SetRenderingMode( Tr2EffectStateManager::RM_VIS_WIREFRAME );
			}
			GetEnlightenTargetMeshOpaqueBatches( batches, perObjectData );
			batches->SetRenderingMode( oldRenderingMode );
			
		}

		// Gather batches for the Enlighten detail meshes
		if( m_displayDetailMeshes && m_visualizeMethod != VM_EN_ALBEDO && m_visualizeMethod != VM_EN_TARGET_CHARTS )
		{
			GetEnlightenDetailMeshOpaqueBatches( batches, perObjectData, batchType );
		}
	}
	else if( batchType == TRIBATCHTYPE_DECAL || batchType == TRIBATCHTYPE_DECAL_PREPASS )
	{
		// Don't display decal batches if we're rendering the albedo
		// (only exists on the enlighten areas)
		// or target/detail mode (transparent areas obscure detail/target status)
		if( m_displayDetailMeshes
			&& m_visualizeMethod != VM_EN_ALBEDO 
			&& m_visualizeMethod != VM_EN_TARGET_DETAIL )
		{
			for( PTr2MeshVector::iterator it = m_detailMeshes.begin(); 
				it != m_detailMeshes.end(); 
				++it )
			{
				Tr2Mesh* mesh = *it;

				mesh->GetBatches( batches, 
					mesh->GetAreas( batchType ), 
					perObjectData );
			}
		}
	}
	else if( batchType == TRIBATCHTYPE_TRANSPARENT )
	{
		// Don't display transparent batches if we're rendering the albedo
		// (only exists on the enlighten areas)
		// or target/detail mode (transparent areas obscure detail/target status)
		if( m_displayDetailMeshes 
			&& m_visualizeMethod != VM_EN_ALBEDO 
			&& m_visualizeMethod != VM_EN_TARGET_DETAIL )
		{
			for( PTr2MeshVector::iterator it = m_detailMeshes.begin(); 
				it != m_detailMeshes.end(); 
				++it )
			{
				Tr2Mesh* mesh = *it;

				GetTransparentBatches( batches, mesh, perObjectData );
			}
		}
	}
	else if( m_displayDetailMeshes )
	{
		// In any other case, look at the detail meshes
		for( PTr2MeshVector::iterator it = m_detailMeshes.begin(); 
			it != m_detailMeshes.end(); 
			++it )
		{
			Tr2Mesh* mesh = *it;

			mesh->GetBatches( batches, 
				mesh->GetAreas( batchType ), 
				perObjectData );
		}
	}
}
// ---------------------------------------------------------------------------------------
float Tr2InteriorStatic::GetSortValue()
{
	Vector3 min, max;
	GetBoundingBox( min, max );

	Matrix worldTransform = GetWorldTransform();

	// Transform the local BB min & max to world coordinates
	D3DXVec3TransformCoord( &min, &min, &worldTransform );
	D3DXVec3TransformCoord( &max, &max, &worldTransform );

	Vector3 center = 0.5f * ( max + min );

	Vector3 d = Tr2Renderer::GetViewPosition() - center;
	return D3DXVec3Length( &d );
}

// ---------------------------------------------------------------------------------------
void Tr2InteriorStatic::SetVisibility( bool bVisible )
{
	m_isVisible = bVisible;
	if( m_isVisible && m_parentSystem )
	{
		m_parentSystem->SetVisibility( true );
	}
}

// ---------------------------------------------------------------------------------------
CullResult Tr2InteriorStatic::AddToCell( Umbra::Cell* cell )
{
	// if mesh is not set this fails!
	if( !m_geometryResource )
	{
		return CULLRES_FAILED;
	}

	// if this mesh is still getting loaded, we are not able to obtain bounding info yet!
	if( m_geometryResource->IsLoading() )
	{
		return CULLRES_NOTREADY;
	}

	// now we should get bounding info
	Vector3 minBounds, maxBounds;
	if( !GetBoundingBox( minBounds, maxBounds ) )
	{
		return CULLRES_FAILED;
	}

	// create bounding box and add to cell
	m_umbraModel = ( Umbra::Model* )Umbra::OBBModel::create( 
									AS_UMBRA_VECTOR3( minBounds ), 
									AS_UMBRA_VECTOR3( maxBounds ) );

	m_umbraObject = Umbra::Object::create( m_umbraModel );
	
	if( m_umbraObject )
	{
		m_umbraObject->setCell( cell );
		m_umbraObject->setObjectToCellMatrix( AS_UMBRA_MATRIX( m_transform ) );

		ITr2Interior* interiorObject = dynamic_cast<ITr2Interior*>( this );

		m_umbraObject->setUserPointer( CONVERT_TO_UMBRA_USER_DATA( interiorObject ) );
	}

	RebuildOcclusionObjects();

	if( m_parentSystem )
	{
		BoundingBoxTransform( minBounds, maxBounds, m_transform );
		m_parentSystem->UpdateBoundingBox( minBounds, maxBounds );
	}

	return CULLRES_OK;
}

// ---------------------------------------------------------------------------------------
void Tr2InteriorStatic::RemoveFromCell()
{
	if( m_umbraModel )
	{
		m_umbraModel->release();
		m_umbraModel = NULL;
	}

	if( m_umbraObject )
	{
		m_umbraObject->setCell( NULL );
		m_umbraObject->release();
		m_umbraObject = NULL;
	}

	DestroyOcclusionObjects();

	m_parentSystem = NULL;
}

// ---------------------------------------------------------------------------------------
void Tr2InteriorStatic::SetParentSystem( Tr2InteriorEnlightenSystem* parentSystem )
{
	m_parentSystem = parentSystem;

	if( m_parentSystem )
	{
		if( m_parentSystem->GetParentCell() && m_parentSystem->GetParentCell()->GetUmbraCell() )
		{
			AddToCell( m_parentSystem->GetParentCell()->GetUmbraCell() );
			BindLowLevelShaders();
		}
	}
	else
	{
		RemoveFromCell();
	}
}

// ---------------------------------------------------------------------------------------
void Tr2InteriorStatic::SetVisualizeMethod( VisualizeMethod method )
{
	m_visualizeMethod = method;
}

// ---------------------------------------------------------------------------------------
//  Description:
//    Gets per-object data for the static using a per-instance light-set override 
//    and an arbitrary object-to-world matrix.  Routes the call to helper function 
//    GetPerObjectDataWithLightSet.
//  See Also:
//    GetPerObjectData, GetPerObjectDataWithLightSet
//  Arguments:
//    accumulator -         The batch accumulator used to allocate memory for per-object data
//    lightSet -            The set of lights illuminating this object
//    objectToWorldMatrix - The transformation matrix used to position this object 
//                          in world coordinates
//  Return Value:
//    The allocated per-object data, or NULL if the memory allocation failed.
// ---------------------------------------------------------------------------------------
Tr2PerObjectData* Tr2InteriorStatic::GetPerObjectDataWithPerInstanceLighting( 
										ITriRenderBatchAccumulator* accumulator,
										Tr2InteriorLightSet* lightSet,
																			  const Matrix& objectToWorldMatrix,
																			  const Matrix& mirrorToWorldMatrix )
{
	return GetPerObjectDataWithLightSet( accumulator, 
										 lightSet, 
										 objectToWorldMatrix, 
										 mirrorToWorldMatrix );
}

// ---------------------------------------------------------------------------------------
//  Description:
//    Gets per-object data for the static using a reduced per-object data optimized for
//    pre-pass.
//  See Also:
//    GetPerObjectData, GetPerObjectDataWithPerInstanceLighting
//  Arguments:
//    accumulator -         The batch accumulator used to allocate memory for per-object data
//    objectToWorldMatrix - The transformation matrix used to position this object 
//                          in world coordinates
//  Return Value:
//    The allocated per-object data, or NULL if the memory allocation failed.
// ---------------------------------------------------------------------------------------
Tr2PerObjectData* Tr2InteriorStatic::GetPerObjectDataForPrePass( 
	ITriRenderBatchAccumulator* accumulator,
	const Matrix& objectToWorldMatrix )
{
	Tr2PerObjectDataPrePass* data = accumulator->Allocate<Tr2PerObjectDataPrePass>();

	if( !data )
	{
		return NULL;
	}

	Tr2InteriorPerObjectVSData perObjectVSBuffer;

	// 0
	memset( &perObjectVSBuffer, 0, sizeof( perObjectVSBuffer ) );

	// column_major for shaders
	D3DXMatrixTranspose( &perObjectVSBuffer.WorldMat, &objectToWorldMatrix );

	// Do the copy
	data->CopyToVSFloatBuffer( perObjectVSBuffer );

	return data;	
}

// ---------------------------------------------------------------------------------------
TriGeometryRes* Tr2InteriorStatic::GetGeometryResource() const
{
	if( m_geometryResource )
	{
		return m_geometryResource;
	}

	return NULL;
}

std::string Tr2InteriorStatic::GetGeometryResourcePath() const
		{
	return m_geometryResPath;
}

// --------------------------------------------------------------------------------------
// Description:
//   Gets the dirty flag
// Return Value:
//   true, if the static is idrty
//   false, otherwise
// --------------------------------------------------------------------------------------
bool Tr2InteriorStatic::IsDirty( void ) const
{
	return m_isDirty;
}

// --------------------------------------------------------------------------------------
// Description:
//   Resets the dirty flag
// --------------------------------------------------------------------------------------
void Tr2InteriorStatic::ResetDirtyFlag( void )
{
	m_isDirty = false;
}

void Tr2InteriorStatic::SetEnlightenInstanceTransform( float linearTransform[2][2], 
													   float translation[2] )
{
	m_uvLinearTransform.x = linearTransform[0][0];
	m_uvLinearTransform.y = linearTransform[0][1];
	m_uvLinearTransform.z = linearTransform[1][0];
	m_uvLinearTransform.w = linearTransform[1][1];

	m_uvTranslation.x = translation[0];
	m_uvTranslation.y = translation[1];
}

const PTr2EnlightenAreaVector& Tr2InteriorStatic::GetEnlightenAreas() const
{
	return m_enlightenAreas;
}

namespace {

struct ImmutableHelper : IBlueResManNotifications
{
	void OnResourceCreated( void* res )
	{
		static_cast<TriGeometryRes*>(res)->m_immutable = true;
	}

	void OnResourceFromCache( void* res ) {}
};

}


void Tr2InteriorStatic::InitializeGeometryResource()
{
	if( m_geometryResource )
	{
		m_geometryResource->RemoveNotifyTarget( this );
		m_geometryResource.Unlock();
	}
	if( !m_geometryResPath.empty() )
	{
		BeResMan->GetResource( m_geometryResPath.c_str(), "", m_geometryResource, &ImmutableHelper() );
		m_geometryResource->AddNotifyTarget( this );
	}
}

void Tr2InteriorStatic::InitializeOcclusionResource( void )
{
	if( m_occlusionResource )
	{
		m_occlusionResource->RemoveNotifyTarget( this );
		m_occlusionResource.Unlock();
	}
	if( !m_occlusionResPath.empty() )
	{
		BeResMan->GetResource( m_occlusionResPath.c_str(), "raw", m_occlusionResource );
		m_occlusionResource->AddNotifyTarget( this );
	}
	m_occlusionDebugRenderResource = NULL;
}

void Tr2InteriorStatic::ReleaseCachedData( BlueAsyncRes* p )
{
	if( p == m_occlusionResource )
	{
		DestroyOccluders();
	}
	else if( p == m_geometryResource )
	{
		if( m_parentSystem )
		{
			m_parentSystem->RebuildBoundingBox();
		}
	}
}

void Tr2InteriorStatic::RebuildCachedData( BlueAsyncRes* p )
{
	if( p == m_occlusionResource )
	{
		BuildOcclusionGeometry();
	}
	else if( p == m_geometryResource )
	{
		if( p->IsGood() )
		{
			if( m_parentSystem && m_parentSystem->GetParentCell() && m_parentSystem->GetParentCell()->GetUmbraCell() )
			{
				AddToCell( m_parentSystem->GetParentCell()->GetUmbraCell() );
				BindLowLevelShaders();
			}
		}
	}
}

void Tr2InteriorStatic::BuildOcclusionGeometry( void )
{
	if( m_occlusionResource )
	{
		DestroyOccluders();

		// Create umbra objects for occluder
		granny_file* f = m_occlusionResource->GetGrannyFile();
		if( f )
		{
			granny_file_info* info = GrannyGetFileInfo( f );
			if( info )
			{
				for( int meshIx = 0; meshIx < info->MeshCount; ++meshIx )
				{
					Occluder occluder;
					occluder.m_model = TriGrannyRes::CreateUmbraMeshFromGrannyMesh( info->Meshes[meshIx], true );
					occluder.m_object = NULL;
					if( occluder.m_model )
					{

						m_occluders.push_back( occluder );
					}
				}
				RebuildOcclusionObjects();
			}
		}
	}
}

void Tr2InteriorStatic::DestroyOccluders( void )
{	
	for( std::vector<Occluder>::iterator it = m_occluders.begin();
		 it != m_occluders.end(); ++it )
	{
		Occluder& occluder = ( *it );
		
		if( occluder.m_model )
		{
			occluder.m_model->release();
			occluder.m_model = NULL;
		}

		if( occluder.m_object )
		{
			occluder.m_object->setCell( NULL );
			occluder.m_object->release();
			occluder.m_object = NULL;
		}
	}

	m_occluders.clear();
}

// -------------------------------------------------------------
// Description:
//   Rebuilds Umbra occlusion objects for each loaded Umbra
//   occlusion model.
// -------------------------------------------------------------
void Tr2InteriorStatic::RebuildOcclusionObjects()
{
	for( std::vector<Occluder>::iterator it = m_occluders.begin();
		 it != m_occluders.end(); ++it )
	{
		Occluder& occluder = ( *it );

		if( occluder.m_object )
		{
			occluder.m_object->setCell( NULL );
			occluder.m_object->release();
			occluder.m_object = NULL;
		}

		occluder.m_object = Umbra::Object::create( occluder.m_model );
		if( occluder.m_object )
		{
			// The object is only for occluding - doesn't need to render
			occluder.m_object->set( Umbra::Object::INFORM_VISIBLE, false );

			if( m_umbraObject )
			{
				occluder.m_object->setCell( m_umbraObject->getCell() );
			}
			else
			{
				occluder.m_object->setCell( NULL );
			}
			occluder.m_object->setWriteModel( occluder.m_model );
			occluder.m_object->setObjectToCellMatrix( AS_UMBRA_MATRIX( m_transform ) );

			if( !m_useOcclusionGeometry )
			{
				occluder.m_object->set( Umbra::Object::ENABLED, false );
			}
		}
	}
}

// -------------------------------------------------------------
// Description:
//   Destroys Umbra occlusion objects, but leaves occlusion 
//   models alive.
// -------------------------------------------------------------
void Tr2InteriorStatic::DestroyOcclusionObjects()
{
	for( std::vector<Occluder>::iterator it = m_occluders.begin();
		 it != m_occluders.end(); ++it )
	{
		Occluder& occluder = ( *it );

		if( occluder.m_object )
		{
			occluder.m_object->setCell( NULL );
			occluder.m_object->release();
			occluder.m_object = NULL;
		}
	}
}

void Tr2InteriorStatic::GetEnlightenTargetMeshOpaqueBatches( 
							ITriRenderBatchAccumulator* batches, 
							const Tr2PerObjectData* perObjectData )
{
	for( PTr2EnlightenAreaVector::iterator it = m_enlightenAreas.begin(); 
		 it != m_enlightenAreas.end(); 
		 ++it )
	{
		Tr2EnlightenArea* area = *it;

		if( area->IsHidden() )
		{
			continue;
		}

		// Allocate a new per-object data buffer, depending on the visualizer method
		Tr2LitPerObjectData* overrideData = NULL;
		if( m_visualizeMethod == VM_EN_ALBEDO )
		{
			overrideData = batches->Allocate<Tr2LitPerObjectData>();

			if( !overrideData )
			{
				return;
			}

			const Tr2LitPerObjectData* oldData = dynamic_cast<const Tr2LitPerObjectData*>( perObjectData );

			memcpy( overrideData, oldData, sizeof( Tr2LitPerObjectData ) );

			Tr2InteriorPerObjectPSData perObjectPSBuffer;
			memset( &perObjectPSBuffer, 0, sizeof( perObjectPSBuffer ) );

			const Color& color = area->GetAlbedoColor();
			perObjectPSBuffer.shadowCaster0 = Vector4( color.r, color.g, color.b, 1.0f );

			overrideData->CopyToPSFloatBuffer( perObjectPSBuffer );
		}
		else if( m_visualizeMethod == VM_EN_TARGET_DETAIL )
		{
			overrideData = batches->Allocate<Tr2LitPerObjectData>();

			if( !overrideData )
			{
				return;
			}

			const Tr2LitPerObjectData* oldData = dynamic_cast<const Tr2LitPerObjectData*>( perObjectData );

			memcpy( overrideData, oldData, sizeof( Tr2LitPerObjectData ) );

			Tr2InteriorPerObjectPSData perObjectPSBuffer;
			memset( &perObjectPSBuffer, 0, sizeof( perObjectPSBuffer ) );
			perObjectPSBuffer.shadowCaster0 = Vector4( 0.3f, 0.3f, 0.3f, 0.1f );

			overrideData->CopyToPSFloatBuffer( perObjectPSBuffer );
		}

		TriGeometryBatch* batch = batches->Allocate<TriGeometryBatch>();
		// Note that this can fail if the accumulator can't add more batches!
		if( batch )
		{
			batch->SetShaderMaterial( m_enlightenAreaMaterial ); 

			if( m_visualizeMethod == VM_EN_ALBEDO || m_visualizeMethod == VM_EN_TARGET_DETAIL )
			{
				batch->SetPerObjectData( overrideData );
			}
			else
			{
				batch->SetPerObjectData( perObjectData );
			}
			batch->SetGeometryResource( m_geometryResource );
			batch->SetMeshParameters( 0, area->GetIndex(), area->GetCount(), area->GetReversed() );

			batches->Commit( batch );
		}
	}
}

void Tr2InteriorStatic::GetEnlightenDetailMeshOpaqueBatches( 
							ITriRenderBatchAccumulator* batches, 
							const Tr2PerObjectData* perObjectData,
							TriBatchType batchType )
{
	// Allocate a new per-object data buffer, depending on the visualizer method
	Tr2LitPerObjectData* overrideData = NULL;
	if( m_visualizeMethod == VM_EN_TARGET_DETAIL )
	{
		overrideData = batches->Allocate<Tr2LitPerObjectData>();

		if( !overrideData )
		{
			return;
		}

		const Tr2LitPerObjectData* oldData = dynamic_cast<const Tr2LitPerObjectData*>( perObjectData );

		memcpy( overrideData, oldData, sizeof( Tr2LitPerObjectData ) );

		Tr2InteriorPerObjectPSData perObjectPSBuffer;
		memset( &perObjectPSBuffer, 0, sizeof( perObjectPSBuffer ) );
		perObjectPSBuffer.shadowCaster0 = Vector4( 0.5f, 0.1f, 0.1f, 1.0f );

		overrideData->CopyToPSFloatBuffer( perObjectPSBuffer );
	}

	for( PTr2MeshVector::iterator it = m_detailMeshes.begin(); it != m_detailMeshes.end(); ++it )
	{
		Tr2Mesh* mesh = *it;
		if( m_visualizeMethod == VM_EN_TARGET_DETAIL )
		{
			mesh->GetBatches( batches, mesh->GetAreas( batchType ), overrideData );
		}
		else
		{
			mesh->GetBatches( batches, mesh->GetAreas( batchType ), perObjectData );
		}
	}
}

void Tr2InteriorStatic::GetTransparentBatches( ITriRenderBatchAccumulator* batches,
											   Tr2Mesh* mesh,
											   const Tr2PerObjectData* data )
{
	// Only gather transparent batches if the mesh isn't hidden
	if( !mesh->IsHidden() )
{
		// Get the transparent areas
		Tr2MeshAreaVector* areas = mesh->GetAreas( TRIBATCHTYPE_TRANSPARENT );

		if( areas )
		{
			float maxDepth = Tr2Renderer::GetFrustumRadius();
			Matrix instanceTransform = m_transform * m_mirrorToWorldMatrix;

			// Loop over the transparent areas
			for( Tr2MeshAreaVector::const_iterator it = areas->begin(); it != areas->end(); ++it )
{
				Tr2MeshArea* area = *it;
				ITr2ShaderMaterial* shaderMat = area->GetMaterialInterface();

				if( area->IsHidden() )
				{
					continue;
}

				if( !shaderMat )
{
					continue;
}

				// Compute the depth
				Vector3 bbMin, bbMax;
				mesh->GetAreaBoundingBox( area->GetIndex(), bbMin, bbMax );
				Vector3 center = 0.5f * ( bbMin + bbMax );
				D3DXVec3TransformCoord( &center, &center, &instanceTransform );
				center -= Tr2Renderer::GetViewPosition();
				float z = std::min( std::max( ( D3DXVec3Length( &center ) + m_depthOffset ) / maxDepth, 0.f ), 1.f );

				unsigned int depth = ( unsigned int )( ( float )0xFFFFFFF * ( 1.0f - z ) );

				const Tr2PerObjectData *perAreaData = data;

				if( m_shSolver && area->GetUseSHLighting() )
				{
					Tr2PerAreaSHLightingData* areaData = batches->Allocate<Tr2PerAreaSHLightingData>();
					if( areaData )
					{
						Vector3 minBounds, maxBounds;
						mesh->GetAreaBoundingBox( area->GetIndex(), minBounds, maxBounds );
						for( int i = 1; i < area->GetCount(); ++i )
						{
							Vector3 min, max;
							mesh->GetAreaBoundingBox( area->GetIndex() + i, min, max );
							BoundingBoxUpdate( minBounds, maxBounds, min, max );
						}

						areaData->SetPerObjectData( data );
						m_shSolver->AddVolume( minBounds, maxBounds, m_transform, areaData );
						perAreaData = areaData;
					}
				}

				TriGeometryBatch* batch = batches->Allocate<TriGeometryBatch>();
				// Note that this can fail if the accumulator can't add more batches!
				if( batch )
				{
					batch->SetShaderMaterial( shaderMat ); 
					batch->SetPerObjectData( perAreaData );
					batch->SetGeometryResource( mesh->GetGeometryResource() );

					batch->SetMeshParameters( mesh->GetMeshIndex(), area->GetIndex(), area->GetCount(), area->GetReversed() );
					batch->SetDepth( depth );

					batches->Commit( batch );
				}
			}
		}
	}
}

#endif
