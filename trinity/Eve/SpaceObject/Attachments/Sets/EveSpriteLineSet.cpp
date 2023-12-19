////////////////////////////////////////////////////////////
//
//    Created:   January 2016
//    Copyright: CCP 2016
//
#include "StdAfx.h"

#include "EveSpriteLineSet.h"

#include "Include/TriMath.h"
#include "Tr2QuadRenderer.h"
#include "Utilities/MatrixUtils.h"
#include "Shader/Tr2Effect.h"
#include "Utilities/BoundingSphere.h"

using namespace Tr2RenderContextEnum;

// --------------------------------------------------------------------------------
// Description:
//   Initialize data members
// --------------------------------------------------------------------------------
EveSpriteLineSet::EveSpriteLineSet( IRoot* lockobj ) :
	PARENTLOCK( m_spriteLines ),
	PARENTLOCK( m_lights ),
	m_display( true ),
	m_skinned( false ),
	m_effectHash( 0 ),
	m_buffer( "EveSpriteLineSet::m_buffer" ),
	m_spriteData( "EveSpriteLineSet::m_spriteData" )
{
}

// --------------------------------------------------------------------------------
// Description:
//   Destructor
// --------------------------------------------------------------------------------
EveSpriteLineSet::~EveSpriteLineSet()
{
}

// --------------------------------------------------------------------------------
// Description:
//   If loading from a .red file, we now can start creating some stuff
// --------------------------------------------------------------------------------
bool EveSpriteLineSet::Initialize()
{
	Rebuild();
	return true;
}


// --------------------------------------------------------------------------------
// Description:
//   Set it up from the outside
// --------------------------------------------------------------------------------
void EveSpriteLineSet::Setup( Tr2EffectPtr effect, bool isSkinned )
{
	m_effect = effect;
	m_skinned = isSkinned;
}

// --------------------------------------------------------------------------------
// Description:
//   Add a line from the outside
// --------------------------------------------------------------------------------
void EveSpriteLineSet::Add( EveSpriteLineSetItemPtr newItem )
{
	m_spriteLines.Append( newItem );
}

// --------------------------------------------------------------------------------
// Description:
//   Rebuild resources after adding/removing/changing individual sprites
// --------------------------------------------------------------------------------
void EveSpriteLineSet::Rebuild()
{
	ReallocateResources();
	CreateItemSetBoundingBoxes( m_aabb, m_boundingBoxes, m_skinned, begin( m_spriteLines ), end( m_spriteLines ) );
}

// --------------------------------------------------------------------------------
// Description:
//   (Re)-allocate vertex and sprite info to buffers
// --------------------------------------------------------------------------------
bool EveSpriteLineSet::ReallocateResources()
{
	// create a hash value for the effect, global quadrenderer needs it!
	if( m_effect )
	{
		m_effectHash = m_effect->GetHashValue();
	}

	// determine total sprite buffer size
	m_buffer.clear();
	m_spriteData.clear();
	size_t totalBufferidx = 0, totalBufferSize = 0;
	for( auto slit = m_spriteLines.begin(); slit != m_spriteLines.end(); ++slit )
	{
		auto spriteLine = *slit;
		auto positions = spriteLine->GetPositions();

		// increase buffer
		totalBufferSize += positions.size();
		m_buffer.resize(totalBufferSize);
		m_spriteData.resize(totalBufferSize);

		EveSpriteSet::PoolVertex* vtx = &m_buffer[totalBufferidx];
		EveSpriteSet::SpriteData* spr = &m_spriteData[totalBufferidx];

		float index = 0.0f;
		for (auto& pos : positions) {
			// fill static pool data
			vtx->position = pos;
			vtx->warpColor = vtx->color = ((spriteLine->m_color & 0xff0000) >> 16) | (spriteLine->m_color & 0xff00ff00) | ((spriteLine->m_color & 0xff) << 16);
			vtx->blinkPhase = Float_16(spriteLine->m_blinkPhase + spriteLine->m_blinkPhaseShift * index);
			vtx->blinkRate = Float_16(spriteLine->m_blinkRate);
			vtx->minScale = Float_16(spriteLine->m_minScale);
			vtx->maxScale = Float_16(spriteLine->m_maxScale);
			vtx->falloff = Float_16(spriteLine->m_falloff);
			vtx->activation = Float_16(1.f);

			// fill animated pool data
			spr->position = pos;
			spr->boneIndex = spriteLine->m_boneIndex;

			++vtx;
			++spr;
			index += 1.0f;
		}
	}

	return true;
}

// --------------------------------------------------------------------------------------
// Description:
//   Get bounding box around sprite lines, update visibility based on if box is visible or not
// --------------------------------------------------------------------------------------
bool EveSpriteLineSet::UpdateVisibility( const TriFrustum& frustum, const Matrix& parentTransform, const granny_matrix_3x4* bones, size_t boneCount )
{
	auto aabb = GetAabb( bones, boneCount );
	if( !aabb.IsInitialized() )
	{
		return false;
	}
	aabb.Transform( parentTransform );

	return frustum.IsBoxVisible( aabb.m_min, aabb.m_max );
}

// --------------------------------------------------------------------------------------
// Description:
//   Get bounding box surrounding sprite lines
// --------------------------------------------------------------------------------------
AxisAlignedBoundingBox EveSpriteLineSet::GetAabb( const granny_matrix_3x4* bones, size_t boneCount ) const
{
	return GetItemSetAabb( m_aabb, m_boundingBoxes, bones, m_skinned ? boneCount : 0 );
}

