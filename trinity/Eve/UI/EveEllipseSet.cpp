// Copyright © 2026 CCP ehf.

#include "StdAfx.h"
#include "EveEllipseSet.h"
#include "Eve/EveConstantBufferFormats.h"
#include "TriRenderBatch.h"
#include "Tr2Renderer.h"
#include "Tr2PerObjectData.h"
#include "Shader/Tr2EffectStateManager.h"
#include "Utilities/BoundingSphere.h"
#include "TriMath.h"
#include "TriSettingsRegistrar.h"
#include "TriPoolAllocator.h"

namespace
{

struct EllipseVertex
{
	Vector3 centerPos;
	Vector4 tangentSide; // TEXCOORD0: (tangent.xyz, sideSign) — sideSign = -1 left, +1 right
};
static_assert( sizeof( EllipseVertex ) == 28u, "EllipseVertex stride must match Tr2VertexDefinition stream layout." );

// Same construction as frontier.hud.system_view.scene_new.util._get_orbit_rotation_from_axes:
// basis_u = normalize(n x ref), basis_v = normalize(basis_u x n). Rotation degrees are atan2(maj·v, maj·u) in that frame.
void BuildOrbitPlaneBasisPython( const Vector3& planeNormal, Vector3& basisU, Vector3& basisV, Vector3& n )
{
	n = Normalize( planeNormal );
	Vector3 ref( 0.f, 1.f, 0.f );
	basisU = Cross( n, ref );
	float lenSq = Dot( basisU, basisU );
	if( lenSq < 1e-3f )
	{
		const Vector3 ax0( 1.f, 0.f, 0.f );
		const Vector3 ax1( 0.f, 0.f, 1.f );
		const float d0 = std::fabs( Dot( n, ax0 ) );
		const float d1 = std::fabs( Dot( n, ax1 ) );
		ref = ( d0 < d1 ) ? ax0 : ax1;
		basisU = Cross( n, ref );
		lenSq = Dot( basisU, basisU );
	}
	CCP_ASSERT( lenSq > 0.f );
	basisU = basisU / std::sqrt( lenSq );
	basisV = Cross( basisU, n );
	const float lv = Length( basisV );
	basisV = basisV / lv;
}

// Major along U, minor along V; rotationDegrees matches Python atan2(maj·basis_v, maj·basis_u).
void BuildOrbitEllipseAxes( const EveEllipseDefinition& spec, Vector3& outU, Vector3& outV )
{
	Vector3 basisU, basisV, n;
	BuildOrbitPlaneBasisPython( spec.m_planeNormal, basisU, basisV, n );
	const float phiRad = spec.m_rotationDegrees * ( TRI_PI / 180.f );
	const float cf = cosf( phiRad );
	const float sf = sinf( phiRad );
	// unit since basisU and basisV are unit
	outU = basisU * cf + basisV * sf;
	outV = Cross( outU, n );
	const float lv = Length( outV );
	outV = outV / lv;
}

} // namespace

std::string g_ellipseRibbonEffectPath = "";
TRI_REGISTER_SETTING( "ellipseRibbonEffectPath", g_ellipseRibbonEffectPath );

EveEllipseSet::EveEllipseSet( IRoot* lockobj ) :
	PARENTLOCK( m_ellipses )
{
	m_ellipses.SetNotify( this );
	BoundingSphereInitialize( m_boundingSphere );
	PrepareResources();
}

void EveEllipseSet::ReleaseResources( TriStorage s )
{
	m_vertexBuffer = Tr2BufferAL();
	m_indexBuffer = Tr2BufferAL();
	m_vertexDeclHandle = Tr2EffectStateManager::UNINITIALIZED_DECLARATION;
	m_indexCount = 0;
	m_geometryDirty = true;
}

