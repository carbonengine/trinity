// Copyright © 2026 CCP ehf.

#include "StdAfx.h"
#include "EveAudioOccluder.h"
#include "Resources/TriGeometryRes.h"
#include "TriSettingsRegistrar.h"

#include <algorithm>
#include <atomic>

// Only the big meshes block sound: every mesh in Wwise costs ray tracing time, and the small ones block little.
// 0 hands every mesh to carbon-audio.
float g_eveAudioOcclusionMinSize = 1000.0f;
TRI_REGISTER_SETTING( "eveAudioOcclusionMinSize", g_eveAudioOcclusionMinSize );

static std::atomic<uint64_t> s_nextInstanceId{ 1 };

// How big a mesh is as something that blocks sound: the middle side of its bounding box in the world.
// A wall or a block scores its width; a pipe or a beam scores its thickness however long it is.
static float OccluderSize( const Tr2AudGeometryData& data, const Matrix& m )
{
	const Vector3 box = data.m_maxBounds - data.m_minBounds;
	float sides[3] = {
		box.x * Length( Vector3( m._11, m._12, m._13 ) ),
		box.y * Length( Vector3( m._21, m._22, m._23 ) ),
		box.z * Length( Vector3( m._31, m._32, m._33 ) ),
	};
	std::sort( sides, sides + 3 );
	return sides[1];
}

EveAudioOccluder::EveAudioOccluder() :
	m_instanceId( s_nextInstanceId++ )
{
}

EveAudioOccluder::~EveAudioOccluder()
{
	Release();
}

EveAudioOccluder::EveAudioOccluder( EveAudioOccluder&& other ) noexcept :
	m_audio( other.m_audio ),
	m_instanceId( other.m_instanceId ),
	m_geometrySetId( other.m_geometrySetId ),
	m_transform( other.m_transform )
{
	// The registration moved with the instance ID, so the other one needs its own.
	other.m_instanceId = s_nextInstanceId++;
	other.m_geometrySetId = 0;
}

EveAudioOccluder& EveAudioOccluder::operator=( EveAudioOccluder&& other ) noexcept
{
	if( this != &other )
	{
		Release();
		// The registration belongs to the instance ID, so the IDs swap and both stay unique.
		std::swap( m_instanceId, other.m_instanceId );
		m_audio = other.m_audio;
		m_geometrySetId = other.m_geometrySetId;
		m_transform = other.m_transform;
		other.m_geometrySetId = 0;
	}
	return *this;
}

ITr2AudGeometryPtr EveAudioOccluder::CreateAudioGeometry()
{
	ITr2AudGeometryPtr audio;
	BeClasses->CreateInstanceFromName( "AudGeometry", BlueInterfaceIID<ITr2AudGeometry>(), reinterpret_cast<void**>( &audio.p ) );
	return audio;
}

void EveAudioOccluder::Update( ITr2AudGeometry* audio, TriGeometryRes* geometry, unsigned int meshIndex, const Matrix& worldTransform )
{
	const AudioGeometryResData* audioGeometry = nullptr;
	if( audio && geometry && geometry->IsGood() )
	{
		audioGeometry = geometry->GetAudioGeometry( meshIndex );
	}

	if( audioGeometry && OccluderSize( audioGeometry->m_data, worldTransform ) < g_eveAudioOcclusionMinSize )
	{
		audioGeometry = nullptr;
	}

	if( !audioGeometry )
	{
		Release();
		return;
	}

	if( m_geometrySetId == audioGeometry->m_id && m_audio.p == audio )
	{
		SetTransform( worldTransform );
		return;
	}

	Release();
	m_audio = audio;
	m_geometrySetId = audioGeometry->m_id;
	m_transform = worldTransform;
	m_audio->SetGeometry( m_geometrySetId, m_instanceId, audioGeometry->m_data, worldTransform );
}

void EveAudioOccluder::SetTransform( const Matrix& worldTransform )
{
	if( m_geometrySetId != 0 && m_transform != worldTransform )
	{
		m_transform = worldTransform;
		m_audio->SetGeometryTransform( m_geometrySetId, m_instanceId, worldTransform );
	}
}

void EveAudioOccluder::Release()
{
	if( m_geometrySetId != 0 )
	{
		m_audio->RemoveGeometry( m_geometrySetId, m_instanceId );
		m_geometrySetId = 0;
	}
	m_audio = nullptr;
}
