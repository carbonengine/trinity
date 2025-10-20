////////////////////////////////////////////////////////////
//
//    Created:   March 2013
//    Copyright: CCP 2013
//
#include "StdAfx.h"
#include "TriRenderBatch.h"
#include "TriFrustum.h"
#include "Shader/Tr2Effect.h"
#include "EvePlaneSet.h"
#include "EvePlaneSetItem.h"
#include "Utilities/BoundingSphere.h"
#include "Tr2DebugRenderer.h"
#include "Tr2Renderer.h"
#include "Utilities/MatrixUtils.h"
#include "Shader/Parameter/TriTextureParameter.h"
#include "Resources/TriTextureRes.h"
#include "Resources/Tr2LightProfileRes.h"
#include "Tr2QuadRenderer.h"

EvePlaneLight::EvePlaneLight() :
	lightData( LightData() ),
	index( 0 ),
	saturation( 1.0f ),
	boneMatrix( IdentityMatrix() ),
	fadeType( EveSpaceObjectAttachmentUtils::FT_NONE ),
	blinkPhase( 0.0f ),
	blinkRate( 0.0f )
{
}

EvePlaneLight::EvePlaneLight( const LightData& lightData, float saturation, uint32_t index, const std::wstring& profilePath, EveSpaceObjectAttachmentUtils::FadeType fade, float blinkPhase, float blinkRate ) :
	lightData( lightData ),
	saturation( saturation ),
	index( index ),
	boneMatrix( IdentityMatrix() ),
	fadeType( fade ),
	blinkPhase( blinkPhase ),
	blinkRate( blinkRate )
{
	if( !profilePath.empty() )
	{
		BeResMan->GetResource( profilePath, L"lp", lightProfile );
	}
}

// vertex layout struct
struct PlaneVertex
{
	Vector4 transform1;
	Vector4 transform2;
	Vector4 transform3;
	Vector4 color;
	Vector4 layer1Transform;
	Vector4 layer2Transform;
	Vector4 layer1Scroll;
	Vector4 layer2Scroll;
	uint8_t index;
	uint8_t boneIndex;
	uint8_t maskMapAtlasIndex;
	uint8_t pickBufferID;
	Vector4 blinkData;
};



using namespace Tr2RenderContextEnum;

// --------------------------------------------------------------------------------
// Description:
//   Initialize data members
// --------------------------------------------------------------------------------
EvePlaneSet::EvePlaneSet( IRoot* lockobj ) :
	PARENTLOCK( m_planes ),
	m_display( true ),
	m_hideOnLowQuality( false ),
	m_isSkinned( false ),
	m_pickBufferID( 0 ),
	m_activationStrength( 0 ),
	m_effectHash( 0 )
{
}

// --------------------------------------------------------------------------------
// Description:
//   Cleanup
// --------------------------------------------------------------------------------
EvePlaneSet::~EvePlaneSet()
{
}

// --------------------------------------------------------------------------------
// Description:
//   Set the main effect of this set from the outside
// --------------------------------------------------------------------------------
void EvePlaneSet::SetEffect( Tr2EffectPtr effect )
{
	m_effect = effect;
}

// --------------------------------------------------------------------------------
// Description:
//   If loading from a .red file, we now can start creating resources
// --------------------------------------------------------------------------------
bool EvePlaneSet::Initialize()
{
	CreateBoundingBoxes();
	if( m_effect )
	{
		m_effectHash = m_effect->GetHashValue();
	}
	return true;
}

// --------------------------------------------------------------------------------
// Description:
//   If someone changed some data we must re-create buffers etc.
// --------------------------------------------------------------------------------
bool EvePlaneSet::OnModified( Be::Var* val )
{
	if( IsMatch( val, m_pickBufferID ) )
	{
		Rebuild();
	}
	return true;
}

// --------------------------------------------------------------------------------
// Description:
//   Allow access to change the pickbuffer ID. (this is so the SOF can set the
//   pickbuffers for hangar/space videos
// --------------------------------------------------------------------------------
void EvePlaneSet::SetPickBufferID( uint8_t pickBufferID )
{
	m_pickBufferID = pickBufferID;

	if( m_planes.empty() )
	{
		return;
	}

	Rebuild();
}

