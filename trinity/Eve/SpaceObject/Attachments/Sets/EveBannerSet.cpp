////////////////////////////////////////////////////////////
//
//    Created:   July 2018
//    Copyright: CCP 2018
//

#include "StdAfx.h"
#include "EveBannerSet.h"
#include "Shader/Tr2Effect.h"
#include "Tr2Renderer.h"
#include "Eve/EveTransform.h"
#include "TriFrustum.h"
#include "Tr2PickingHelperBatch.h"
#include "Utilities/BoundingSphere.h"
#include "Utilities/MatrixUtils.h"
#include "Resources/TriTextureRes.h"
#include "Lights/Tr2Light.h"
#include "Shader/Parameter/TriTextureParameter.h"


namespace
{
	const int32_t PICK_ID_OFFSET = 101;

	BlueStructureDefinition s_bannerStructureDef[] =
	{
		{ "bone", Be::INT32_1, 0 },
		{ "position", Be::FLOAT32_3, 4 },
		{ "rotation", Be::FLOAT32_4, 16 },
		{ "scaling", Be::FLOAT32_3, 32 },
		{ "angleX", Be::FLOAT32_1, 44 },
		{ "angleY", Be::FLOAT32_1, 48 },
		{ 0 }
	};

	EveBannerItem s_defaultBannerItem;

	class RenderBatch : public TriRenderBatch
	{
	public:
		RenderBatch()
			:m_bannerSet( nullptr )
		{
		}

		void SetBannerSet( const EveBannerSet* bannerSet )
		{
			m_bannerSet = bannerSet;
		}

		virtual void SubmitGeometry( Tr2RenderContext& renderContext )
		{
			m_bannerSet->Render( renderContext );
		}
		virtual unsigned int GetPickingData() const
		{
			return m_bannerSet->GetPickingID();
		}
	private:
		const EveBannerSet* m_bannerSet;
	};
}


EveBannerItem::EveBannerItem()
	:bone( -1 ),
	position( 0, 0, 0 ),
	rotation( 0, 0, 0, 1 ),
	scaling( 1, 1, 1 ),
	angleX( 0 ),
	angleY( 0 ),
	reference( 0 )
{
}


struct EveBannerSet::Vertex
{
	Vector3 position;
	Vector4_16 normal;
	Vector2_16 texCoord;

	Vertex()
	{
	}

	Vertex( const Vector3& pos, const Vector3& norm, const Vector2& tc, int32_t bone )
		:position( pos ),
		normal( Vector4( norm, 0 ) ),
		texCoord( tc )
	{
		reinterpret_cast<int8_t*>( &normal.w )[0] = int8_t( bone );
		reinterpret_cast<int8_t*>( &normal.w )[1] = int8_t( bone );
	}
};


EveBannerSet::EveBannerSet( IRoot* lockobj )
	:PARENTLOCK( m_banners ),
	PARENTLOCK( m_lights ),
	m_key( 0 ),
	m_vertexDeclaration( Tr2EffectStateManager::UNINITIALIZED_DECLARATION ),
	m_maxBannerRadius( 0 ),
	m_display( true ),
	m_isPickable( false ),
	m_isVisible( true ),
	m_colorSaturation( 1 )
{
	m_banners.SetStructureDefinition( s_bannerStructureDef );
	m_banners.SetDefaultValue( &s_defaultBannerItem );
	m_banners.SetNotify( this );
	PrepareResources();
}

bool EveBannerSet::Initialize()
{
	Rebuild();
	return true;
}


namespace
{
const std::vector<float> s_fullScreenSize = { 1 };
}

bool EveBannerSet::UpdateVisibility( const TriFrustum& frustum, const Matrix& parentTransform, const granny_matrix_3x4* bones, size_t boneCount )
{
	auto aabb = GetAabb( bones, boneCount );
	if( !aabb.IsInitialized() )
	{
		m_isVisible = false;
		return false;
	}
	aabb.Transform( parentTransform );
	m_isVisible = frustum.IsBoxVisible( aabb.m_min, aabb.m_max );

	bool isLoddedOut = true;

	float screenSize = std::numeric_limits<float>::max();

	Vector4 sphere;
	BoundingSphereFromBox( sphere, aabb.m_min, aabb.m_max );
	if( BoundingSphereIsInside( sphere, frustum.m_viewPos ) )
	{
		isLoddedOut = false;
	}
	else
	{
		extern float g_eveSpaceSceneVisibilityThreshold;

		auto closest = sphere.GetXYZ() + Normalize( frustum.m_viewPos - sphere.GetXYZ() ) * sphere.w;
		Vector4 elementSphere( closest, m_maxBannerRadius );

		screenSize = frustum.GetPixelSizeAccrossEst( &elementSphere );
		if( screenSize > g_eveSpaceSceneVisibilityThreshold * 0.5f )
		{
			isLoddedOut = false;
		}
	}

	if( m_effect )
	{
		m_effect->UsedWithScreenSize( screenSize, m_maxBannerRadius, { 1.f } );
	}

	if( isLoddedOut )
	{
		m_isVisible = false;
	}
	return m_isVisible;
}

