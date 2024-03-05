////////////////////////////////////////////////////////////
//
//    Created:   February 2023
//    Copyright: CCP 2023
//

#pragma once

class TriFrustum;
class ITriRenderBatchAccumulator;

#include "Eve/EveComponentRegistry.h"

enum class Tr2VolumerticQuality
{
	Low,
	Medium,
	High,
	Ultra,
};
extern const Be::VarChooser Tr2VolumetricQuality_Chooser[];

BLUE_INTERFACE( ITr2VolumetricRenderable ) :
	public IRoot
{
public:
	struct SceneInformation
	{
		static const size_t depthSliceCount = 4;
		Tr2VolumerticQuality quality;
		float depthSlices[depthSliceCount];
		uint32_t targetWidth;
		uint32_t targetHeight;
		Vector3 sunDirection;
		bool receiveShadows;
		bool castShadows;
	};

	virtual float GetSortValue( const TriFrustum& frustum ) = 0;
	virtual void GetVolumetricBatches( const TriFrustum& frustum, ITriRenderBatchAccumulator* batches ) = 0;
	virtual bool UpdateVolumetricLightmap( Tr2RenderContext & renderContext ) = 0;
	virtual void SetSceneInformation( const SceneInformation& sceneInformation ) = 0;
	virtual void GetVolumetricShadowBatches( ITriRenderBatchAccumulator * batches ) = 0;
};

REGISTER_COMPONENT_TYPE( "VolumetricRenderable", ITr2VolumetricRenderable )