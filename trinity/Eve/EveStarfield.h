// Copyright © 2023 CCP ehf.

#pragma once
#ifndef EveStarfield_H
#define EveStarfield_H


#include "Tr2DeviceResource.h"

BLUE_DECLARE( EveStarfield );
BLUE_DECLARE( Tr2Effect );

class ITriRenderBatchAccumulator;
class Tr2PerObjectData;

class EveStarfield : public INotify,
					 public IInitialize,
					 public Tr2DeviceResource
{
public:
	EXPOSE_TO_BLUE();

	using IInitialize::Lock;
	using IInitialize::Unlock;

	EveStarfield( IRoot* lockobj = NULL );
	~EveStarfield();

	void GetBatches( ITriRenderBatchAccumulator* accumulator, const Tr2PerObjectData* perObjectData );

	Tr2Effect* GetEffect();
	void SetEffect( Tr2Effect* effect );

	void Update( Be::Time time );

	//////////////////////////////////////////////////////////////////////////////////////
	// IInitialize
	bool Initialize();

	//////////////////////////////////////////////////////////////////////////////////////
	// ITriDeviceResource
	void ReleaseResources( TriStorage s );

private:
	bool OnPrepareResources();

public:
	/////////////////////////////////////////////////////////////////////////////////////
	// INotify
	bool OnModified( Be::Var* val );

private:
	bool m_display;
	int32_t m_starCount;
	int32_t m_seed;
	float m_maxDistance;
	float m_minDistance;
	float m_minFlashRate;
	float m_maxFlashRate;
	float m_minFlashIntensity;

	bool m_dirty;

	Tr2EffectPtr m_effect;

	unsigned int m_vertexDeclHandle;
	unsigned int m_bytesPerVertex;
	unsigned int m_vertexCount;
	Tr2BufferAL m_vertexBuffer;
};

TYPEDEF_BLUECLASS( EveStarfield );

// inlines
inline Tr2Effect* EveStarfield::GetEffect()
{
	return m_effect;
}

inline void EveStarfield::SetEffect( Tr2Effect* effect )
{
	m_effect = effect;
}


#endif // EveStarfield_H