void EveBannerSet::GetBatches( ITriRenderBatchAccumulator* batches, TriBatchType batchType, const Tr2PerObjectData* perObjectData, Tr2RenderReason reason )
{
	if( !m_display || !m_vertexBuffer.IsValid() || !m_effect )
	{
		return;
	}
	if( m_primaryTextureParameter && !m_primaryTextureParameter->GetResource() )
	{
		return;
	}
	if( !m_isVisible )
	{
		return;
	}
	if( batchType != TRIBATCHTYPE_ADDITIVE && batchType != TRIBATCHTYPE_PICKING )
	{
		return;
	}
	if( batchType == TRIBATCHTYPE_PICKING && !m_isPickable )
	{
		return;
	}
	if( auto batch = batches->Allocate<RenderBatch>() )
	{
		batch->SetPerObjectData( perObjectData );
		batch->SetShaderMaterial( m_effect );
		batch->SetBannerSet( this );
		if( batchType == TRIBATCHTYPE_ADDITIVE )
		{
			batch->SetRenderingMode( Tr2EffectStateManager::RM_ALPHA_ADDITIVE );
		}
		batches->Commit( batch );
	}
}

void EveBannerSet::GetDebugOptions( Tr2DebugRendererOptions& options )
{
	options.insert( "Banners" );
	options.insert( "Banner Bounds" );
}

void EveBannerSet::RenderDebugInfo( ITr2DebugRenderer2& renderer, const Matrix& parentTransform, const granny_matrix_3x4* bones, size_t boneCount )
{
	if( renderer.HasOption( GetRawRoot(), "Banners" ) )
	{
		uint32_t index = 0;
		for( auto it = m_banners.begin(); it != m_banners.end(); ++it, ++index )
		{
			Matrix t = TransformationMatrix( it->scaling, it->rotation, it->position );
			Color color( 0.1f, 0.1f, 0.7f, 0.5f );
			if( it->bone >= 0 )
			{
				if( size_t( it->bone ) < boneCount )
				{
					Matrix boneTF = IdentityMatrix();
					TriMatrixCopyFrom3x4( &boneTF, &bones[it->bone] );
					t *= boneTF;
				}
				else
				{
					color = Color( 0.7f, 0.1f, 0.1f, 0.5f );
				}
			}
			t *= parentTransform;

			renderer.DrawBox(
				Tr2DebugObjectReference( m_banners.GetRawRoot(), index ),
				t,
				Vector3( -0.5f, -0.5f, -0.005f ),
				Vector3( 0.5f, 0.5f, 0.005f ),
				Tr2DebugRenderer::Wireframe,
				Tr2DebugColor( color ) );
			renderer.DrawBox(
				Tr2DebugObjectReference( m_banners.GetRawRoot(), index ),
				t,
				Vector3( -0.5f, -0.5f, -0.005f ),
				Vector3( 0.5f, 0.5f, 0.005f ),
				Tr2DebugRenderer::Solid,
				0 );
		}
	}
	if( renderer.HasOption( GetRawRoot(), "Banner Bounds" ) )
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
	if( renderer.HasOption( this, "Lights" ) )
	{
		for( auto l = begin( m_lights ); l != end( m_lights ); ++l )
		{
			( *l )->RenderDebugInfo( renderer, parentTransform );
		}
	}
}

void EveBannerSet::SetShaderOption( const BlueSharedString& name, const BlueSharedString& value )
{
	if( nullptr != m_effect )
	{
		m_effect->SetOption( name, value );
	}
}

void EveBannerSet::AddBanner( const EveBannerItem& banner )
{
	m_banners.Append( &banner );
}

void EveBannerSet::SetEffect( Tr2Effect* effect )
{
	m_effect = effect;
}