void EvePlaneSet::SetIsSkinned( bool isSkinned )
{
	m_isSkinned = isSkinned;
}

void EvePlaneSet::RegisterWithQuadRenderer( Tr2QuadRenderer& quadRenderer )
{
	if( m_effect )
	{
		m_effectHash = m_effect->GetHashValue();
	}

	static Tr2VertexDefinition s_spriteVertexDecl;
	if( s_spriteVertexDecl.empty() )
	{
		Tr2VertexDefinition& vd = s_spriteVertexDecl;
		vd.Add( vd.FLOAT32_4, vd.TEXCOORD, 0, 1, 1 );
		vd.Add( vd.FLOAT32_4, vd.TEXCOORD, 1, 1, 1 );
		vd.Add( vd.FLOAT32_4, vd.TEXCOORD, 2, 1, 1 );
		vd.Add( vd.FLOAT32_4, vd.COLOR, 0, 1, 1 );
		vd.Add( vd.FLOAT16_4, vd.TEXCOORD, 3, 1, 1 );
		vd.Add( vd.FLOAT16_4, vd.TEXCOORD, 4, 1, 1 );
		vd.Add( vd.FLOAT16_4, vd.TEXCOORD, 5, 1, 1 );
		vd.Add( vd.FLOAT16_4, vd.TEXCOORD, 6, 1, 1 );
		vd.Add( vd.FLOAT16_4, vd.TEXCOORD, 8, 1, 1 );
		vd.Add( vd.UBYTE_4, vd.TEXCOORD, 7, 1, 1 );
	}

	quadRenderer.RegisterEffect( m_effectHash, TRIBATCHTYPE_ADDITIVE, sizeof( PlaneVertex ), 1, s_spriteVertexDecl, m_effect );
}

void EvePlaneSet::AddToQuadRenderer( Tr2QuadRenderer& quadRenderer, const Matrix& parentTransform, float activation, float boosterGain, const granny_matrix_3x4* bones, size_t boneCount )
{
	if( !m_display )
	{
		return;
	}
	if( m_hideOnLowQuality && Tr2Renderer::IsLowQuality() )
	{
		return;
	}
	if( m_items.empty() )
	{
		return;
	}

	Matrix boneTransform = IdentityMatrix();
	size_t idx = 0;

	if ( m_isSkinned )
	{
		for( auto& vertex : m_items )
		{
			// build transformation matrix out of the individual item data
			const auto& data = m_volatileData[idx++];
			auto boneIndex = vertex.boneIndex;
			Matrix transform;
			if( boneIndex < boneCount )
			{
				TriMatrixCopyFrom3x4( &boneTransform, &bones[boneIndex] );
				transform = data.transform * boneTransform * parentTransform;
			}
			else
			{
				transform = data.transform * parentTransform;
			}
			vertex.transform1 = Vector4( transform._11, transform._21, transform._31, transform._41 );
			vertex.transform2 = Vector4( transform._12, transform._22, transform._32, transform._42 );
			vertex.transform3 = Vector4( transform._13, transform._23, transform._33, transform._43 );
			vertex.color = data.color * activation;
		}

	}
	else
	{
		for( auto& vertex : m_items )
		{
			// build transformation matrix out of the individual item data
			const auto& data = m_volatileData[idx++];
			auto transform = data.transform * parentTransform;

			vertex.transform1 = Vector4( transform._11, transform._21, transform._31, transform._41 );
			vertex.transform2 = Vector4( transform._12, transform._22, transform._32, transform._42 );
			vertex.transform3 = Vector4( transform._13, transform._23, transform._33, transform._43 );
			vertex.color = data.color * activation;
		}
	}

	quadRenderer.AddQuads( m_effectHash, m_items.data(), m_items.size() );
}

