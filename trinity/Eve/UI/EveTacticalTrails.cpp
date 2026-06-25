// Copyright © 2025 CCP ehf.

#include "StdAfx.h"
#include "EveTacticalTrails.h"
#include "../../Shader/Tr2Effect.h"
#include "../../Tr2Renderer.h"
#include "../../TriPoolAllocator.h"
#include "../../TriRenderBatch.h"
#include <ITriFunction.h>
#include <numeric>

namespace
{

struct LineVertex
{
	Vector3 position; // position in 3D
	Vector3 lineDir; // [xyz] = offset to next/previous line segment
	float time;
	Vector2 quad; // quad coordinates (-1 to 1)
};

double DistanceFromPointToLine( const Vector3d& point, const Vector3d& lineStart, const Vector3d& lineEnd )
{
	Vector3d line = lineEnd - lineStart;
	Vector3d lineToPoint = point - lineStart;
	double lineLengthSquared = LengthSq( line );
	if( lineLengthSquared == 0 )
	{
		return Length( lineToPoint );
	}
	double t = Dot( lineToPoint, line ) / lineLengthSquared;
	Vector3d projection = lineStart + line * t;
	return Length( point - projection );
}

const uint32_t SEGMENT_VERTEX_COUNT = 12; // 4 triangles: one quad and two connectors

void WriteLineVerticesToBuffer(
	const Vector3& pos1,
	float time1,
	const Vector3& pos2,
	float time2,
	const Vector3& pos3,
	LineVertex* buffer )
{
	Vector3 dirOffset = pos2 - pos1;

	buffer[0] = { pos1, dirOffset, time1, { -1.f, -1.f } };
	buffer[1] = { pos1, dirOffset, time1, { -1.f, 1.f } };
	buffer[2] = { pos2, -dirOffset, time2, { 1.f, -1.f } };

	buffer[3] = { pos1, dirOffset, time1, { -1.f, 1.f } };
	buffer[4] = { pos2, -dirOffset, time2, { 1.f, 1.f } };
	buffer[5] = { pos2, -dirOffset, time2, { 1.f, -1.f } };


	buffer[6] = { pos2, -dirOffset, time2, { 1.f, -1.f } };
	buffer[7] = { pos2, -dirOffset, time2, { 1.f, 0.f } };
	buffer[8] = { pos2, pos3 - pos2, time2, { -1.f, -1.f } };

	buffer[9] = { pos2, -dirOffset, time2, { 1.f, 0.f } };
	buffer[10] = { pos2, -dirOffset, time2, { 1.f, 1.f } };
	buffer[11] = { pos2, pos3 - pos2, time2, { -1.f, -1.f } };
}

}

EveTacticalTrails::EveTacticalTrails( IRoot* )
{
}

void EveTacticalTrails::ReleaseResources( TriStorage )
{
	m_vertexDeclHandle = Tr2EffectStateManager::UNINITIALIZED_DECLARATION;
}

bool EveTacticalTrails::OnPrepareResources()
{
	return true;
}


void EveTacticalTrails::RegisterObject( ITriVectorFunction* object )
{
	if( !object )
	{
		CCP_LOGERR( "EveTacticalTrails::RegisterObject: Object is null" );
		return;
	}
	auto found = std::find_if( m_trackedObjects.begin(), m_trackedObjects.end(), [object]( const TrackedObject& obj ) { return obj.ball == object; } );
	if( found == m_trackedObjects.end() )
	{
		TrackedObject newObject;
		newObject.ball = object;
		m_trackedObjects.push_back( newObject );
	}
	else
	{
		CCP_LOGWARN( "EveTacticalTrails::RegisterObject: Object already registered" );
	}
}

void EveTacticalTrails::UnregisterObject( ITriVectorFunction* object )
{
	auto found = std::find_if( m_trackedObjects.begin(), m_trackedObjects.end(), [object]( const TrackedObject& obj ) { return obj.ball == object; } );
	if( found != m_trackedObjects.end() )
	{
		found->ball = nullptr;
	}
}