void EveBannerSet::SetKey( int32_t key )
{
	m_key = key;
}

void EveBannerSet::AddLight( Tr2Light* light )
{
	m_lights.Append( light->GetRawRoot() );
}

void EveBannerSet::SetPrimaryTextureParameter( TriTextureParameter* primaryTextureParameter )
{
	m_primaryTextureParameter = primaryTextureParameter;
}

void EveBannerSet::Render( Tr2RenderContext& renderContext ) const
{
	renderContext.m_esm.ApplyStreamSource( 0, m_vertexBuffer, 0, sizeof( Vertex ) );
	renderContext.m_esm.ApplyIndexBuffer( m_indexBuffer );
	renderContext.m_esm.ApplyVertexDeclaration( m_vertexDeclaration );

	renderContext.DrawIndexedPrimitive( m_vertexBuffer.GetSize(), 0, m_indexBuffer.GetDesc().count / 3 );
}

unsigned int EveBannerSet::GetPickingID() const
{
	return unsigned( PICK_ID_OFFSET + m_key );
}

int32_t EveBannerSet::GetReference( size_t index ) const
{
	return m_banners[index].reference;
}

void EveBannerSet::OnStructureListModified( Event, const void*, size_t, IBlueStructureList* )
{
	Rebuild();
}

void EveBannerSet::ReleaseResources( TriStorage )
{
}

bool EveBannerSet::OnPrepareResources()
{
	USE_MAIN_THREAD_RENDER_CONTEXT();

	Tr2VertexDefinition def;
	def.Add( Tr2VertexDefinition::FLOAT32_3, Tr2VertexDefinition::POSITION );
	def.Add( Tr2VertexDefinition::FLOAT16_4, Tr2VertexDefinition::NORMAL );
	def.Add( Tr2VertexDefinition::FLOAT16_2, Tr2VertexDefinition::TEXCOORD );

	Tr2VertexDefinition::Item item;
	item.m_dataType = Tr2VertexDefinition::BYTE_4;
	item.m_offset = 4 * 3 + 2 * 2;
	item.m_stream = 0;
	item.m_usage = Tr2VertexDefinition::BLENDINDICES;
	item.m_usageIndex = 0;
	item.m_instanceStepRate = 0;
	def.m_items.push_back( item );

	m_vertexDeclaration = renderContext.m_esm.GetVertexDeclarationHandle( def );
	if( !m_vertexBuffer.IsValid() || !m_indexBuffer.IsValid() )
	{
		Rebuild();
	}
	return true;
}

AxisAlignedBoundingBox EveBannerSet::GetAabb( const granny_matrix_3x4* bones, size_t boneCount ) const
{
	return GetItemSetAabb( m_aabb, m_skinnedBoxes, bones, boneCount );
}

void EveBannerSet::Rebuild()
{
	m_aabb = AxisAlignedBoundingBox();
	m_vertexBuffer = Tr2BufferAL();
	m_indexBuffer = Tr2BufferAL();
	m_maxBannerRadius = 0;
	m_skinnedBoxes.clear();

	if( !m_effect )
	{
		return;
	}

	std::map<int32_t, CcpMath::AxisAlignedBox> boxes;
	std::vector<Vertex> vertices;
	std::vector<uint16_t> indices;

	for( auto jt = m_banners.begin(); jt != m_banners.end(); ++jt )
	{
		CreateBannerGeometry( vertices, indices, *jt );
		AxisAlignedBoundingBox aabb( Vector3( -0.5f, -0.5f, -0.5f ), Vector3( 0.5f, 0.5f, 0.0f ) );
		aabb.Transform( TransformationMatrix( jt->scaling, jt->rotation, jt->position ) );

		m_maxBannerRadius = std::max( m_maxBannerRadius, Length( aabb.m_max - aabb.m_min ) / 2 );

		if( jt->bone >= 0 )
		{
			boxes[jt->bone].Include( aabb );
		}
		else
		{
			m_aabb.IncludeBox( aabb );
		}
	}
	m_skinnedBoxes.insert( end( m_skinnedBoxes ), begin( boxes ), end( boxes ) );

	if( !vertices.empty() )
	{
		USE_MAIN_THREAD_RENDER_CONTEXT();
		m_vertexBuffer.Create( sizeof( Vertex), uint32_t( vertices.size() ), Tr2GpuUsage::VERTEX_BUFFER, Tr2CpuUsage::NONE, &vertices[0], renderContext );
		m_indexBuffer.Create( sizeof( uint16_t ), uint32_t( indices.size() ), Tr2GpuUsage::INDEX_BUFFER, Tr2CpuUsage::NONE, &indices[0], renderContext );
	}
}