// --------------------------------------------------------------------------------------
// Description:
//   Get bounding box around planes, update visibility based on if box is visible or not
// --------------------------------------------------------------------------------------
bool EvePlaneSet::UpdateVisibility( const EveUpdateContext& updateContext, const Matrix& parentTransform, const granny_matrix_3x4* bones, size_t boneCount )
{
	auto aabb = GetAabb( bones, boneCount );
	if( !aabb.IsInitialized() )
	{
		return false;
	}
	aabb.Transform( parentTransform );

	return updateContext.GetFrustum().IsBoxVisible( aabb.m_min, aabb.m_max );
}

void EvePlaneSet::UpdateLights( const Matrix& parentTransform, const granny_matrix_3x4* bones, size_t boneCount, float activationStrength, float boosterGain )
{
	for( auto& light : m_lights ) 
	{
		if( light.lightData.boneIndex > 0 && light.lightData.boneIndex < boneCount )
		{
			TriMatrixCopyFrom3x4( &( light.boneMatrix ), &bones[light.lightData.boneIndex] );
			light.boneMatrix._14 = 0.f;
			light.boneMatrix._24 = 0.f;
			light.boneMatrix._34 = 0.f;
			light.boneMatrix._44 = 1.f;
			light.boneMatrix *= parentTransform;
		}
		else
		{
			light.boneMatrix = parentTransform;
		}
	}
	m_activationStrength = activationStrength;
}

// --------------------------------------------------------------------------------------
// Description:
//   Get bounding box surrounding planes
// --------------------------------------------------------------------------------------
AxisAlignedBoundingBox EvePlaneSet::GetAabb( const granny_matrix_3x4* bones, size_t boneCount ) const
{
	return GetItemSetAabb( m_aabb, m_boundingBoxes, bones, boneCount );
}

// --------------------------------------------------------------------------------
// Description:
//   Trinity's way of providing batches to render
// --------------------------------------------------------------------------------
void EvePlaneSet::GetBatches( ITriRenderBatchAccumulator* accumulator, TriBatchType batchType, const Tr2PerObjectData* perObjectData, Tr2RenderReason reason )
{
}

// --------------------------------------------------------------------------------
// Description:
//   Rebuild resources after adding/removing/changing individual planes
// --------------------------------------------------------------------------------
void EvePlaneSet::Rebuild()
{
	m_items.clear();
	m_items.reserve( m_planes.size() );
	m_volatileData.clear();
	m_volatileData.reserve( m_planes.size() );
	for( auto& plane : m_planes )
	{
		if( plane->m_color == Color( 0, 0, 0, 0 ) )
		{
			continue;
		}
		auto itemTransform = TransformationMatrix( plane->m_scaling, plane->m_rotation, plane->m_position );
		m_volatileData.push_back( { itemTransform, Vector4( plane->m_color.r, plane->m_color.g, plane->m_color.b, plane->m_color.a ) } );

		PlaneVertex item;
		item.layer1Transform = Vector4_16( plane->m_layer1Transform );
		item.layer2Transform = Vector4_16( plane->m_layer2Transform );
		item.layer1Scroll = Vector4_16( plane->m_layer1Scroll );
		item.layer2Scroll = Vector4_16( plane->m_layer2Scroll );
		item.blinkData = Vector4_16( plane->m_blinkData );
		item.boneIndex = plane->m_boneIndex;
		item.maskMapAtlasIndex = plane->m_maskAtlasID;
		m_items.push_back( item );
	}

	CreateBoundingBoxes();
}

// --------------------------------------------------------------------------------------
// Description:
//   Create bounding boxes around planes and group together those who have the same bone index
// --------------------------------------------------------------------------------------
void EvePlaneSet::CreateBoundingBoxes()
{
	m_boundingBoxes.clear();
	m_aabb = CcpMath::AxisAlignedBox();

	std::map<int32_t, CcpMath::AxisAlignedBox> boxes;

	for( auto& item : m_planes )
	{
		if( item->m_color == Color( 0, 0, 0, 0 ) )
		{
			continue;
		}
		auto itemBounds = item->GetBounds();
		auto boneIndex = item->GetBoneIndex();
		if( m_isSkinned && boneIndex >= 0 )
		{
			auto found = boxes.find( boneIndex );
			if( found != end( boxes ) )
			{
				found->second.Include( itemBounds );
			}
			else
			{
				boxes[boneIndex] = CcpMath::AxisAlignedBox( itemBounds );
			}
		}
		else
		{
			m_aabb.Include( itemBounds );
		}
	}
	m_boundingBoxes.insert( end( m_boundingBoxes ), begin( boxes ), end( boxes ) );
}

