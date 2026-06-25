// Copyright © 2023 CCP ehf.

#include "StdAfx.h"
#include "EveStarfield.h"
#include "Shader/Tr2Effect.h"
#include "TriRenderBatch.h"
#include "include/TriMath.h"
#include "Tr2Renderer.h"

using namespace Tr2RenderContextEnum;

struct StarfieldSpriteVertex
{
	Vector3 position;
	float colorIndex;
	float flashIntensity;
	float flashPhase;
	float flashRate;
	uint8_t cornerIndex;
	uint8_t textureIndex;
	uint8_t padding[2];
};

void GenerateStar( float minDist, float maxDist, float minFlashRate, float maxFlashRate, float minIntensity, StarfieldSpriteVertex* star )
{
	float t = TriRand() * 2.0f * 3.1415926535897932384626433832795f;
	float u = ( TriRand() - 0.5f ) * 2.0f;
	float dist = TriRand();
	float radius = Lerp( minDist, maxDist, dist * dist );
	float sq = sqrtf( 1.0f - u * u );

	star->position = Vector3( radius * sq * cosf( t ), radius * sq * sinf( t ), radius * u );
	star->colorIndex = TriRand();
	star->flashIntensity = TriRand() * ( 1 - minIntensity ) + minIntensity;
	star->flashPhase = TriRand();
	star->flashRate = TriRand() * ( maxFlashRate - minFlashRate ) + minFlashRate;
	star->textureIndex = TriRandInt( 4 );
}

EveStarfield::EveStarfield( IRoot* lockobj ) :
	m_bytesPerVertex( 0 ),
	m_vertexCount( 0 ),
	m_starCount( 500 ),
	m_seed( 0 ),
	m_maxDistance( 300 ),
	m_minDistance( 100 ),
	m_maxFlashRate( 1 ),
	m_minFlashRate( 0.5 ),
	m_minFlashIntensity( 0 ),
	m_display( true ),
	m_dirty( false ),
	m_vertexDeclHandle( Tr2EffectStateManager::UNINITIALIZED_DECLARATION )
{
}

EveStarfield::~EveStarfield()
{
}

void EveStarfield::GetBatches( ITriRenderBatchAccumulator* accumulator, const Tr2PerObjectData* perObjectData )
{
	if( !m_vertexBuffer.IsValid() || !m_effect )
	{
		return;
	}

	if( m_vertexDeclHandle == Tr2EffectStateManager::UNINITIALIZED_DECLARATION )
	{
		return;
	}

	if( !m_display )
	{
		return;
	}

	if( m_starCount <= 0 )
	{
		return;
	}

	auto& ib = Tr2Renderer::GetQuadListIndexBuffer();
	if( !ib.IsValid() )
	{
		return;
	}

	Tr2RenderBatch batch;
	batch.SetMaterial( m_effect );
	batch.SetPerObjectData( perObjectData );
	batch.SetVertexDeclaration( m_vertexDeclHandle );
	batch.SetStreamSource( 0, m_vertexBuffer, uint32_t( m_bytesPerVertex ) );
	batch.SetInidices( ib );
	batch.SetDrawIndexedInstanced( m_starCount * 6, 1, ib.GetStartIndex(), 0, 0 );
	accumulator->Commit( batch );
}

bool EveStarfield::Initialize()
{
	PrepareResources();
	return true;
}

void EveStarfield::ReleaseResources( TriStorage s )
{
	m_vertexDeclHandle = Tr2EffectStateManager::UNINITIALIZED_DECLARATION;
	m_vertexBuffer = Tr2BufferAL();
}

bool EveStarfield::OnPrepareResources()
{
	if( m_starCount <= 0 )
	{
		return true;
	}

	// register vertex declaration
	static Tr2VertexDefinition s_spriteVertexDecl;
	if( s_spriteVertexDecl.empty() )
	{
		Tr2VertexDefinition& vd = s_spriteVertexDecl;
		vd.Add( vd.FLOAT32_3, vd.POSITION );
		vd.Add( vd.FLOAT32_1, vd.TEXCOORD, 0 );
		vd.Add( vd.FLOAT32_1, vd.TEXCOORD, 1 );
		vd.Add( vd.FLOAT32_1, vd.TEXCOORD, 2 );
		vd.Add( vd.FLOAT32_1, vd.TEXCOORD, 3 );
		vd.Add( vd.UBYTE_4, vd.TEXCOORD, 4 );
	}

	m_vertexDeclHandle = Tr2EffectStateManager::GetVertexDeclarationHandle( s_spriteVertexDecl );
	if( m_vertexDeclHandle == Tr2EffectStateManager::UNINITIALIZED_DECLARATION )
	{
		return false;
	}

	m_vertexCount = m_starCount * 4;
	m_bytesPerVertex = sizeof( StarfieldSpriteVertex );


	std::vector<StarfieldSpriteVertex> verts( m_vertexCount );

	TriSrand( Be::Time( this->m_seed ) );

	StarfieldSpriteVertex star;
	for( int i = 0; i < m_starCount; ++i )
	{
		GenerateStar( m_minDistance, m_maxDistance, m_minFlashRate, m_maxFlashRate, m_minFlashIntensity, &star );
		for( int j = 0; j < 4; ++j )
		{
			StarfieldSpriteVertex& vertex = verts[i * 4 + j];
			vertex.position = star.position;
			vertex.colorIndex = star.colorIndex;
			vertex.flashIntensity = star.flashIntensity;
			vertex.flashPhase = star.flashPhase;
			vertex.flashRate = star.flashRate;
			vertex.textureIndex = star.textureIndex;
			vertex.cornerIndex = j;
		}
	}

	USE_MAIN_THREAD_RENDER_CONTEXT();
	CR_RETURN_VAL( m_vertexBuffer.Create(
					   m_bytesPerVertex,
					   m_vertexCount,
					   Tr2GpuUsage::VERTEX_BUFFER,
					   Tr2CpuUsage::NONE,
					   &verts[0],
					   renderContext ),
				   false );

	Tr2Renderer::ReserveQuadListIndexBuffer( m_starCount );

	return true;
}

bool EveStarfield::OnModified( Be::Var* val )
{
	m_dirty = true;

	return true;
}

void EveStarfield::Update( Be::Time time )
{
	if( m_dirty )
	{
		ReleaseResources( 0 );
		if( PrepareResources() )
		{
			m_dirty = false;
		}
	}
}