// --------------------------------------------------------------------------------
// Description:
//   Register this set with the global quad render module
// --------------------------------------------------------------------------------
void EveSpriteLineSet::RegisterWithQuadRenderer( Tr2QuadRenderer& quadRenderer )
{
	quadRenderer.RegisterEffect( m_effectHash, TRIBATCHTYPE_ADDITIVE, sizeof( EveSpriteSet::PoolVertex ), 1, EveSpriteSet::PoolVertex::GetDefinition(), m_effect );
}

// --------------------------------------------------------------------------------
void EveSpriteLineSet::AddToQuadRenderer( Tr2QuadRenderer& quadRenderer, const Matrix& parentTransform, float activation, float, const granny_matrix_3x4* bones, size_t boneCount )
{
	if( !m_display || m_spriteData.empty() )
	{
		return;
	}
	Matrix m = IdentityMatrix();
	auto n = m_spriteData.size();
	if( !m_skinned )
	{
		XMVector3TransformCoordStream(
			reinterpret_cast<XMFLOAT3*>( &m_buffer[0].position ),
			sizeof( EveSpriteSet::PoolVertex ),
			reinterpret_cast<XMFLOAT3*>( &m_spriteData[0] ),
			sizeof( EveSpriteSet::SpriteData ),
			uint32_t( n ),
			parentTransform );
	}
	else
	{
		for( size_t i = 0; i < n; ++i )
		{
			auto boneIndex = m_spriteData[i].boneIndex;
			if( boneIndex < boneCount )
			{
				TriMatrixCopyFrom3x4( &m, &bones[boneIndex] );
				XMVECTOR position = XMVector3TransformCoord( XMLoadFloat3( reinterpret_cast<XMFLOAT3*>( &m_spriteData[i] ) ), m );
				XMStoreFloat3(
					reinterpret_cast<XMFLOAT3*>( &m_buffer[i].position ),
					XMVector3TransformCoord( position, parentTransform ) );
			}
			else
			{
				XMStoreFloat3(
					reinterpret_cast<XMFLOAT3*>( &m_buffer[i].position ),
					XMVector3TransformCoord( XMLoadFloat3( reinterpret_cast<XMFLOAT3*>( &m_spriteData[i] ) ), parentTransform ) );
			}
		}
	}

	Float_16 activation16( activation );
	for( size_t i = 0; i < n; ++i )
	{
		m_buffer[i].activation = activation16;
	}

	quadRenderer.AddQuads( m_effectHash, &m_buffer[0], m_buffer.size() );
}

void EveSpriteLineSet::SetShaderOption( const BlueSharedString& name, const BlueSharedString& value )
{
	if( nullptr != m_effect )
	{
		m_effect->SetOption( name, value );
	}
}

void EveSpriteLineSet::GetDebugOptions( Tr2DebugRendererOptions& options )
{
	options.insert( "Sprite Line Sets" );
	options.insert( "Lights" );
}

void EveSpriteLineSet::RenderDebugInfo( ITr2DebugRenderer2& renderer, const Matrix& parentTransform, const granny_matrix_3x4* bones, size_t boneCount )
{
	if( renderer.HasOption( GetRawRoot(), "Sprite Line Sets" ) )
	{
		Matrix transform = parentTransform;

		for( auto& spriteLine : m_spriteLines ) {
			auto boneIndex = spriteLine->m_boneIndex;

			Tr2DebugColor color( Color( 0.0f, 0.7f, 0.9f, 0.5f ), Color( 0.0f, 0.7f, 0.9f, 0.1f ) );
			if( boneIndex >= 0 )
			{
				if( boneIndex < int( boneCount ) )
				{
					Matrix boneTF = IdentityMatrix();
					TriMatrixCopyFrom3x4( &boneTF, &bones[boneIndex] );
					transform = boneTF * transform;
				}
				else
				{
					color = Tr2DebugColor( Color( 0.9f, 0.2f, 0.2f, 0.5f ), Color( 0.9f, 0.2f, 0.2f, 0.1f ) );
				}
			}

			unsigned index = 0;
			Vector3 lastPos( 0.0, 0.0, 0.0 );
			for( auto& position : spriteLine->GetPositions() ) {
				if( index != 0 ) {
					renderer.DrawCylinder(
						spriteLine,
						transform,
						lastPos,
						position,
						spriteLine->m_minScale / 2.0f,
						8,
						Tr2DebugRenderer::Lit,
						color
					);
				}
				index += 1;
				lastPos = position;

				renderer.DrawSphere(
					spriteLine,
					transform,
					position,
					spriteLine->m_maxScale,
					6,
					Tr2DebugRenderer::Lit,
					color );

			}
		}
	}

	if( renderer.HasOption( this, "Lights" ) )
	{
		for( auto& l : m_lights )
		{
			l->RenderDebugInfo( renderer, parentTransform );
		}
	}
}

void EveSpriteLineSet::AddLight( Tr2Light* light )
{
	m_lights.Append( light->GetRawRoot() );
}

void EveSpriteLineSet::GetLights( Tr2LightManager& lightManager, const Matrix& parentTransform ) const
{
	for( auto& light : m_lights )
	{
		light->AddLight( lightManager, parentTransform, 1.0f );
	}
}

void EveSpriteLineSet::SetLightColorSaturation( float saturation )
{

}