// --------------------------------------------------------------------------------
// Description:
//   Add a new plane (aka item) to this set
// Arguments:
//   the new plane
// --------------------------------------------------------------------------------
void EvePlaneSet::AddPlaneItem( EvePlaneSetItemPtr item )
{
	m_planes.Insert( -1, item );
}

EvePlaneSetItemVector* EvePlaneSet::GetPlanes()
{
	return &m_planes;
}

void EvePlaneSet::GetDebugOptions( Tr2DebugRendererOptions& options )
{
	options.insert( "Plane Sets" );
	options.insert( "Plane Sets Bounds" );
	options.insert( "Plane Sets Lights" );
}

void EvePlaneSet::RenderDebugInfo( ITr2DebugRenderer2& renderer, const Matrix& parentTransform, const granny_matrix_3x4* bones, size_t boneCount )
{
	if( renderer.HasOption( GetRawRoot(), "Plane Sets" ) )
	{
		for( auto it = m_planes.begin(); it != m_planes.end(); ++it )
		{
			Quaternion rotation( ( *it )->m_rotation );
			Vector3 position( ( *it )->m_position );
			int boneIndex = ( *it )->m_boneIndex;

			if( boneIndex > 0 && boneIndex < int( boneCount ) )
			{
				Matrix boneTF = IdentityMatrix();
				TriMatrixCopyFrom3x4( &boneTF, &bones[boneIndex] );
				position = XMVector3TransformCoord( position, boneTF );

				rotation = XMQuaternionMultiply( rotation, XMQuaternionRotationMatrix( boneTF ) );
			}

			Matrix t( XMMatrixTransformation( Vector3( 0, 0, 0 ), Quaternion( 0, 0, 0, 1 ), ( *it )->m_scaling, Vector3( 0, 0, 0 ), rotation, position ) );
			renderer.DrawBox(
				*it,
				t * parentTransform,
				Vector3( -0.5f, -0.5f, -0.005f ),
				Vector3( 0.5f, 0.5f, 0.005f ),
				Tr2DebugRenderer::Wireframe,
				Tr2DebugColor( Color( 0.0f, 0.7f, 0.9f, 0.5f ), Color( 0.0f, 0.7f, 0.9f, 0.1f ) ) );
			renderer.DrawBox(
				*it,
				t * parentTransform,
				Vector3( -0.5f, -0.5f, -0.005f ),
				Vector3( 0.5f, 0.5f, 0.005f ),
				Tr2DebugRenderer::Solid,
				0 );
		}
	}

	if( renderer.HasOption( GetRawRoot(), "Plane Sets Bounds" ) )
	{
		auto aabb = GetAabb( bones, boneCount );
		renderer.DrawBox(
			Tr2DebugObjectReference( this ),
			parentTransform,
			aabb.m_min,
			aabb.m_max,
			Tr2DebugRenderer::Wireframe,
			0xff00ff00 );
	}

	if( renderer.HasOption( this, "Plane Sets Lights" ) )
	{
		Color averageColor = Color( 0 );
		if( m_lights.size() > 0 )
		{
			averageColor = GetAverageColor();
		}

		for( auto& l : m_lights )
		{
			Matrix t = TranslationMatrix( l.lightData.position ) * l.boneMatrix;

			Color c = l.lightData.color;
			float fade = EveSpaceObjectAttachmentUtils::Fade( l.fadeType, l.blinkRate, l.blinkPhase );

			c = Color( c.r * averageColor.r, c.g * averageColor.g, c.b * averageColor.b, c.a * averageColor.a );
			c = Saturate( c, l.saturation );

			c.a = 0.5f * fade;
			auto planeItem = l.index > m_planes.size() ? nullptr : m_planes[l.index];

			renderer.DrawSphere(
				planeItem,
				t,
				l.lightData.innerRadius,
				10,
				Tr2DebugRenderer::Solid,
				Tr2DebugColor( c ) );

			c.a = 0.3f * fade;
			renderer.DrawSphere(
				planeItem,
				t,
				l.lightData.radius,
				10,
				Tr2DebugRenderer::Solid,
				Tr2DebugColor( c ) );

		}
	}
}

