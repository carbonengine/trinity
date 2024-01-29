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
	boneMatrix( IdentityMatrix() )
{
}

EvePlaneLight::EvePlaneLight( const LightData& lightData, float saturation, uint32_t index, const std::wstring profilePath ) :
	lightData( lightData ),
	saturation( saturation ),
	index( index ),
	boneMatrix( IdentityMatrix() )
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
	for( auto& vertex : m_items )
	{
		// build transformation matrix out of the individual item data
		auto data = m_volatileData[idx++];
		if( m_isSkinned )
		{
			auto boneIndex = vertex.boneIndex;
			if( boneIndex < boneCount )
			{
				TriMatrixCopyFrom3x4( &boneTransform, &bones[boneIndex] );
				data.transform = data.transform * boneTransform;
			}
		}
		data.transform = data.transform * parentTransform;

		vertex.transform1 = Vector4( data.transform._11, data.transform._21, data.transform._31, data.transform._41 );
		vertex.transform2 = Vector4( data.transform._12, data.transform._22, data.transform._32, data.transform._42 );
		vertex.transform3 = Vector4( data.transform._13, data.transform._23, data.transform._33, data.transform._43 );
		vertex.color = data.color * activation;
	}

	quadRenderer.AddQuads( m_effectHash, m_items.data(), m_items.size() );
}

// --------------------------------------------------------------------------------------
// Description:
//   Get bounding box around planes, update visibility based on if box is visible or not
// --------------------------------------------------------------------------------------
bool EvePlaneSet::UpdateVisibility( const TriFrustum& frustum, const Matrix& parentTransform, const granny_matrix_3x4* bones, size_t boneCount )
{
	auto aabb = GetAabb( bones, boneCount );
	if( !aabb.IsInitialized() )
	{
		return false;
	}
	aabb.Transform( parentTransform );

	return frustum.IsBoxVisible( aabb.m_min, aabb.m_max );
}

void EvePlaneSet::UpdateLights( const granny_matrix_3x4* bones, size_t boneCount, float activationStrength, float boosterGain )
{
	for( auto& light : m_lights ) 
	{
		if( light.lightData.boneIndex > 0 && light.lightData.boneIndex < boneCount )
		{
			TriMatrixCopyFrom3x4( &( light.boneMatrix ), &bones[light.lightData.boneIndex] );
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
		for( auto& l : m_lights )
		{
			Matrix t = TranslationMatrix( l.lightData.position ) * l.boneMatrix * parentTransform;

			Color c = Saturate( l.lightData.color, l.saturation );

			if( nullptr != m_primaryTextureParameter ) 
			{
				c = GetAverageColor();
			}

			c.a = 0.5;
			auto planeItem = l.index > m_planes.size() ? nullptr : m_planes[l.index];

			renderer.DrawSphere(
				planeItem,
				t,
				l.lightData.innerRadius,
				10,
				Tr2DebugRenderer::Solid,
				Tr2DebugColor( c ) );

			c.a = 0.3;
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

void EvePlaneSet::SetPrimaryTextureParameter( TriTextureParameterPtr primaryTextureParameter )
{
	m_primaryTextureParameter = primaryTextureParameter;
}

Color EvePlaneSet::GetAverageColor() const
{
	if( nullptr == m_primaryTextureParameter || nullptr == m_primaryTextureParameter->GetResource() )
	{
		return Color( 0, 0, 0, 0 );
	}

	auto resource = dynamic_cast<TriTextureRes*>( m_primaryTextureParameter->GetResource() );
	if( nullptr == resource )
	{
		return Color( 0, 0, 0, 0 );
	}

	return resource->GetAverageColor();
}

void EvePlaneSet::AddLight( const EvePlaneLight& light )
{
	m_lights.push_back( light );
}

void EvePlaneSet::GetLights( Tr2LightManager& lightManager, const Matrix& parentTransform ) const
{
	bool useAverageColor = m_primaryTextureParameter != nullptr;
	LightFeatures features = LightFeatures();

	features.parentBrightness = m_activationStrength;

	if( useAverageColor ) 
	{
		Color aveageColor = GetAverageColor();

		for( auto light : m_lights )
		{
			auto lightDataCopy = light.lightData;
			lightDataCopy.color = Saturate( aveageColor, light.saturation );
			features.profileIndex = light.lightProfile == nullptr ? 0 : light.lightProfile->GetTextureIndex();

			auto perLightData = lightDataCopy.AsPerPointLightData( light.boneMatrix * parentTransform, features );
			lightManager.AddLight( perLightData );
		}
	}
	else
	{
		for( auto& light : m_lights )
		{
			features.profileIndex = light.lightProfile == nullptr ? 0 : light.lightProfile->GetTextureIndex();

			auto perLightData = light.lightData.AsPerPointLightData( light.boneMatrix * parentTransform, features );
			lightManager.AddLight( perLightData );
		}
	}
}