Color EveBannerSet::GetSaturatedLightColor() const
{
	if( !m_primaryTextureParameter )
	{
		return Color( 0.0, 0.0, 0.0, 0.0 );
	}

	TriTextureRes* resource = dynamic_cast<TriTextureRes*>( m_primaryTextureParameter->GetResource() );

	if( !resource )
	{
		return Color( 0.0, 0.0, 0.0, 0.0 );
	}

	Color c = resource->GetAverageColor();

	Color c2 = Color( c.r * c.a, c.g * c.a, c.b * c.a, c.a );
	
	if( m_colorSaturation == 1.f )
	{
		return c2;
	} 

	// intencity (the magic numbers are values based on how strongly our eyes perceive each color)
	float i = ( c2.r * 0.299f ) + ( c2.g * 0.587f ) + (c2.b * 0.114f);

	Color finalC = Lerp( Color(i,i,i,i), c2, max( 0.0f, m_colorSaturation ) );

	return  finalC;
}

void EveBannerSet::SetLightColorSaturation(float saturation )
{
	m_colorSaturation = saturation;
}

void EveBannerSet::GetLights( Tr2LightManager& lightManager, const Matrix& parentTransform ) const
{
	if( !m_display )
	{
		return;
	}

	Color c = GetSaturatedLightColor();

	if( c.a == 0.0f )
	{
		return;
	}

	for( auto b = m_lights.begin(); b != m_lights.end(); ++b )
	{
		( *b )->ChangeLightColor( c );
		( *b )->AddLight( lightManager, parentTransform, 1.0f );
	}
}

void EveBannerSet::CreateBannerGeometry( std::vector<Vertex>& vertexBuffer, std::vector<uint16_t>& indexBuffer, const EveBannerItem& item ) const
{
	bool flatX = item.angleX <= 0;
	bool flatY = item.angleY <= 0;
	if( flatX && flatY )
	{
		CreateFlatBannerGeometry( vertexBuffer, indexBuffer, item );
	}
	else if( flatX )
	{
		CreateVerticalCurvedBannerGeometry( vertexBuffer, indexBuffer, item );
	}
	else if( flatY )
	{
		CreateHorizontalCurvedBannerGeometry( vertexBuffer, indexBuffer, item );
	}
	else
	{
		CreateCurvedBannerGeometry( vertexBuffer, indexBuffer, item );
	}
}

void EveBannerSet::CreateFlatBannerGeometry( std::vector<Vertex>& vertexBuffer, std::vector<uint16_t>& indexBuffer, const EveBannerItem& item ) const
{
	uint16_t startIndex = uint16_t( vertexBuffer.size() );
	auto transform = TransformationMatrix( item.scaling, item.rotation, item.position );

	auto normal = Normalize( TransformNormal( Vector3( 0, 0, 1 ), transform ) );
	vertexBuffer.push_back( Vertex( TransformCoord( Vector3( -0.5f, -0.5, 0 ), transform ), normal, Vector2( 0, 1 ), item.bone ) );
	vertexBuffer.push_back( Vertex( TransformCoord( Vector3( -0.5f, 0.5, 0 ), transform ), normal, Vector2( 0, 0 ), item.bone ) );
	vertexBuffer.push_back( Vertex( TransformCoord( Vector3( 0.5f, -0.5, 0 ), transform ), normal, Vector2( 1, 1 ), item.bone ) );
	vertexBuffer.push_back( Vertex( TransformCoord( Vector3( 0.5f, 0.5, 0 ), transform ), normal, Vector2( 1, 0 ), item.bone ) );


	indexBuffer.push_back( startIndex + 0 );
	indexBuffer.push_back( startIndex + 1 );
	indexBuffer.push_back( startIndex + 2 );
	indexBuffer.push_back( startIndex + 2 );
	indexBuffer.push_back( startIndex + 1 );
	indexBuffer.push_back( startIndex + 3 );
}

