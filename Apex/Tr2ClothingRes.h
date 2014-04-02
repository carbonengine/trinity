#pragma once

#ifndef Tr2ClothingRes_h
#define Tr2ClothingRes_h

#if APEX_ENABLED

BLUE_DECLARE( Tr2ClothingRes );
namespace physx {
namespace apex {
class NxClothingAsset;
}
}

BLUE_CLASS( Tr2ClothingRes ):
     public BlueAsyncRes,
	 public ICacheable
{
public:
    EXPOSE_TO_BLUE();

	using BlueAsyncRes::Lock;
	using BlueAsyncRes::Unlock;

    Tr2ClothingRes( IRoot* lockobj = NULL );
	~Tr2ClothingRes();

	physx::apex::NxClothingAsset* GetAsset() const;

	typedef TrackableStdVector<unsigned int> BoneMap_t;
	void BindToRig( const std::string* boneList, unsigned int numBones, BoneMap_t& boneMap );

	//////////////////////////////////////////////////////////////////////////
	// ICacheable
	bool IsMemoryUsageKnown();
	size_t GetMemoryUsage();

	int GetMaximumSimulationBudget() const;
	int GetNumGraphicalLodLevels() const;
	int GetGraphicalLodValue( int lodLevel ) const;
	float GetBiggestMaxDistance() const;
	int GetNumBones() const;
	std::string GetBoneName( unsigned int ix ) const;


protected:
	bool DoOpenStream();
	void DoCloseStream();
	LoadingResult DoLoad();
	bool DoPrepare();

	LoadingResult InternalLoadFunction();

	void SafeReload();

protected:
	IBlueStreamPtr m_dataStream;
	physx::apex::NxClothingAsset* m_asset;
	uint32_t m_dataSize;
	std::string m_apexName;
};

TYPEDEF_BLUECLASS_WR_SHUTDOWN( Tr2ClothingRes );

#endif // APEX_ENABLED
#endif //Tr2ClothingRes_h