bool EveEllipseSet::OnPrepareResources()
{
	if( m_vertexDeclHandle != Tr2EffectStateManager::UNINITIALIZED_DECLARATION )
	{
		return true;
	}

	Tr2VertexDefinition decl;
	decl.Add( decl.FLOAT32_3, decl.POSITION );
	// TEXCOORD0 tangent + sideSign
	decl.Add( decl.FLOAT32_4, decl.TEXCOORD, 0 );
	m_vertexDeclHandle = Tr2EffectStateManager::GetVertexDeclarationHandle( decl );
	return m_vertexDeclHandle != Tr2EffectStateManager::UNINITIALIZED_DECLARATION;
}

void EveEllipseSet::ClearEllipses()
{
	for( auto& spec : m_ellipses )
	{
		spec->SetDirtyFlag( nullptr );
	}
	m_ellipses.Clear();
	MarkGeometryDirty();
}

bool EveEllipseSet::AddEllipse( const Vector3& center, float semiMajor, float semiMinor, const Vector3& planeNormal, float rotationDegrees )
{
	EveEllipseDefinitionPtr spec;
	if( !spec.CreateInstance() )
	{
		return false;
	}
	spec->m_center = center;
	spec->m_semiMajor = semiMajor;
	spec->m_semiMinor = semiMinor;
	spec->m_planeNormal = planeNormal;
	spec->m_rotationDegrees = rotationDegrees;
	spec->SetDirtyFlag( &m_geometryDirty );
	m_ellipses.Insert( -1, spec );
	MarkGeometryDirty();
	return true;
}

const char* EveEllipseSet::GetName() const
{
	return m_name.c_str();
}

void EveEllipseSet::SetName( const char* name )
{
	m_name = BlueSharedString( name );
}

EveEllipseSet::~EveEllipseSet()
{
	// Specs may outlive via Python references
	for( auto& spec : m_ellipses )
	{
		spec->SetDirtyFlag( nullptr );
	}
}

void EveEllipseSet::OnListModified( long event, ssize_t key, ssize_t key2, IRoot* value, const IList* list )
{
	if( list == &m_ellipses && ( event & BELIST_LOADING ) == 0 )
	{
		switch( event & BELIST_EVENTMASK )
		{
		case BELIST_INSERTED:
			if( EveEllipseDefinitionPtr spec = BlueCastPtr( value ) )
			{
				spec->SetDirtyFlag( &m_geometryDirty );
			}
			break;
		case BELIST_REMOVED:
			if( EveEllipseDefinitionPtr spec = BlueCastPtr( value ) )
			{
				spec->SetDirtyFlag( nullptr );
			}
			break;
		case BELIST_LOADFINISHED:
			// Bulk load (deserialization / AssignTo) suppresses per-item INSERTED — bind dirty flags now.
			for( auto& spec : m_ellipses )
			{
				spec->SetDirtyFlag( &m_geometryDirty );
			}
			break;
		case BELIST_UNLOADSTART:
			// Bulk clear (Remove(-1)) suppresses per-item REMOVED — unbind dirty flags before items are popped.
			for( auto& spec : m_ellipses )
			{
				spec->SetDirtyFlag( nullptr );
			}
			break;
		default:
			break;
		}
	}
	MarkGeometryDirty();
}

void EveEllipseSet::MarkGeometryDirty()
{
	m_geometryDirty = true;
}

bool EveEllipseSet::OnModified( Be::Var* )
{
	MarkGeometryDirty();
	return true;
}

void EveEllipseSet::py__init__()
{
	if( !m_effect && !g_ellipseRibbonEffectPath.empty() )
	{
		m_effect.CreateInstance();
		m_effect->StartUpdate();
		m_effect->SetEffectPathName( g_ellipseRibbonEffectPath.c_str() );
		m_effect->EndUpdate();
	}
}

void EveEllipseSet::UpdateSyncronous( const EveUpdateContext& updateContext, const EveChildUpdateParams& params )
{
	if( m_vertexDeclHandle == Tr2EffectStateManager::UNINITIALIZED_DECLARATION )
	{
		PrepareResources();
	}
	RefreshGeometryIfNeeded( updateContext );
}