void EveBannerSet::CreateVerticalCurvedBannerGeometry( std::vector<Vertex>& vertexBuffer, std::vector<uint16_t>& indexBuffer, const EveBannerItem& item ) const
{
	uint16_t startIndex = uint16_t( vertexBuffer.size() );
	auto transform = TransformationMatrix( item.scaling, item.rotation, item.position );
	auto normalTransform = Inverse( Transpose( transform ) );

	float clamppedAngleY = std::max( 0.f, std::min( item.angleY, 180.f ) );

	uint32_t segmentsX = 1;
	uint32_t segmentsY = 1 + uint32_t( clamppedAngleY / 5 );

	auto halfAngleY = clamppedAngleY / 180.f * XM_PI / 2;

	float scaleY = 0.5f / sin( halfAngleY );

	for( uint32_t j = 0; j <= segmentsY; ++j )
	{
		float y = float( j ) / ( segmentsY );
		float angleY = -halfAngleY + y * 2 * halfAngleY;
		float sinAngleY = sin( angleY + XM_PIDIV2 );
		float cosAngleY = cos( angleY + XM_PIDIV2 );
		for( uint32_t i = 0; i <= segmentsX; ++i )
		{
			float x = float( i );
			float sinAngleX = i == 0 ? -0.5f : 0.5f;
			float cosAngleX = 1;

			Vector3 normal = Vector3( 0, cosAngleY / scaleY, sinAngleY / scaleY );

			vertexBuffer.push_back( Vertex(
				TransformCoord( Vector3( sinAngleX, cosAngleY * scaleY, ( cosAngleX * sinAngleY - 1 ) * scaleY ), transform ),
				Normalize( TransformNormal( normal, normalTransform ) ),
				Vector2( x, y ),
				item.bone ) );
		}
	}

	auto vertexIndex = [=]( uint32_t x, uint32_t y ) { return x + 2 * y; };

	for( uint32_t j = 0; j < segmentsY; ++j )
	{
		indexBuffer.push_back( startIndex + vertexIndex( 0, j ) );
		indexBuffer.push_back( startIndex + vertexIndex( 1, j ) );
		indexBuffer.push_back( startIndex + vertexIndex( 0, j + 1 ) );
		indexBuffer.push_back( startIndex + vertexIndex( 0, j + 1 ) );
		indexBuffer.push_back( startIndex + vertexIndex( 1, j ) );
		indexBuffer.push_back( startIndex + vertexIndex( 1, j + 1 ) );
	}
}

void EveBannerSet::CreateHorizontalCurvedBannerGeometry( std::vector<Vertex>& vertexBuffer, std::vector<uint16_t>& indexBuffer, const EveBannerItem& item ) const
{
	uint16_t startIndex = uint16_t( vertexBuffer.size() );
	auto transform = TransformationMatrix( item.scaling, item.rotation, item.position );
	auto normalTransform = Inverse( Transpose( transform ) );

	float clamppedAngleX = std::max( 0.f, std::min( item.angleX, 180.f ) );

	uint32_t segmentsX = 1 + uint32_t( clamppedAngleX / 5 );
	uint32_t segmentsY = 1;

	auto halfAngleX = clamppedAngleX / 180.f * XM_PI / 2;

	float scaleX = 0.5f / sin( halfAngleX );

	for( uint32_t j = 0; j <= segmentsY; ++j )
	{
		float y = float( j ) / ( segmentsY );
		float sinAngleY = 1;
		float cosAngleY = j == 0 ? 0.5f : -0.5f;

		for( uint32_t i = 0; i <= segmentsX; ++i )
		{
			float x = float( i ) / ( segmentsX );
			float angleX = -halfAngleX + x * 2 * halfAngleX;
			float sinAngleX = sin( angleX );
			float cosAngleX = cos( angleX );

			Vector3 normal = Vector3( sinAngleX / scaleX, 0, cosAngleX / scaleX );

			vertexBuffer.push_back( Vertex(
				TransformCoord( Vector3( sinAngleX * sinAngleY * scaleX, cosAngleY, ( cosAngleX * sinAngleY - 1 ) * scaleX ), transform ),
				Normalize( TransformNormal( normal, normalTransform ) ),
				Vector2( x, y ),
				item.bone ) );
		}
	}

	auto vertexIndex = [=]( uint32_t x, uint32_t y ) { return x + ( segmentsX + 1 ) * y; };

	for( uint32_t i = 0; i < segmentsX; ++i )
	{
		indexBuffer.push_back( startIndex + vertexIndex( i, 0 ) );
		indexBuffer.push_back( startIndex + vertexIndex( i + 1, 0 ) );
		indexBuffer.push_back( startIndex + vertexIndex( i, 1 ) );
		indexBuffer.push_back( startIndex + vertexIndex( i, 1 ) );
		indexBuffer.push_back( startIndex + vertexIndex( i + 1, 0 ) );
		indexBuffer.push_back( startIndex + vertexIndex( i + 1, 1 ) );
	}
}

