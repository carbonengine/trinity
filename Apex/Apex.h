#pragma once
#ifndef Apex_h
#define Apex_h

BLUE_DECLARE( Tr2ClothingActor );
BLUE_DECLARE( BlueAsyncRes );
BLUE_DECLARE_VECTOR( Tr2ClothingActor );
class ITriRenderBatchAccumulator;
class Tr2PerObjectData;
struct ITr2InteriorSHLightingSolver;

BLUE_DECLARE( Tr2Apex );

namespace physx
{
	namespace apex
	{
		class NxApexSDK;
		class NxApexScene;
		class NxModule;
		class NxModuleClothing;
		class NxClothingMaterialLibrary;
		class NxClothingActor;
	};
};

namespace PLATFORM_ANALYZER
{
	class PlatformAnalyzer;
};

class NxPhysicsSDK;
class NxScene;

// Tr2Apex exposes various settings and debugging aids to Python
// for easy access in Jessica.
class Tr2Apex : public IRoot
{
public:
	Tr2Apex( IRoot* lockobj = 0 );
	~Tr2Apex(void);

	EXPOSE_TO_BLUE();

	bool GetIsVisualizationEnabled();
	void SetIsVisualizationEnabled( bool val );

	bool GetVisualizeCollisionShapes();
	void SetVisualizeCollisionShapes( bool val );

	bool GetRendererVerbose();
	void SetRendererVerbose( bool val );

	bool GetVisualizeOnly();
	void SetVisualizeOnly( bool val );

	bool GetClothSimParallel();
	void SetClothSimParallel( bool val );

	bool GetParallelPhysxMeshSkinning();
	void SetParallelPhysxMeshSkinning( bool val );

	bool GetParallelMeshMeshSkinning();
	void SetParallelMeshMeshSkinning( bool val );

	bool GetParallelCpuSkinning();
	void SetParallelCpuSkinning( bool val );

	void CoreDumpScene(void);
	void MemoryUsageReport(void);

	unsigned int GetNbActorsInPhysxScene();
	unsigned int GetNbStaticShapesInPhysxScene();
	unsigned int GetNbDynamicShapesInPhysxScene();
	unsigned int GetNbJointsInPhysxScene();
	unsigned int GetNbEffectorsInPhysxScene();

	bool IsClothSimInProgress();

	bool GetFallbackSkinning(void);
	void SetFallbackSkinning(bool val); 

	bool GetEnableLOD(void);
	void SetEnableLOD(bool val); 

	float GetLodBenefitValue(void);
	void SetLodBenefitValue(float v);

	NxPhysicsSDK					*GetPhysXSDK(void) const { return m_physXSDK; };
	physx::apex::NxApexSDK			*GetApexSDK(void) const { return m_apexSDK; };

	physx::apex::NxModuleClothing	*GetModuleClothing(void) const { return m_apexModuleClothing; };

	physx::apex::NxModule			*LoadModule(const char* moduleName);
	void							InitializeApex(NxPhysicsSDK* sdk,PLATFORM_ANALYZER::PlatformAnalyzer *platformAnalyzer);
	void							ShutdownApex(void);
	void							ApexGatherBatches( Tr2ClothingActorVector& clothMeshes, 
												ITriRenderBatchAccumulator* batches, 
												int batchType, 
												const Tr2PerObjectData* perObjectData, 
												ITr2InteriorSHLightingSolver* shSolver = NULL,
												unsigned int depth = 0xFFFFFFFF );
	unsigned int					ApexGetFrameDelay();
	bool							ApexSceneIsParallel();
	bool							ApexParallelPhysxMeshSkinning();
	bool							ApexGetParallelMeshMeshSkinning();
	bool							ApexGetParallelCpuSkinning();
	
	// Apex gets upset if we try to release an actor while the simulation is stepping.
	// If we see that (ApexIsClothSimInProgress), put the actor in a list of to-be-released
	// as soon as simulation stops.
	void							ApexDelayReleaseActor( physx::apex::NxApexActor* actor );
	// Reloading a resource (eg an apex asset) while the simulation is ticking means we can't
	// release the actor using this resource; which means that the actor will be pointing to
	// a dangling asset.  So delay a Reload until simulation is not in progress
	void							ApexDelayReload( BlueAsyncRes* resource );
	// Notify the delayed release/reload mechanism that an asset got deleted, so any actors using
	// it just got auto-deleted too.
	void							OnApexAssetReleased( physx::apex::NxApexAsset* asset );
	// Execute the delayed actions.
	void							PerformDelayedActions();

	void							LaunchApexParamEditor(class Tr2ApexEmitterActor *emitter);

	void							RenderDebugInfo();

private:
	NxPhysicsSDK					*m_physXSDK;
	physx::apex::NxApexSDK			*m_apexSDK;
	physx::apex::NxModuleClothing	*m_apexModuleClothing;
	physx::apex::NxModule			*m_frameworkLegacy;
	physx::apex::NxModule			*m_clothingLegacy;

	unsigned int					m_apexViewMatrixId;
	unsigned int					m_apexProjectionMatrixId;


	bool							s_isClothSimInProgress:1;
	bool							m_debugVisualize:1;
	bool							m_fallbackSkinning:1;
	bool							m_enableLod:1;
	float							m_lodBenefitValue;

};
TYPEDEF_BLUECLASS( Tr2Apex );


extern Tr2Apex *g_Tr2Apex;


#endif