void EvePlaneSet::SetShaderOption( const BlueSharedString& name, const BlueSharedString& value )
{
	if( nullptr != m_effect )
	{
		m_effect->SetOption( name, value );
	}
}

void EvePlaneSet::SetImageMapParameter( TriTextureParameterPtr imageMap )
{
	m_imageMapParameter = imageMap;
}

void EvePlaneSet::SetLayerMap1Parameter( TriTextureParameterPtr layerMap1 )
{
	m_layerMap1Parameter = layerMap1;
}

void EvePlaneSet::SetLayerMap2Parameter( TriTextureParameterPtr layerMap2 )
{
	m_layerMap2Parameter = layerMap2;
}

void EvePlaneSet::SetMaskMapParameter( TriTextureParameterPtr maskMap )
{
	m_maskMapParameter = maskMap;
}

Color EvePlaneSet::GetAverageColor() const
{
	Color layer1, layer2, image, mask;

	layer1 = GetAverageColor( m_layerMap1Parameter );
	layer2 = GetAverageColor( m_layerMap2Parameter );
	image = GetAverageColor( m_imageMapParameter );
	mask = GetAverageColor( m_maskMapParameter );

	return Color( layer1.r * layer2.r * image.r * mask.r, 
					layer1.g * layer2.g * image.g * mask.g,
					layer1.b * layer2.b * image.b * mask.b, 
					layer1.a * layer2.a * image.a * mask.a );
}

Color EvePlaneSet::GetAverageColor( const TriTextureParameterPtr& map ) const
{
	if( nullptr == map || nullptr == map->GetResource() )
	{
		return Color( 1, 1, 1, 1 );
	}

	auto resource = dynamic_cast<TriTextureRes*>( map->GetResource() );
	if( nullptr == resource )
	{
		return Color( 1, 1, 1, 1 );
	}

	return resource->GetAverageColor();
}

void EvePlaneSet::AddLightFromSOF( const EvePlaneLight& light )
{
	m_lights.push_back( light );
}

void EvePlaneSet::RegisterComponents()
{
	auto registry = this->GetComponentRegistry();
	if( registry && !m_lights.empty() )
	{
		registry->RegisterComponent<ITr2LightOwner>( this );
	}
}

void EvePlaneSet::GetLights( Tr2LightManager& lightManager ) const
{
	LightFeatures features = LightFeatures();

	features.parentBrightness = m_activationStrength;
	Color averageColor = Color( 0 );
	if( m_lights.size() > 0 )
	{
		averageColor = GetAverageColor();
	}

	for( auto light : m_lights )
	{
		auto lightDataCopy = light.lightData;
		Color c = light.lightData.color;
		lightDataCopy.color = Color( c.r * averageColor.r, c.g * averageColor.g, c.b * averageColor.b, c.a * averageColor.a );
		lightDataCopy.color = Saturate( lightDataCopy.color, light.saturation );
		features.profileIndex = light.lightProfile == nullptr ? 0 : light.lightProfile->GetTextureIndex();

		float fade = EveSpaceObjectAttachmentUtils::Fade( light.fadeType, light.blinkRate, light.blinkPhase );
		lightDataCopy.brightness *= fade;
		auto perLightData = lightDataCopy.AsPerPointLightData( light.boneMatrix, features, lightManager.GetCurrentSpaceSceneShadowQuality() );
		lightManager.AddLight( perLightData );
	}
}
