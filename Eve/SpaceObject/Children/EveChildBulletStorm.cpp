////////////////////////////////////////////////////////////
//
//    Created:   December 2015
//    Copyright: CCP 2015
//

#include "StdAfx.h"
#include "EveChildBulletStorm.h"

#include "Eve/EveUpdateContext.h"
#include "TriRenderBatch.h"
#include "Shader/Tr2Effect.h"
#include "Include/TriMath.h"

// vertex layout struct
struct BulletStormVertex
{
	uint32_t cornerID;
};

struct PerInstanceVertex
{
	Vector3 sourcePositionOS;
	Vector3 sourceDirectionOS;
	Vector4 data;
};

// --------------------------------------------------------------------------------
// Description:
//   Initialize data members
// --------------------------------------------------------------------------------
EveChildBulletStorm::EveChildBulletStorm( IRoot* lockobj ) :
	PARENTLOCK( m_targetObjects ),
	m_display( true ),
	m_objectCount( 0 ),
	m_multiplier( 1 ),
	m_vertexDeclHandle( Tr2EffectStateManager::UNINITIALIZED_DECLARATION )
{
	PrepareResources();
}

// --------------------------------------------------------------------------------
// Description:
//   Byebye
// --------------------------------------------------------------------------------
EveChildBulletStorm::~EveChildBulletStorm()
{
}

// --------------------------------------------------------------------------------
// Description:
//   If loading from a .red file, we now can start creating resources
// --------------------------------------------------------------------------------
bool EveChildBulletStorm::Initialize()
{
	PrepareResources();
	return true;
}

// --------------------------------------------------------------------------------
// Description:
//   If someone changed some data we must re-create buffers etc.
// --------------------------------------------------------------------------------
bool EveChildBulletStorm::OnModified( Be::Var* val )
{
	if( IsMatch( val, m_multiplier ) || IsMatch( val, m_sourceObject ) )
	{
		Rebuild();
	}

	return true;
}

// --------------------------------------------------------------------------------
// Description:
//   Trinity's way of rendering
// --------------------------------------------------------------------------------
void EveChildBulletStorm::GetRenderables( const TriFrustum& frustum, std::vector<ITr2Renderable*>& renderables, const Matrix& parentTransform )
{
	if( !m_display )
	{
		return;
	}
	renderables.push_back( this );
}

// --------------------------------------------------------------------------------
// Description:
//   We have to free all device stuff, so release vertex declaration and free
//   all the vertex buffer
// --------------------------------------------------------------------------------
void EveChildBulletStorm::ReleaseResources( TriStorage s )
{
	m_vertexDeclHandle = Tr2EffectStateManager::UNINITIALIZED_DECLARATION;
	m_vertexBuffer.Destroy();
	m_perInstanceBuffer.Destroy();
}

// --------------------------------------------------------------------------------
// Description:
//   (Re)-allocate all device stuff: create a vertex declaration for the instanced
//   rendering and a vertexbuffer
// --------------------------------------------------------------------------------
bool EveChildBulletStorm::OnPrepareResources()
{
	// register vertex declaration
	static Tr2VertexDefinition s_spriteVertexDecl;
	if( s_spriteVertexDecl.empty() )
	{
		Tr2VertexDefinition& vd = s_spriteVertexDecl;
		vd.Add( vd.UINT32_1, vd.TEXCOORD, 0, 0 );
		vd.Add( vd.FLOAT32_3, vd.TEXCOORD, 1, 1, 1 );
		vd.Add( vd.FLOAT32_3, vd.TEXCOORD, 2, 1, 1 );
		vd.Add( vd.FLOAT32_4, vd.TEXCOORD, 3, 1, 1 );
	}
	m_vertexDeclHandle = Tr2EffectStateManager::GetVertexDeclarationHandle( s_spriteVertexDecl );
	if( m_vertexDeclHandle == Tr2EffectStateManager::UNINITIALIZED_DECLARATION )
	{
		return false;
	}

	// the small vertexbuffer can be build right here
	if( !m_vertexBuffer.IsValid() )
	{
		// prepare buffers
		BulletStormVertex verts[4];
		verts[0].cornerID = 0;
		verts[1].cornerID = 1;
		verts[2].cornerID = 2;
		verts[3].cornerID = 3;

		// crate vertexbuffer and init it
		USE_MAIN_THREAD_RENDER_CONTEXT();
		CR_RETURN_VAL( m_vertexBuffer.Create( 4 * sizeof( BulletStormVertex ), Tr2RenderContextEnum::USAGE_IMMUTABLE, &verts[0], renderContext ), false );
	}

	// now build the pre-instance buffer
	Rebuild();

	return true;
}

