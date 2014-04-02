////////////////////////////////////////////////////////////
//
//    Created:   May 2010
//    Copyright: CCP 2010
//

#pragma once
#ifndef Tr2InteriorEnlightenSystemImpl_H
#define Tr2InteriorEnlightenSystemImpl_H

#include "Tr2InteriorDusterCache.h"

BLUE_DECLARE( Tr2RadiosityRes );
BLUE_DECLARE( TriGeometryRes );
class TriEnlightenProgressBar;

// -------------------------------------------------------------
// Description:
//   ITr2InteriorEnlightenGeometryProvider is used by
//   Tr2InteriorEnlightenSystem class to provide a list 
//   packed geometries when packing an Enlighten system.
// SeeAlso:
//   Tr2InteriorEnlightenSystem, Tr2InteriorCell
// -------------------------------------------------------------
class ITr2InteriorEnlightenGeometryProvider
{
public:
#if defined(ENLIGHTEN_PRECOMPUTE_ENABLED)
	// Fill a list of packed geometries for the system
	virtual bool GetEnlightenPackedGeometry( Enlighten::IPrecompute* precompute, 
											 Enlighten::IPrecompInputSystem* inputSystem, 
											 TriEnlightenProgressBar& prog, 
											 float outputPixelSize, 
											 std::vector<Enlighten::IPrecompPackedGeometry*>& geometries ) = 0;
#endif
};

// -------------------------------------------------------------
// Description:
//   Tr2InteriorEnlightenSystem class encapsulates basic
//   functionality of an Enlighten system: building, working
//   with Tr2RadiosityRes. It is used by Tr2InteriorEnlightenSystem and
//   Tr2InteriorCell.
// SeeAlso:
//   Tr2InteriorEnlightenSystem, Tr2InteriorCell
// -------------------------------------------------------------
class Tr2InteriorEnlightenSystemImpl : public IBlueAsyncResNotifyTarget
{
public:
	Tr2InteriorEnlightenSystemImpl();
	~Tr2InteriorEnlightenSystemImpl();

	// Since this class is not exposed to Blue these methods need to
	// be called explicitly by owner objects
	void Initialize();
	void OnModified( Be::Var* value );

	//////////////////////////////////////////////////////////////////////////
	// IBlueAsyncResNotifyTarget
	void ReleaseCachedData( BlueAsyncRes* p );
	void RebuildCachedData( BlueAsyncRes* p );

#if defined(ENLIGHTEN_PRECOMPUTE_ENABLED)
	bool SaveEnlightenSystem( const std::vector<const TriGeometryRes*>& geometries );

	// Building
	enum Quality
	{
		// Low-quality fast Enlighten build
		QUALITY_PREVIEW,
		// Full-quality Enlighten build
		QUALITY_RELEASE,
	};

	bool PackEnlightenSystem( Quality quality,
							  Enlighten::IPrecompute* pPrecompute, 
		                      TriEnlightenProgressBar& prog, 
							  unsigned int systemInCellIdx, 
							  ITr2InteriorEnlightenGeometryProvider* geometryProvider );
	void BuildPreClustering( Enlighten::IPrecompute* pPrecompute, 
							 std::vector< Enlighten::IPrecompPackedSystem* >& neighbours, 
							 TriEnlightenProgressBar& prog );
	void BuildClustering( Enlighten::IPrecompute* pPrecompute, 
						  std::vector< Enlighten::IPrecompSystemPreClustering* >& neighbours, 
						  TriEnlightenProgressBar& prog );
	void BuildEnlightenSystem( Enlighten::IPrecompute* pPrecompute, 
							   std::vector< Enlighten::IPrecompPackedSystem* >& neighbours, 
							   std::vector< Enlighten::IPrecompSystemClustering* >& systemClusters, 
							   TriEnlightenProgressBar& prog );
	void DeletePrecompData();