void EveEllipseSet::UpdateAsyncronous( const EveUpdateContext& updateContext, const EveChildUpdateParams& params )
{
	UpdateTransform( params.localToWorldTransform );
}

void EveEllipseSet::RefreshGeometryIfNeeded( const EveUpdateContext& updateContext )
{
	if( !m_display )
	{
		return;
	}

	if( !m_geometryDirty )
	{
		return;
	}

	// RebuildGeometry clears m_geometryDirty on success paths only — CR_RETURN bailouts on
	// GPU buffer failure leave it true so the next frame retries.
	RebuildGeometry( updateContext );
}

void EveEllipseSet::RebuildGeometry( const EveUpdateContext& updateContext )
{
	m_indexCount = 0;

	BoundingSphereInitialize( m_boundingSphere );

	const unsigned int segmentCount = std::max( 16u, std::min( 512u, m_ribbonSegmentCount ) );
	const unsigned int ellipseCount = static_cast<unsigned int>( m_ellipses.size() );
	const uint32_t vertexCount = ellipseCount * 2u * segmentCount;
	const uint32_t indexCount = ellipseCount * 6u * segmentCount;

	if( ellipseCount == 0 )
	{
		m_vertexBuffer = Tr2BufferAL();
		m_indexBuffer = Tr2BufferAL();
		m_geometryDirty = false;
		return;
	}

	TriPoolAllocator* allocator = Tr2Renderer::GetPoolAllocator();
	if( !allocator )
	{
		return;
	}

	auto vertices = reinterpret_cast<EllipseVertex*>( allocator->Allocate( vertexCount * sizeof( EllipseVertex ) ) );
	auto indices = reinterpret_cast<uint32_t*>( allocator->Allocate( indexCount * sizeof( uint32_t ) ) );
	if( !vertices || !indices )
	{
		return;
	}

	uint32_t vWrite = 0;
	uint32_t iWrite = 0;

	// Screen-space billboard ribbon: store centerline + tangent; VS expands to pixel width at draw time.
	for( auto& spec : m_ellipses )
	{
		Vector3 semiMajorAxis, semiMinorAxis;
		BuildOrbitEllipseAxes( *spec, semiMajorAxis, semiMinorAxis );

		const Vector3& center = spec->m_center;
		const float semiMajor = std::max( spec->m_semiMajor, 1e-4f );
		const float semiMinor = std::max( spec->m_semiMinor, 1e-4f );

		const uint32_t baseVertexIndex = vWrite;

		for( unsigned int i = 0; i < segmentCount; ++i )
		{
			const float angle = ( static_cast<float>( i ) / static_cast<float>( segmentCount ) ) * ( 2.f * TRI_PI );
			const float cosAngle = cosf( angle );
			const float sinAngle = sinf( angle );
			const Vector3 point = center + semiMajorAxis * ( semiMajor * cosAngle ) + semiMinorAxis * ( semiMinor * sinAngle );
			Vector3 tangent = semiMajorAxis * ( -semiMajor * sinAngle ) + semiMinorAxis * ( semiMinor * cosAngle );
			float tangentLength = Length( tangent );
			if( tangentLength < 1e-12f )
			{
				tangent = semiMajorAxis;
				tangentLength = Length( tangent );
			}
			tangent = tangent * ( 1.f / tangentLength );

			// Left edge: sideSign = -1
			vertices[vWrite].centerPos = point;
			vertices[vWrite].tangentSide = Vector4( tangent.x, tangent.y, tangent.z, -1.f );
			BoundingSphereUpdate( point, m_boundingSphere );
			vWrite++;

			// Right edge: sideSign = +1
			vertices[vWrite].centerPos = point;
			vertices[vWrite].tangentSide = Vector4( tangent.x, tangent.y, tangent.z, 1.f );
			BoundingSphereUpdate( point, m_boundingSphere );
			vWrite++;
		}

		for( unsigned int i = 0; i < segmentCount; ++i )
		{
			const unsigned int next = ( i + 1u ) % segmentCount;
			const uint32_t curLeft = baseVertexIndex + 2u * i;
			const uint32_t curRight = baseVertexIndex + 2u * i + 1u;
			const uint32_t nextLeft = baseVertexIndex + 2u * next;
			const uint32_t nextRight = baseVertexIndex + 2u * next + 1u;
			indices[iWrite++] = curLeft;
			indices[iWrite++] = nextLeft;
			indices[iWrite++] = curRight;
			indices[iWrite++] = curRight;
			indices[iWrite++] = nextLeft;
			indices[iWrite++] = nextRight;
		}
	}

	USE_MAIN_THREAD_RENDER_CONTEXT();

	const uint32_t vertexStride = static_cast<uint32_t>( sizeof( EllipseVertex ) );
	if( !m_vertexBuffer.IsValid() || m_vertexBuffer.GetSize() < vertexStride * vertexCount )
	{
		CR_RETURN( m_vertexBuffer.Create( vertexStride, vertexCount, Tr2GpuUsage::VERTEX_BUFFER, Tr2CpuUsage::WRITE, nullptr, renderContext ) );
	}
	{
		void* dst = nullptr;
		CR_RETURN( m_vertexBuffer.MapForWriting( dst, renderContext ) );
		memcpy( dst, vertices, static_cast<size_t>( vertexStride ) * vertexCount );
		m_vertexBuffer.UnmapForWriting( renderContext );
	}

	if( !m_indexBuffer.IsValid() || m_indexBuffer.GetSize() < sizeof( uint32_t ) * indexCount )
	{
		CR_RETURN( m_indexBuffer.Create( sizeof( uint32_t ), indexCount, Tr2GpuUsage::INDEX_BUFFER, Tr2CpuUsage::WRITE, nullptr, renderContext ) );
	}
	{
		void* dst = nullptr;
		CR_RETURN( m_indexBuffer.MapForWriting( dst, renderContext ) );
		memcpy( dst, indices, sizeof( uint32_t ) * indexCount );
		m_indexBuffer.UnmapForWriting( renderContext );
	}

	m_indexCount = indexCount;
	m_geometryDirty = false;
}