// --------------------------------------------------------------------------------
// Description:
//   Rebuild the internal buffers, mainly the per-instance buffer when we get a new
//   source object aka shooter
// --------------------------------------------------------------------------------
void EveChildBulletStorm::Rebuild()
{
	// invalidate buffer
	m_perInstanceBuffer.Destroy();

	// if we don't have a shooter, that's it: exit
	if( !m_sourceObject )
	{
		return;
	}

	// re-build the per-instance buffer here
	m_objectCount = m_multiplier * m_sourceObject->GetDamageLocatorCount();
	if( m_objectCount > 0 )
	{
		// put all locators from parent into mem
		std::vector<PerInstanceVertex> verts( m_objectCount );
		PerInstanceVertex* v = &verts[0];
		for( unsigned int i = 0; i < m_sourceObject->GetDamageLocatorCount(); ++i )
		{
			// per stream start
			Vector3 startPosOS, startDirOS;
			m_sourceObject->GetDamageLocatorPosition( &startPosOS, i, false );
			m_sourceObject->GetDamageLocatorDirection( &startDirOS, i, false );
			// per stream rnds
			float rndf = TriRand();
			for( unsigned int j = 0; j < m_multiplier; ++j )
			{
				Vector3 rndv( TriRand(), TriRand(), TriRand() );
				v->sourcePositionOS = startPosOS;
				v->sourceDirectionOS = startDirOS;
				v->data = Vector4( rndv, ( (float)j + rndf ) / (float)( m_multiplier ) );
				++v;
			}
		}
		// crate vertexbuffer and init it
		USE_MAIN_THREAD_RENDER_CONTEXT();
		CR_RETURN( m_perInstanceBuffer.Create( m_objectCount * sizeof( PerInstanceVertex ), Tr2RenderContextEnum::USAGE_IMMUTABLE, &verts[0], renderContext ) );
	}
}

// --------------------------------------------------------------------------------
// Description:
//   Setup instanced reandering and call DIP
// --------------------------------------------------------------------------------
void EveChildBulletStorm::SubmitGeometry( Tr2RenderContext& renderContext )
{
	renderContext.m_esm.ApplyVertexDeclaration( m_vertexDeclHandle );
	renderContext.m_esm.ApplyStreamSource( 0, m_vertexBuffer, 0, sizeof( BulletStormVertex ) );
	renderContext.m_esm.ApplyStreamSource( 1, m_perInstanceBuffer, 0, sizeof( PerInstanceVertex ) );
	auto ib = Tr2Renderer::GetQuadListIndexBuffer( 2 );
	if( !ib )
	{
		return;
	}
	renderContext.m_esm.ApplyIndexBuffer( *ib );
	renderContext.SetTopology( Tr2RenderContextEnum::TOP_TRIANGLES );
	renderContext.DrawIndexedInstanced( 4, 0, 2, m_objectCount );
}

// --------------------------------------------------------------------------------
// Description:
//   This object might have a huge bounding sphere
// --------------------------------------------------------------------------------
bool EveChildBulletStorm::GetBoundingSphere( Vector4& sphere, BoundingSphereQuery query ) const
{
	return true;
}

// --------------------------------------------------------------------------------
// Description:
//   Syncronous updates happen here
// --------------------------------------------------------------------------------
void EveChildBulletStorm::UpdateSyncronous( EveUpdateContext& updateContext, IEveSpaceObject2* spaceObjectParent, IEveSpaceObjectChild* childParent )
{
	// combining target blobs from the actuall spaceship targets
	m_targetBlobs.resize( m_targetObjects.size() );
	for( size_t i = 0; i < m_targetObjects.size(); ++i )
	{
		m_targetObjects[i]->GetValueAt( (Vector3*)&m_targetBlobs[i], updateContext.GetTime() );
		m_targetBlobs[i].w = 1.f;
	}
}

// --------------------------------------------------------------------------------
// Description:
//   Assyncronous updates happen here
// --------------------------------------------------------------------------------
void EveChildBulletStorm::UpdateAsyncronous( EveUpdateContext& updateContext, IEveSpaceObject2* spaceObjectParent, IEveSpaceObjectChild* childParent )
{
	if( spaceObjectParent )
	{
		spaceObjectParent->GetLocalToWorldTransform( m_worldTransform );
	}
}

