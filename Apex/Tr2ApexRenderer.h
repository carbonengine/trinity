#pragma once
#ifndef Tr2ApexRenderer_h
#define Tr2ApexRenderer_h

#if APEX_ENABLED

extern class Tr2ApexRenderer& g_apexRenderer;
extern class physx::apex::NxUserRenderResourceManager* g_apexRenderResourceManager;

class ITriRenderBatchAccumulator;
class Tr2PerObjectData;
BLUE_DECLARE( Tr2Effect );

BLUE_DECLARE( Tr2Material );

class Tr2ApexRenderer : public physx::apex::NxUserRenderer
{
public:
	Tr2ApexRenderer();

	ITriRenderBatchAccumulator* GetAccumulator() const;
	void SetAccumulator( ITriRenderBatchAccumulator* val );
	const Tr2PerObjectData* GetPerObjectData() const;
	void SetPerObjectData(const Tr2PerObjectData* val);
	void SetEffect( Tr2Material* effect );
	void SetReversedEffect( Tr2Material* effect );
	void SetEffectApexLod( Tr2Material* effect );
	void SetReversedEffectApexLod( Tr2Material* effect );
	void SetDepth( unsigned int depth );
	unsigned int GetDepth() const;

	//////////////////////////////////////////////////////////////////////////
	// NxUserRenderer
	void renderResource( const physx::apex::NxApexRenderContext &context );

	Tr2EffectPtr m_skinnedVS;	// for use with LOD

private:
	ITriRenderBatchAccumulator* m_accumulator;
	const Tr2PerObjectData* m_perObjectData;
	Tr2MaterialPtr m_effect;
	// An effect to use with reversed triangle order rendering
	Tr2MaterialPtr m_reversedEffect;

	Tr2MaterialPtr m_effectApexLod;
	Tr2MaterialPtr m_reversedEffectApexLod;

        // Depth value used for sorting batches
	unsigned int m_depth;
};

#endif // APEX_ENABLED

#endif