void EveEllipseSet::UpdateVisibility( const EveUpdateContext& updateContext, const Matrix& parentTransform, Tr2Lod parentLod )
{
	m_isVisible = false;
	if( !m_display )
	{
		return;
	}
	if( m_indexCount > 0 && parentLod >= m_lowestLodVisible )
	{
		m_isVisible = true;
	}
}

void EveEllipseSet::GetRenderables( std::vector<ITr2Renderable*>& renderables )
{
	if( m_isVisible && m_indexCount > 0 )
	{
		renderables.push_back( this );
	}
}

bool EveEllipseSet::GetBoundingSphere( Vector4& sphere, BoundingSphereQuery query ) const
{
	sphere = m_boundingSphere;
	BoundingSphereTransform( m_worldTransform, sphere );
	return true;
}

void EveEllipseSet::GetLocalToWorldTransform( Matrix& transform ) const
{
	transform = m_worldTransform;
}

void EveEllipseSet::Setup( const Vector3* scale, const Quaternion* rotation, const Vector3* translation, Tr2Lod lowestLodVisible )
{
	// call base class's setup
	EveChildTransform::Setup( scale, rotation, translation, lowestLodVisible );

	// and remember lodding!
	m_lowestLodVisible = lowestLodVisible;
}

void EveEllipseSet::ChangeLOD( Tr2Lod lod )
{
}

IRoot* EveEllipseSet::GetID( uint16_t )
{
	return GetRawRoot();
}