void EveTacticalTrails::UpdateSyncronous( const EveUpdateContext& updateContext )
{
	auto now = updateContext.GetTime();
	// Remove expired control points
	for( auto& object : m_trackedObjects )
	{
		if( object.positions.size() >= 2 )
		{
			auto it = begin( object.positions );
			while( it + 1 != end( object.positions ) && TimeAsFloat( now - ( it + 1 )->time ) > m_fadeOutTime )
			{
				++it;
			}
			object.positions.erase( begin( object.positions ), it );
		}
	}
	// Remove deleted objects when trails fully faded out
	m_trackedObjects.erase( std::remove_if( m_trackedObjects.begin(), m_trackedObjects.end(), []( const auto& obj ) { return !obj.ball && obj.positions.size() < 2; } ), m_trackedObjects.end() );

	// Add new control points
	for( auto& object : m_trackedObjects )
	{
		if( !object.ball )
		{
			continue;
		}
		Vector3d position = { 0, 0, 0 };
		object.ball->InterpolatedPosition( &position, now );
		if( object.positions.size() < 2 )
		{
			object.positions.push_back( { position, now } );
		}
		else
		{
			auto& p0 = object.positions[object.positions.size() - 2].position;
			auto& p1 = object.positions[object.positions.size() - 1].position;

			auto distance = DistanceFromPointToLine( p1, p0, position );
			if( distance > 1 || Dot( position - p1, p0 - p1 ) > 0 || Length( p1 - p0 ) > 10000 )
			{
				object.positions.push_back( { position, now } );
			}
			else
			{
				object.positions.back() = { position, now };
			}
		}
	}

	if( m_egoBall )
	{
		m_egoBall->InterpolatedPosition( &m_egoBallPosition, now );
	}
	else
	{
		m_egoBallPosition = { 0, 0, 0 };
	}

	UpdateGraphicsState( now );
}

void EveTacticalTrails::UpdateGraphicsState( Be::Time now )
{
	m_segmentCount = accumulate( m_trackedObjects.begin(), m_trackedObjects.end(), 0u, []( uint32_t sum, const auto& object ) { return sum + static_cast<uint32_t>( std::max( object.positions.size(), size_t( 1 ) ) - 1 ); } );
	if( m_segmentCount == 0 )
	{
		return;
	}

	USE_MAIN_THREAD_RENDER_CONTEXT();
	if( !renderContext.IsValid() )
	{
		m_segmentCount = 0;
		return;
	}

	if( m_vertexDeclHandle == Tr2EffectStateManager::UNINITIALIZED_DECLARATION )
	{
		static Tr2VertexDefinition s_curveLineDataVertexDecl;
		if( s_curveLineDataVertexDecl.empty() )
		{
			Tr2VertexDefinition& tvd = s_curveLineDataVertexDecl;

			tvd.Add( tvd.FLOAT32_3, tvd.POSITION );
			tvd.Add( tvd.FLOAT32_3, tvd.TEXCOORD, 0 );
			tvd.Add( tvd.FLOAT32_1, tvd.TEXCOORD, 1 );
			tvd.Add( tvd.FLOAT32_2, tvd.TEXCOORD, 2 );
		}

		m_vertexDeclHandle = Tr2EffectStateManager::GetVertexDeclarationHandle( s_curveLineDataVertexDecl );
	}

	auto data = reinterpret_cast<LineVertex*>( Tr2Renderer::GetPoolAllocator()->Allocate( m_segmentCount * SEGMENT_VERTEX_COUNT * sizeof( LineVertex ) ) );
	if( !data )
	{
		return;
	}
	auto dest = data;
	for( auto& object : m_trackedObjects )
	{
		if( object.positions.size() < 2 )
		{
			continue;
		}

		auto ToWorld = [&]( const Vector3d& pos ) -> Vector3 {
			return ( pos - m_egoBallPosition ).AsVector3();
		};

		Vector3 pos1 = ToWorld( object.positions[0].position );
		float time1 = TimeAsFloat( now - object.positions[0].time ) / m_fadeOutTime;
		Vector3 pos2 = ToWorld( object.positions[1].position );

		for( size_t i = 0; i + 1 < object.positions.size(); ++i )
		{
			float time2 = TimeAsFloat( now - object.positions[i + 1].time ) / m_fadeOutTime;
			auto posNext = i + 2 >= object.positions.size() ? pos2 + ( pos2 - pos1 ) : ToWorld( object.positions[i + 2].position );
			WriteLineVerticesToBuffer(
				pos1,
				time1,
				pos2,
				time2,
				posNext,
				dest );
			dest += SEGMENT_VERTEX_COUNT;
			pos1 = pos2;
			pos2 = posNext;
			time1 = time2;
		}
	}

	if( !m_vertexBuffer.IsValid() || m_vertexBuffer.GetDesc().count < m_segmentCount * SEGMENT_VERTEX_COUNT )
	{
		auto capacity = std::max( 1024u, m_segmentCount * SEGMENT_VERTEX_COUNT * 2 );
		m_vertexBuffer.Create( sizeof( LineVertex ), capacity, Tr2GpuUsage::VERTEX_BUFFER, Tr2CpuUsage::WRITE_OFTEN, nullptr, renderContext );
	}
	void* bufferData = nullptr;
	if( SUCCEEDED( m_vertexBuffer.MapForWriting( bufferData, renderContext ) ) && bufferData )
	{
		memcpy( bufferData, data, m_segmentCount * SEGMENT_VERTEX_COUNT * sizeof( LineVertex ) );
		m_vertexBuffer.UnmapForWriting( renderContext );
	}
	else
	{
		m_segmentCount = 0;
	}
}

