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

// consts
static const size_t BULLETSTORM_MAX_TARGETBLOBS = 10;
static const float BULLETSTORM_MIN_TARGETSIZE = 4050.f;

// vertex layout struct
struct BulletStormVertex
{
	float cornerID;
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
	m_speed( 1000.f ),
	m_clipShereMul( 0 ),
	m_changingClipSphere( false ),
	m_range( 1000.f ),
	m_sourceRadius( 0.f ),
	m_clipSphere( 1.f ),
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
	if( IsMatch( val, m_multiplier ) || IsMatch( val, m_sourceObject ) || IsMatch( val, m_sourceLocatorSet ) )
	{
		Rebuild();
	}

	return true;
}

// --------------------------------------------------------------------------------
const char* EveChildBulletStorm::GetName() const
{
	return m_name.c_str();
}

// --------------------------------------------------------------------------------
void EveChildBulletStorm::SetName( const char* name )
{
	m_name = name;
}

// --------------------------------------------------------------------------------
// Description:
//   Trinity's way of rendering
// --------------------------------------------------------------------------------
void EveChildBulletStorm::GetRenderables( std::vector<ITr2Renderable*>& renderables )
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
	m_vertexBuffer = Tr2BufferAL();
	m_perInstanceBuffer = Tr2BufferAL();
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
		vd.Add( vd.FLOAT32_1, vd.TEXCOORD, 0, 0 );
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
		verts[0].cornerID = 0.f;
		verts[1].cornerID = 1.f;
		verts[2].cornerID = 2.f;
		verts[3].cornerID = 3.f;

		// crate vertexbuffer and init it
		USE_MAIN_THREAD_RENDER_CONTEXT();
		CR_RETURN_VAL( m_vertexBuffer.Create( sizeof( BulletStormVertex ), 4, Tr2GpuUsage::VERTEX_BUFFER, Tr2CpuUsage::NONE, &verts[0], renderContext ), false );
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
	m_perInstanceBuffer = Tr2BufferAL();

	// if we don't have a shooter, that's it: exit
	if( !m_sourceObject )
	{
		return;
	}

	// get the specified locator set from the sourceobject, if it has it
	const LocatorStructureList* locatorList = m_sourceObject->GetLocatorsForSet( BlueSharedString( m_sourceLocatorSet.c_str() ) );
	if( !locatorList )
	{
		return;
	}

	// re-build the per-instance buffer here
	m_objectCount = m_multiplier * (unsigned int)locatorList->size();
	if( m_objectCount > 0 )
	{
		// put all locators from parent into mem
		std::vector<PerInstanceVertex> verts( m_objectCount );
		PerInstanceVertex* v = &verts[0];
		for( unsigned int i = 0; i < (unsigned int)locatorList->size(); ++i )
		{
			const Locator& locator = (*locatorList)[i];

			// per stream start
			Vector3 startPosOS = locator.position;
			Vector3 startDirOS = (Vector3)XMVector3Rotate( Vector3( 0.f, 1.f, 0.f ), locator.direction );
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
		CR_RETURN( m_perInstanceBuffer.Create( sizeof( PerInstanceVertex ), m_objectCount, Tr2GpuUsage::VERTEX_BUFFER, Tr2CpuUsage::NONE, &verts[0], renderContext ) );
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
void EveChildBulletStorm::UpdateSyncronous( EveUpdateContext&, const EveChildUpdateParams& )
{
}

void EveChildBulletStorm::StartEffect()
{
	m_clipShereMul = 1.0f;
	m_clipSphere = 0.0f;
	m_changingClipSphere = true;
}

// --------------------------------------------------------------------------------
// Description:
//   This stops bullets appearing, but keeps the bullets that have been generated, 
//   so the effects seems to be stopping
// --------------------------------------------------------------------------------
void EveChildBulletStorm::StopEffect() 
{
	m_clipShereMul = -1.0f;
	m_clipSphere = 0.0f;
	m_changingClipSphere = true;
}

// --------------------------------------------------------------------------------
// Description:
//   Helper method that is exposed to python, to see if we can start/stop the effect
//   We don't want the effect to stop when it is in the progress of starting and
//   vice versa
// --------------------------------------------------------------------------------
bool EveChildBulletStorm::CanChangeState()
{
	return !m_changingClipSphere;
}

// --------------------------------------------------------------------------------
// Description:
//   Assyncronous updates happen here
// --------------------------------------------------------------------------------
void EveChildBulletStorm::UpdateAsyncronous( EveUpdateContext& updateContext, const EveChildUpdateParams& params )
{
	// we need to move with our parent
	if( params.spaceObjectParent )
	{
		params.spaceObjectParent->GetLocalToWorldTransform( m_worldTransform );
	}

	// combining target blobs from the actuall spaceship targets
	m_targetBlobs.resize( min( m_targetObjects.size(), BULLETSTORM_MAX_TARGETBLOBS ) );
	for( size_t i = 0; i < m_targetBlobs.size(); ++i )
	{
		// world position
		Vector3 targetPosWS;
		m_targetObjects[i]->GetModelCenterWorldPosition( targetPosWS );
		// radius
		Vector4 targetSize;
		m_targetObjects[i]->GetBoundingSphere( targetSize );

		m_targetBlobs[i] = Vector4( targetPosWS, max( targetSize.w, BULLETSTORM_MIN_TARGETSIZE ) );
	}

	
	// keep the radius of the soutrceobject here
	if( m_sourceObject )
	{
		Vector4 sphere;
		if( m_sourceObject->GetBoundingSphere( sphere ) )
		{
			m_sourceRadius = sphere.w;
		}
	}

	// update the start/stop mechanic
	if( m_changingClipSphere )
	{
		m_clipSphere += m_clipShereMul * m_speed * updateContext.GetDeltaT() / (m_sourceRadius + m_range);
		m_clipSphere = std::max( -1.0f, std::min( 1.0f, m_clipSphere ) );
		m_changingClipSphere = std::abs( m_clipSphere ) != 1;
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
void EveChildBulletStorm::Setup( const Vector3* scale, const Quaternion* rotation, const Vector3* translation, Tr2Lod lowestLodVisible )
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
void EveChildBulletStorm::GetBatches( ITriRenderBatchAccumulator* batches, TriBatchType batchType, const Tr2PerObjectData* perObjectData, Tr2RenderReason reason )
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
	perObjectData->m_effectInfo = Vector4( float(m_targetObjects.size()), m_sourceRadius + m_range, m_clipSphere, m_speed );

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
}