void EveEllipseSet::CommitRibbonBatch( ITriRenderBatchAccumulator* accumulator, const Tr2PerObjectData* perObjectData, Tr2EffectPtr& effect )
{
	Tr2RenderBatch batch;
	batch.SetMaterial( effect );
	batch.SetPerObjectData( perObjectData );

	float maxDepth = Tr2Renderer::GetFrustumRadius();
	Vector3 center( m_boundingSphere.x, m_boundingSphere.y, m_boundingSphere.z );
	center = TransformCoord( center, m_worldTransform );
	center -= Tr2Renderer::GetViewPosition();
	float z = std::min( std::max( ( Length( center ) + m_depthOffset ) / maxDepth, 0.f ), 1.f );
	batch.m_depth = static_cast<unsigned int>( static_cast<float>( 0xFFFFFFF ) * ( 1.0f - z ) );

	batch.SetVertexDeclaration( m_vertexDeclHandle );
	batch.SetStreamSource( 0, m_vertexBuffer, sizeof( EllipseVertex ) );
	batch.SetInidices( m_indexBuffer, sizeof( uint32_t ) );
	batch.SetTopology( Tr2RenderContextEnum::TOP_TRIANGLES );
	batch.SetDrawIndexedInstanced( m_indexCount, 1, 0, 0, 0 );
	accumulator->Commit( batch );
}

void EveEllipseSet::GetBatches( ITriRenderBatchAccumulator* accumulator, TriBatchType batchType, const Tr2PerObjectData* perObjectData, Tr2RenderReason reason )
{
	if( !m_display || !perObjectData )
	{
		return;
	}
	if( m_vertexDeclHandle == Tr2EffectStateManager::UNINITIALIZED_DECLARATION )
	{
		PrepareResources();
	}
	if( !m_effect || !m_vertexBuffer.IsValid() || !m_indexBuffer.IsValid() || m_indexCount == 0 || m_vertexDeclHandle == Tr2EffectStateManager::UNINITIALIZED_DECLARATION )
	{
		return;
	}

	if( batchType == TRIBATCHTYPE_PICKING )
	{
		CommitRibbonBatch( accumulator, perObjectData, m_effect );
		return;
	}

	if( batchType != TRIBATCHTYPE_TRANSPARENT )
	{
		return;
	}

	CommitRibbonBatch( accumulator, perObjectData, m_effect );
}

void EveEllipseSet::GetPickingBatches( ITriRenderBatchAccumulator* batches, Tr2PickTypes pickTypes, const Tr2PerObjectData* perObjectData )
{
	if( !m_enablePicking )
	{
		return;
	}
	if( ( pickTypes & PICK_TYPE_PICKING ) != 0 )
	{
		GetBatches( batches, TRIBATCHTYPE_PICKING, perObjectData );
	}
}

bool EveEllipseSet::HasTransparentBatches()
{
	return m_display;
}

float EveEllipseSet::GetSortValue()
{
	Vector3 worldCenter( m_boundingSphere.x, m_boundingSphere.y, m_boundingSphere.z );
	worldCenter = TransformCoord( worldCenter, m_worldTransform );
	Vector3 toCamera = Tr2Renderer::GetViewPosition() - worldCenter;
	return Length( toCamera ) + m_depthOffset;
}

Tr2PerObjectData* EveEllipseSet::GetPerObjectData( ITriRenderBatchAccumulator* accumulator )
{
	Tr2PerObjectDataStandard* data = accumulator->Allocate<Tr2PerObjectDataStandard>();
	if( !data )
	{
		return nullptr;
	}

	EvePerObjectVSData vs{};
	EvePerObjectPSData ps{};
	vs.WorldMat = Transpose( m_worldTransform );
	ps.WorldMat = Transpose( m_worldTransform );

	data->CopyToVSFloatBuffer( vs );
	data->CopyToPSFloatBuffer( ps );
	return data;
}
