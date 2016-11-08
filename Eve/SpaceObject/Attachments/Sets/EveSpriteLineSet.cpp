////////////////////////////////////////////////////////////
//
//    Created:   January 2016
//    Copyright: CCP 2016
//
#include "StdAfx.h"

#include "EveSpriteLineSet.h"

#include "Include/TriMath.h"
#include "Tr2QuadRenderer.h"
#include "Tr2Renderer.h"
#include "Utilities/MatrixUtils.h"
#include "Shader/Tr2Effect.h"

using namespace Tr2RenderContextEnum;

// --------------------------------------------------------------------------------
// Description:
//   Initialize data members
// --------------------------------------------------------------------------------
EveSpriteLineSet::EveSpriteLineSet( IRoot* lockobj ) :
	PARENTLOCK( m_spriteLines ),
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
	PrepareResources();
	return true;
}

// --------------------------------------------------------------------------------
// Description:
//   We have to free all device stuff
// --------------------------------------------------------------------------------
void EveSpriteLineSet::ReleaseResources( TriStorage s )
{
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
	ReleaseResources( 0 );
	PrepareResources();
}

// --------------------------------------------------------------------------------
// Description:
//   (Re)-allocate all device stuff
// --------------------------------------------------------------------------------
bool EveSpriteLineSet::OnPrepareResources()
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

		// need matrix for roation
		Matrix m;
		D3DXMatrixRotationQuaternion( &m, &spriteLine->m_rotation );

		// interpret as circle or line?
		if( spriteLine->m_isCircle )
		{
			// how many sprites on this line?
			size_t numOfSprites = size_t( spriteLine->m_spacing );

			// increase buffer
			totalBufferSize += numOfSprites;
			m_buffer.resize( totalBufferSize );
			m_spriteData.resize( totalBufferSize );

			// start populating the sprites from this circle
			float alpha = 0.f;
			EveSpriteSet::PoolVertex* vtx = &m_buffer[totalBufferidx];
			EveSpriteSet::SpriteData* spr = &m_spriteData[totalBufferidx];
			for( size_t i = 0; i < numOfSprites; ++i )
			{
				// position on an ellipsoid in x,z-plane
				Vector3 pos( spriteLine->m_scaling.x * sinf( alpha ), 0.f, spriteLine->m_scaling.y * cosf( alpha ) );
				D3DXVec3TransformCoord( &pos, &pos, &m );
				pos += spriteLine->m_position;
				// fill static pool data
				vtx->position = pos;
				vtx->warpColor = vtx->color = ( ( spriteLine->m_color & 0xff0000 ) >> 16 ) | ( spriteLine->m_color & 0xff00ff00 ) | ( ( spriteLine->m_color & 0xff ) << 16 );
				vtx->blinkPhase = spriteLine->m_blinkPhase + spriteLine->m_blinkPhaseShift * (float)i;
				vtx->blinkRate = spriteLine->m_blinkRate;
				vtx->minScale = spriteLine->m_minScale;
				vtx->maxScale = spriteLine->m_maxScale;
				vtx->falloff = spriteLine->m_falloff;
				vtx->activation = 1.f;

				// fill animated pool data
				spr->position = pos;
				spr->boneIndex = spriteLine->m_boneIndex;

				// next
				alpha += TRI_2PI / spriteLine->m_spacing;
				++vtx;
				++spr;
			}

			totalBufferidx = totalBufferSize;
		}
		else
		{
			// how many sprites on this line?
			size_t numOfSprites = size_t( spriteLine->m_scaling.x );

			// increase buffer
			totalBufferSize += numOfSprites;
			m_buffer.resize( totalBufferSize );
			m_spriteData.resize( totalBufferSize );

			// start populating the sprites from this line
			Vector3 pos( spriteLine->m_position );
			Vector3 dir( 1.f, 0.f, 0.f );
			D3DXVec3TransformNormal( &dir, &dir, &m );
			EveSpriteSet::PoolVertex* vtx = &m_buffer[totalBufferidx];
			EveSpriteSet::SpriteData* spr = &m_spriteData[totalBufferidx];
			for( size_t i = 0; i < numOfSprites; ++i )
			{
				// fill static pool data
				vtx->position = pos;
				vtx->warpColor = vtx->color = ( ( spriteLine->m_color & 0xff0000 ) >> 16 ) | ( spriteLine->m_color & 0xff00ff00 ) | ( ( spriteLine->m_color & 0xff ) << 16 );
				vtx->blinkPhase = spriteLine->m_blinkPhase + spriteLine->m_blinkPhaseShift * (float)i;
				vtx->blinkRate = spriteLine->m_blinkRate;
				vtx->minScale = spriteLine->m_minScale;
				vtx->maxScale = spriteLine->m_maxScale;
				vtx->falloff = spriteLine->m_falloff;
				vtx->activation = 1.f;

				// fill animated pool data
				spr->position = pos;
				spr->boneIndex = spriteLine->m_boneIndex;

				// next
				pos += spriteLine->m_spacing * dir;
				++vtx;
				++spr;
			}

			totalBufferidx = totalBufferSize;
		}

	}

	return true;
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
// Description:
//   Adds sprites to render with quad renderer if quad rendering was enabled with 
//   UseQuadRenderer call.
// Arguments:
//   quadRenderer - quad renderer
//   world - parent local to world transform
//   activation - parent "activation" state
//   bones - array of bone transforms
//   boneCount - number of bones
// --------------------------------------------------------------------------------
void EveSpriteLineSet::AddToQuadRenderer( Tr2QuadRenderer& quadRenderer, const Matrix& world, float activation, const granny_matrix_3x4* bones, size_t boneCount )
{
	if( !m_display || m_spriteData.empty() )
	{
		return;
	}
	Matrix m = Tr2Renderer::GetIdentityTransform();
	auto n = m_spriteData.size();
	if( !m_skinned )
	{
		XMVector3TransformCoordStream(
			reinterpret_cast<XMFLOAT3*>( &m_buffer[0].position ),
			sizeof( EveSpriteSet::PoolVertex ),
			reinterpret_cast<XMFLOAT3*>( &m_spriteData[0] ),
			sizeof( EveSpriteSet::SpriteData ),
			uint32_t( n ),
			world );
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
				XMStoreFloat3A(
					reinterpret_cast<XMFLOAT3*>( &m_buffer[i].position ),
					XMVector3TransformCoord( position, world ) );
			}
			else
			{
				XMStoreFloat3A(
					reinterpret_cast<XMFLOAT3*>( &m_buffer[i].position ),
					XMVector3TransformCoord( XMLoadFloat3( reinterpret_cast<XMFLOAT3*>( &m_spriteData[i] ) ), world ) );
			}
		}
	}

	quadRenderer.AddQuads( m_effectHash, &m_buffer[0], m_buffer.size() );
}


