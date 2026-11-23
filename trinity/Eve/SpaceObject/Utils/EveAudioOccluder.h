// Copyright © 2026 CCP ehf.

#pragma once

#include <ITr2AudGeometry.h>

BLUE_DECLARE( TriGeometryRes );

// Represents one instance of a mesh's audio geometry in carbon-audio and keeps its transform up to date.
class EveAudioOccluder
{
public:
	EveAudioOccluder();
	~EveAudioOccluder();

	EveAudioOccluder( EveAudioOccluder&& other ) noexcept;
	EveAudioOccluder& operator=( EveAudioOccluder&& other ) noexcept;
	EveAudioOccluder( const EveAudioOccluder& ) = delete;
	EveAudioOccluder& operator=( const EveAudioOccluder& ) = delete;

	// Registers the mesh once its audio geometry is loaded and registers it again when the resource reloads.
	// Releases when there is nothing to register. Reads the resource, so call it from the synchronous update.
	void Update( ITr2AudGeometry* audio, TriGeometryRes* geometry, unsigned int meshIndex, const Matrix& worldTransform );

	// Moves the registered instance. Does not touch the resource.
	void SetTransform( const Matrix& worldTransform );

	void Release();

	// Returns carbon-audio's geometry object, or null when carbon-audio is not loaded.
	static ITr2AudGeometryPtr CreateAudioGeometry();

private:
	ITr2AudGeometryPtr m_audio;
	uint64_t m_instanceId;
	// 0 while not registered
	uint64_t m_geometrySetId = 0;
	Matrix m_transform = IdentityMatrix();
};