	Enlighten::IPrecompPackedSystem* GetPackedSystem() const;
	Enlighten::IPrecompSystemPreClustering* GetSystemPreClustering() const;
	Enlighten::IPrecompSystemClustering* GetSystemClustering() const;
#endif

	Enlighten::InputWorkspace* GetEnlightenWorkspace() const;
	Enlighten::ClusterAlbedoWorkspace* GetAlbedoWorkspace() const;
	Enlighten::ClusterAlbedoWorkspaceMaterialData* GetAlbedoWorkspaceData() const;
	const Enlighten::RadSystemCore* GetRadSystem() const;

	Enlighten::InputLightingBuffer* GetCurrentInputLightingBuffer() const;
	Enlighten::InputLightingBuffer* GetNextInputLightingBuffer() const;

	void* GetBounceData() const;
	Geo::u32* GetAlbedoTexture() const;
	Geo::u32* GetEmissiveTexture() const;
	bool GetAlbedoTextureSize( int& width, int& height ) const;

	void EndInputWorkspace();

	const Enlighten::InputLightingBuffer** GetInputLightingList();

	Tr2InteriorDusterCache *GetDusterCache();

	void SetRadNotificationTarget( IBlueAsyncResNotifyTarget* notificationTarget );

	// Path to .rad file
	std::string m_radResPath;
	// Rad resource
	Tr2RadiosityResPtr m_radSystemResource;
	// Unique system ID
	unsigned int m_systemID;
	// Index of the system in the cell
	unsigned m_systemInCellIdx;
	// Pixel size in meters
	float m_enlightenPixelSize;

#if defined(ENLIGHTEN_PRECOMPUTE_ENABLED)
	// For chart visualisation
	bool CaptureChartMapping();
#endif
	int GetChartIndex( int x, int y ) const;
	int GetChartWidth() const { return m_debugChartMapWidth; }
	int GetChartHeight() const { return m_debugChartMapHeight; }

private:
	void ReleaseWorkspace();
	void ReleaseRadCore();
	void SetRadiosityResource();
	void CreateEnlightenWorkspaces();

	// Flag to indicate Enlighten resources were build (as opposed to loaded) and need to be deleted
	bool m_radBuiltInternally;
	// Flag to indicate that the last build was a preview buid
	bool m_previewBuild;

	// Enlighten Data Structures
	Enlighten::ClusterAlbedoWorkspace* m_albedoWorkspace;
	Enlighten::InputWorkspace* m_inputWorkspace;
	Enlighten::ClusterAlbedoWorkspaceMaterialData* m_albedoWorkspaceData;
	Enlighten::RadSystemCore* m_radSystem;

	Enlighten::InputLightingBuffer* m_inputLightingCurrentFrame;
	Enlighten::InputLightingBuffer* m_inputLightingNextFrame;

	// Bounce data memory for Enlighten solver
	void* m_bounceData;
	// Enlighten albedo texture
	Geo::u32* m_albedoTexture;
	// Enlighten emissive texture
	Geo::u32* m_emissiveTexture;

	const Enlighten::InputLightingBuffer** m_inputLightingList;

	// Duster values cache (position, normal, albedo per duster)
	Tr2InteriorDusterCache m_dusterCache;

	// An external notification target for m_radSystemResource
	IBlueAsyncResNotifyTarget* m_notificationTarget;

#if defined(ENLIGHTEN_PRECOMPUTE_ENABLED)
	// Enlighten stuff used during various stages of precompute
	Geo::GeoAutoReleasePtr<Enlighten::IPrecompPackedSystem> m_packedSystem;
	Geo::GeoAutoReleasePtr<Enlighten::IPrecompSystemPreClustering> m_systemPreClustering;
	Geo::GeoAutoReleasePtr<Enlighten::IPrecompSystemClustering> m_systemClustering;
#endif

	// Enlighten chart map
	int *m_debugChartMap;
	int m_debugChartMapHeight;
	int m_debugChartMapWidth;
};

#endif // Tr2InteriorEnlightenSystem_H