void EveTacticalTrails::UpdateAsyncronous( const EveUpdateContext& updateContext )
{
}

void EveTacticalTrails::UpdateVisibility( const EveUpdateContext& updateContext, const Matrix& parentTransform )
{
}

void EveTacticalTrails::GetRenderables( std::vector<ITr2Renderable*>& renderables, Tr2ImpostorManager* )
{
	renderables.push_back( this );
}

bool EveTacticalTrails::GetBoundingSphere( Vector4&, BoundingSphereQuery ) const
{
	return false;
}

void EveTacticalTrails::UpdateModelCenterWorldPosition( Vector3& position, Be::Time t )
{
	position = { 0.f, 0.f, 0.f };
}

void EveTacticalTrails::GetModelCenterWorldPosition( Vector3& position ) const
{
	position = { 0.f, 0.f, 0.f };
}

bool EveTacticalTrails::GetLocalBoundingBox( Vector3& min, Vector3& max )
{
	return false;
}

void EveTacticalTrails::GetLocalToWorldTransform( Matrix& transform ) const
{
	transform = IdentityMatrix();
}

void EveTacticalTrails::GetBatches( ITriRenderBatchAccumulator* batches, TriBatchType batchType, const Tr2PerObjectData* perObjectData, Tr2RenderReason reason )
{
	if( batchType != TRIBATCHTYPE_TRANSPARENT )
	{
		return;
	}
	if( m_segmentCount == 0 || !m_vertexBuffer.IsValid() || m_vertexDeclHandle == Tr2EffectStateManager::UNINITIALIZED_DECLARATION )
	{
		return;
	}
	Tr2RenderBatch batch;
	batch.SetMaterial( m_trailEffect );
	batch.SetPerObjectData( perObjectData );
	batch.SetVertexDeclaration( m_vertexDeclHandle );
	batch.SetStreamSource( 0, m_vertexBuffer, sizeof( LineVertex ) );
	batch.SetDrawInstanced( SEGMENT_VERTEX_COUNT * m_segmentCount, 1, 0, 0 );
	batch.SetRenderingMode( Tr2EffectStateManager::RM_ALPHA );
	batches->Commit( batch );
}

bool EveTacticalTrails::HasTransparentBatches()
{
	return true;
}

float EveTacticalTrails::GetSortValue()
{
	return 0;
}

Tr2PerObjectData* EveTacticalTrails::GetPerObjectData( ITriRenderBatchAccumulator* )
{
	return nullptr;
}