void EveBannerSet::CreateCurvedBannerGeometry( std::vector<Vertex>& vertexBuffer, std::vector<uint16_t>& indexBuffer, const EveBannerItem& item ) const
{
	uint16_t startIndex = uint16_t( vertexBuffer.size() );
	auto transform = TransformationMatrix( item.scaling, item.rotation, item.position );
	auto normalTransform = Inverse( Transpose( transform ) );

	float clamppedAngleX = std::max( 0.f, std::min( item.angleX, 180.f ) );
	float clamppedAngleY = std::max( 0.f, std::min( item.angleY, 180.f ) );

	uint32_t segmentsX = 1 + uint32_t( clamppedAngleX / 5 );
	uint32_t segmentsY = 1 + uint32_t( clamppedAngleY / 5 );

	auto halfAngleX = clamppedAngleX / 180.f * XM_PI / 2;
	auto halfAngleY = clamppedAngleY / 180.f * XM_PI / 2;

	float scaleX = 0.5f / sin( halfAngleX );
	float scaleY = 0.5f / sin( halfAngleY );
	float scaleZ = std::min( scaleX, scaleY );

	for( uint32_t j = 0; j <= segmentsY; ++j )
	{
		float y = float( j ) / ( segmentsY );
		float angleY = -halfAngleY + y * 2 * halfAngleY;
		float sinAngleY = sin( angleY + XM_PIDIV2 );
		float cosAngleY = cos( angleY + XM_PIDIV2 );

		for( uint32_t i = 0; i <= segmentsX; ++i )
		{
			float x = float( i ) / ( segmentsX );
			float angleX = -halfAngleX + x * 2 * halfAngleX;
			float sinAngleX = sin( angleX );
			float cosAngleX = cos( angleX );

			Vector3 normal = Vector3( sinAngleX * sinAngleY / scaleX, cosAngleY / scaleY, cosAngleX * sinAngleY / scaleZ );

			vertexBuffer.push_back( Vertex(
				TransformCoord( Vector3( sinAngleX * sinAngleY * scaleX, cosAngleY * scaleY, ( cosAngleX * sinAngleY - 1 ) * scaleZ ), transform ),
				Normalize( TransformNormal( normal, normalTransform ) ),
				Vector2( x, y ),
				item.bone ) );
		}
	}

	auto vertexIndex = [=]( uint32_t x, uint32_t y ) { return x + ( segmentsX + 1 ) * y; };

	for( uint32_t j = 0; j < segmentsY; ++j )
	{
		for( uint32_t i = 0; i < segmentsX; ++i )
		{
			indexBuffer.push_back( startIndex + vertexIndex( i, j ) );
			indexBuffer.push_back( startIndex + vertexIndex( i + 1, j ) );
			indexBuffer.push_back( startIndex + vertexIndex( i, j + 1 ) );
			indexBuffer.push_back( startIndex + vertexIndex( i, j + 1 ) );
			indexBuffer.push_back( startIndex + vertexIndex( i + 1, j ) );
			indexBuffer.push_back( startIndex + vertexIndex( i + 1, j + 1 ) );
		}
	}
}

namespace
{
	float GetVerticalCurvedBannerApectRatio( const EveBannerItem& banner )
	{
		auto transform = TransformationMatrix( banner.scaling, banner.rotation, banner.position );
		auto normalTransform = Inverse( Transpose( transform ) );

		float clamppedAngleY = std::max( 0.f, std::min( banner.angleY, 180.f ) );

		uint32_t segmentsY = 1 + uint32_t( clamppedAngleY / 5 );

		auto halfAngleY = clamppedAngleY / 180.f * XM_PI / 2;

		float scaleY = 0.5f / sin( halfAngleY );

		Vector3 prevPos;
		float uLength = banner.scaling.x;
		float vLength = 0;

		for( uint32_t j = 0; j <= segmentsY; ++j )
		{
			float y = float( j ) / ( segmentsY );
			float angleY = -halfAngleY + y * 2 * halfAngleY;
			float sinAngleY = sin( angleY + XM_PIDIV2 );
			float cosAngleY = cos( angleY + XM_PIDIV2 );

			Vector3 pos = TransformCoord( Vector3( 0, cosAngleY * scaleY, ( sinAngleY - 1 ) * scaleY ), transform );
			if( j )
			{
				vLength += Length( pos - prevPos );
			}
			prevPos = pos;
		}

		return uLength / vLength;
	}

