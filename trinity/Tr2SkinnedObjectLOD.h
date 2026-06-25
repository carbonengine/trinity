// Copyright © 2023 CCP ehf.

#pragma once
#ifndef Tr2SkinnedObjectLod_h
#define Tr2SkinnedObjectLod_h

class Tr2SkinnedModel;
class TriFrustum;

//BLUE_DECLARE( Tr2SkinnedObjectLod );
//BLUE_DECLARE_VECTOR( Tr2SkinnedObjectLod );

// Helper class to centralize all the LOD data and decision making, for easier sharing
// between wod internal, external, and eve use cases.
class Tr2SkinnedObjectLod
{
public:
	//EXPOSE_TO_BLUE();

	//using IInitialize::Lock;
	//using IInitialize::Unlock;

	Tr2SkinnedObjectLod( /*IRoot* lockobj = NULL*/ );
	~Tr2SkinnedObjectLod();

	/////////////////////////////////////////////////////////////////////////////////////
	// INotify
	bool /*BLUEAPI*/ OnModified( Be::Var* val );

	Tr2SkinnedModel* SetLOD( const TriFrustum* frustum, float estimatedPixelDiameter );
	// note: these are legacy from the old LOD in WoD, they actually create a proxy if there isn't one.
	void SetHighDetailModel( Tr2SkinnedModel* model );
	void SetMediumDetailModel( Tr2SkinnedModel* model );
	void SetLowDetailModel( Tr2SkinnedModel* model );
	/// \param time current time
	/// \param deltaTime time since the last update; used to avoid unloading data due to framerate spikes / freezing up, where unload/load could only make it worse.
	/// \return true if the current lod was being unloaded, and the user should clear its visual model pointer
	bool UnloadLodIfNeeded( Be::Time time, Be::Time deltaTime );

	// SetLOD computes a best fit; use SetCurrentLod to just set a number directly. Used for cloth trickery.
	void SetCurrentLod( int lod )
	{
		m_currentLod = lod;
	}
	int GetCurrentLod() const
	{
		return m_currentLod;
	}
	bool HaveLodSetup() const
	{
		return m_allowLodSelection;
	}

	void PopulateLods();

	// for pointlights
	bool IsCastingShadow() const;

	// maxClothLod: maximum lod, inclusive, that should run simulated physics cloth
	bool IsSimulatingCloth( const int maxClothLod ) const;

	void OnModelChanged( Tr2SkinnedModel* model );

private:
	Tr2SkinnedModel* GetHighDetailModel();
	Tr2SkinnedModel* GetMediumDetailModel();
	Tr2SkinnedModel* GetLowDetailModel();

	// LOD selection ok? only true if at least one model/proxy is available
	bool m_allowLodSelection;
	//
	int m_currentLod;

public: // public for blue exposure in the wrapper
	IBlueObjectProxyPtr m_highDetailProxy;
	IBlueObjectProxyPtr m_lowDetailProxy;
	IBlueObjectProxyPtr m_mediumDetailProxy;
};

//TYPEDEF_BLUECLASS( Tr2SkinnedObjectLod );

#endif // Tr2SkinnedObjectLod_h