// --------------------------------------------------------------------------------
// Description:
//   Give out the local transform
// --------------------------------------------------------------------------------
void EveChildBulletStorm::GetLocalToWorldTransform( Matrix& transform ) const
{
	transform = m_worldTransform;
}

// --------------------------------------------------------------------------------
// Description:
//   Used by state machine to animate state-related stuff
// --------------------------------------------------------------------------------
void EveChildBulletStorm::PlayCurveSet( const std::string& name )
{
}

// --------------------------------------------------------------------------------
// Description:
//   Used by state machine to animate state-related stuff
// --------------------------------------------------------------------------------
void EveChildBulletStorm::StopCurveSet( const std::string& name )
{
}

// --------------------------------------------------------------------------------
// Description:
//   Used by state machine to animate state-related stuff
// --------------------------------------------------------------------------------
float EveChildBulletStorm::GetCurveSetDuration( const std::string& name ) const
{
	return 0.f;
}

// --------------------------------------------------------------------------------
// Description:
//   Used by state machine to animate state-related stuff
// --------------------------------------------------------------------------------
void EveChildBulletStorm::Transform( const Vector3* scale, const Quaternion* rotation, const Vector3* translation )
{
}

// --------------------------------------------------------------------------------
// Description:
//   Nothing transparent here, all additive!
// --------------------------------------------------------------------------------
bool EveChildBulletStorm::HasTransparentBatches()
{
	return false;
}

// --------------------------------------------------------------------------------
// Description:
//   Hand out batches to render
// --------------------------------------------------------------------------------
void EveChildBulletStorm::GetBatches( ITriRenderBatchAccumulator* batches, TriBatchType batchType, const Tr2PerObjectData* perObjectData )
{
	if( batchType != TRIBATCHTYPE_ADDITIVE )
	{
		return;
	}

	if( !m_vertexBuffer.IsValid() || !m_perInstanceBuffer.IsValid() || !m_effect )
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

	TriForwardingBatch* batch = batches->Allocate<TriForwardingBatch>();
	if( batch )
	{
		batch->SetPerObjectData( perObjectData );
		batch->SetShaderMaterial( m_effect );
		batch->SetGeometryProvider( this );
		batches->Commit( batch );
	}
}

// --------------------------------------------------------------------------------
// Description:
//   No shadows here
// --------------------------------------------------------------------------------
void EveChildBulletStorm::GetShadowBatches( ITriRenderBatchAccumulator* batches, const Tr2PerObjectData* perObjectData )
{
}

// --------------------------------------------------------------------------------
// Description:
//   No transparency, no sorting
// --------------------------------------------------------------------------------
float EveChildBulletStorm::GetSortValue()
{
	return 0.f;
}

// --------------------------------------------------------------------------------
// Description:
//   The perobject data
// --------------------------------------------------------------------------------
Tr2PerObjectData* EveChildBulletStorm::GetPerObjectData( ITriRenderBatchAccumulator* accumulator )
{
	EveChildBulletStormPerObjectData* perObjectData = accumulator->Allocate<EveChildBulletStormPerObjectData>();
	if( !perObjectData )
	{
		return nullptr;
	}

	perObjectData->m_worldTransform = XMMatrixTranspose( m_worldTransform );
	perObjectData->m_targetPositionInfo = Vector4( float(m_targetObjects.size()), 0.f, 0.f, 0.f );
	for( size_t i = 0; i < m_targetBlobs.size(); ++i )
	{
		perObjectData->m_targetPositionsWS[i] = m_targetBlobs[i];
	}

	return perObjectData;
}

// --------------------------------------------------------------------------------
// Description:
//   Need our own version of perobject data and how to set it
// --------------------------------------------------------------------------------
void EveChildBulletStormPerObjectData::SetPerObjectDataToDevice( Tr2ConstantBufferAL** buffers, unsigned constantTypeMask, Tr2RenderContext& renderContext ) const
{
	FillAndSetConstants( *buffers[Tr2RenderContextEnum::VERTEX_SHADER], &m_worldTransform, 64 + 16 + 10 * 16, Tr2RenderContextEnum::VERTEX_SHADER, Tr2Renderer::GetPerObjectVSStartRegister(), renderContext );
//	FillAndSetConstants( *buffers[Tr2RenderContextEnum::PIXEL_SHADER], &m_displayData, ( 4 + Tr2ShLightingManager::PACKED_COEFFICIENT_COUNT ) * 16, PIXEL_SHADER, Tr2Renderer::GetPerObjectPSStartRegister(), renderContext );
}