	float GetHorizontalCurvedBannerApectRatio( const EveBannerItem& banner )
	{
		auto transform = TransformationMatrix( banner.scaling, banner.rotation, banner.position );
		auto normalTransform = Inverse( Transpose( transform ) );

		float clamppedAngleX = std::max( 0.f, std::min( banner.angleX, 180.f ) );

		uint32_t segmentsX = 1 + uint32_t( clamppedAngleX / 5 );

		auto halfAngleX = clamppedAngleX / 180.f * XM_PI / 2;

		float scaleX = 0.5f / sin( halfAngleX );

		Vector3 prevPos;
		float uLength = 0;
		float vLength = banner.scaling.y;

		for( uint32_t i = 0; i <= segmentsX; ++i )
		{
			float x = float( i ) / ( segmentsX );
			float angleX = -halfAngleX + x * 2 * halfAngleX;
			float sinAngleX = sin( angleX );
			float cosAngleX = cos( angleX );

			Vector3 pos = TransformCoord( Vector3( sinAngleX * scaleX, 0, ( cosAngleX - 1 ) * scaleX ), transform );
			if( i )
			{
				uLength += Length( pos - prevPos );
			}
			prevPos = pos;
		}
		return uLength / vLength;
	}

	float GetCurvedBannerApectRatio( const EveBannerItem& banner )
	{
		auto transform = TransformationMatrix( banner.scaling, banner.rotation, banner.position );
		auto normalTransform = Inverse( Transpose( transform ) );

		float clamppedAngleX = std::max( 0.f, std::min( banner.angleX, 180.f ) );
		float clamppedAngleY = std::max( 0.f, std::min( banner.angleY, 180.f ) );

		uint32_t segmentsX = 1 + uint32_t( clamppedAngleX / 5 );
		uint32_t segmentsY = 1 + uint32_t( clamppedAngleY / 5 );

		auto halfAngleX = clamppedAngleX / 180.f * XM_PI / 2;
		auto halfAngleY = clamppedAngleY / 180.f * XM_PI / 2;

		float scaleX = 0.5f / sin( halfAngleX );
		float scaleY = 0.5f / sin( halfAngleY );
		float scaleZ = std::min( scaleX, scaleY );

		Vector3 prevPos;
		float uLength = 0;
		float vLength = 0;

		for( uint32_t i = 0; i <= segmentsX; ++i )
		{
			float x = float( i ) / ( segmentsX );
			float angleX = -halfAngleX + x * 2 * halfAngleX;
			float sinAngleX = sin( angleX );
			float cosAngleX = cos( angleX );

			Vector3 pos = TransformCoord( Vector3( sinAngleX * scaleX, 0, ( cosAngleX - 1 ) * scaleZ ), transform );
			if( i )
			{
				uLength += Length( prevPos - pos );
			}
			prevPos = pos;
		}

		for( uint32_t j = 0; j <= segmentsY; ++j )
		{
			float y = float( j ) / ( segmentsY );
			float angleY = -halfAngleY + y * 2 * halfAngleY;
			float sinAngleY = sin( angleY + XM_PIDIV2 );
			float cosAngleY = cos( angleY + XM_PIDIV2 );

			Vector3 pos = TransformCoord( Vector3( 0, cosAngleY * scaleY, ( sinAngleY - 1 ) * scaleZ ), transform );
			if( j )
			{
				vLength += Length( prevPos - pos );
			}
			prevPos = pos;
		}
		return uLength / vLength;
	}
}

float EveBannerSet::GetBannerAspectRatio( const EveBannerItem& banner )
{
	bool flatX = banner.angleX <= 0;
	bool flatY = banner.angleY <= 0;
	if( flatX && flatY )
	{
		return banner.scaling.x / banner.scaling.y;
	}
	else if( flatX )
	{
		return GetVerticalCurvedBannerApectRatio( banner );
	}
	else if( flatY )
	{
		return GetHorizontalCurvedBannerApectRatio( banner );
	}
	else
	{
		return GetCurvedBannerApectRatio( banner );
	}
}