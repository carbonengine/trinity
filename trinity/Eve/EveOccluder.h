// Copyright © 2010 CCP ehf.

#pragma once
#ifndef EveOccluder_H
#define EveOccluder_H

#include "ITr2Renderable.h"
#include "TriRenderBatch.h"
#include "../Tr2DeviceResource.h"
#include "Eve/EveUpdateContext.h"

// forwards
class TriFrustum;
BLUE_DECLARE( EveTransform );
BLUE_DECLARE_VECTOR( EveTransform );
BLUE_DECLARE( Tr2GpuBuffer );
BLUE_DECLARE( Tr2Effect );


class Tr2OcclusionBuffer : public Tr2DeviceResource
{
public:
	using Offset = std::shared_ptr<uint32_t>;

	Tr2OcclusionBuffer();

	Offset AllocateOffset();
	void ProcessBuffer( Tr2RenderContext& renderContext );

	static Tr2OcclusionBuffer& GetInstance();

	static uint32_t GetOccluderOffset( const Offset& offset, uint32_t index );

protected:
	bool OnPrepareResources() override;
	void ReleaseResources( TriStorage s ) override;

private:
	void ResizeBuffer();
	static void DestroyOffset( uint32_t* offset );

	static const uint32_t ELEMENT_SIZE = 13;

	Tr2EffectPtr m_management;
	Tr2GpuBufferPtr m_buffer;
	std::vector<uint32_t> m_free;
	std::vector<uint32_t> m_clear;
	uint32_t m_size = 0;
};


// --------------------------------------------------------------------------------
// Description:
//   EveOccluder is used to do occlusion queries for suns and
//   lensflares. Basically it renders a sprite with and without
//   depth testing and the resulting ratio is the visible amount.
//   An EveOccluder can have multiple sprites, each result is
//   multiplied with the previous one to get a final value.
// SeeAlso:
//   EveLensflare
// --------------------------------------------------------------------------------
class EveOccluder : public IRoot
{
public:
	EXPOSE_TO_BLUE();

	EveOccluder( IRoot* lockobj = NULL );

	// do the occlusion rendering/querying
	void RunQuery( Tr2RenderContext& renderContext, const EveUpdateContext& updateContext, const Matrix& transform, uint32_t bufferOffset, float fogWeight );

private:
	// name
	std::string m_name;
	// toggle display
	bool m_display;

	// a vector with all the sprites this occluder is using
	PEveTransformVector m_sprites;

	// we have our own accumulator
	std::unique_ptr<TriRenderBatchAccumulator<EffectKeyGenerator>> m_batches;
};

TYPEDEF_BLUECLASS( EveOccluder );
BLUE_DECLARE_VECTOR( EveOccluder );

#endif // EveOccluder_H
