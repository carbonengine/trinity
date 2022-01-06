////////////////////////////////////////////////////////////
//
//    Created:   October 2014
//    Copyright: CCP 2014
//

#pragma once
#ifndef Tr2LightManager_H
#define Tr2LightManager_H

#include "Tr2DeviceResource.h"
#include "Tr2Variable.h"
#include "TriFrustum.h"
#include "Tr2AddSafeGrowableBuffer.h"

BLUE_DECLARE( Tr2Effect );
BLUE_DECLARE( Tr2GpuBuffer );
BLUE_DECLARE( Tr2GpuStructuredBuffer );


// --------------------------------------------------------------------------------------
// Description:
//   Manages light sources for tiled/clustered lighting. An object of this class does not
//   store lights permanently. Rather a user adds lights every frame and then the object
//   updates GPU buffers needed for tiled lighting. Normally the per-frame operations
//   for the light manager would look like:
//     lightManager.Clear();
//     lightManager.SetFrustum( frustum );
//     for each in scene.lights
//       lightManager.AddPointLight( each ); // this can be done in parallel
//     lightManager.UpdateLists();
//     renderYourStuff();
//  Light manager GPU buffers are accessed through variable store with names 
//  "LightBuffer" and "LightIndexBuffer".
// --------------------------------------------------------------------------------------
class Tr2LightManager: public Tr2DeviceResource
{
public:
	Tr2LightManager( const char* effectPath );
	~Tr2LightManager();

	struct PerLightData
	{
		Vector3 position;
		float radius;
		Vector3 color;
		float innerRadius;
		Vector3 direction;
		float innerAngle;
	};

	void Clear();
	void SetFrustum( const TriFrustum& frustum );
	void AddPointLight( const Vector3& position, float radius, const Color& color, float innerRadius=0.0f);
	void AddLight( PerLightData& data );
	ALResult UpdateLists( uint32_t msaaType, Tr2RenderContext& renderContext );
	void SetVariableStore();

	virtual void ReleaseResources( TriStorage s );

	static void ResetVariableStore();
	static Tr2LightManager* GetOrCreateInstance( const char* effectPath );
	static Tr2LightManager* GetInstance();
	static void DeleteInstance();
private:
	Tr2LightManager( const Tr2LightManager& );
	Tr2LightManager& operator=( const Tr2LightManager& );

	virtual bool OnPrepareResources();

	ALResult DoUpdateLists( uint32_t msaaType, Tr2RenderContext& renderContext );
	ALResult ClearLightIndices( Tr2RenderContext& renderContext );
	ALResult UpdateLightBuffer( Tr2RenderContext& renderContext );

	Tr2AddSafeGrowableBuffer<PerLightData> m_lightData;
	Tr2GpuStructuredBufferPtr m_lightBuffer;
	Tr2GpuStructuredBufferPtr m_indexBuffer;
	
	Tr2GpuStructuredBufferPtr m_emptyLightBuffer;
	Tr2GpuStructuredBufferPtr m_emptyIndexBuffer;

	Tr2GpuBufferPtr m_indexBufferCounter;
	Tr2ConstantBufferAL m_perFrameData;
	Tr2EffectPtr m_effect;
	Tr2Variable m_lightBufferVariable;
	Tr2Variable m_indexBufferVariable;
	TriFrustum m_frustum;
};

#endif
