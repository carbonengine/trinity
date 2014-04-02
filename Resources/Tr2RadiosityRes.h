#pragma once
#ifndef Tr2RadiosityRes_h
#define Tr2RadiosityRes_h

// Forward declarations
namespace Enlighten
{
	class RadSystemCore;
	class InputWorkspace;
	class ClusterAlbedoWorkspaceMaterialData;
}

class TriGeometryRes;

BLUE_CLASS( Tr2RadiosityRes ):
	public BlueAsyncRes,
	public ICacheable
{
public:
	EXPOSE_TO_BLUE();
	Tr2RadiosityRes( IRoot* lockobj = NULL );
	~Tr2RadiosityRes();

	//////////////////////////////////////////////////////////////////////////
	// ICacheable
	bool IsMemoryUsageKnown();
	size_t GetMemoryUsage();

	Enlighten::RadSystemCore* GetSystemRadiosity();
	Enlighten::InputWorkspace* GetInputWorkspace() const;
	Enlighten::ClusterAlbedoWorkspaceMaterialData* GetAlbedoWorkspaceData() const;
	void GetDebugChartMap( int*& chartMap, int& width, int& height ) const;

	static bool WriteWorkspaceResourceHashesToDisk( const std::string& filename, 
													Enlighten::RadSystemCore* sysCore,
													Enlighten::InputWorkspace* inputWorkspace, 
													Enlighten::ClusterAlbedoWorkspaceMaterialData* albedoWorkspaceData, 
													const std::vector<const TriGeometryRes*>& geometries,
													const int chartWidth,
													const int chartHeight,
													const int *const chartData );

	typedef std::pair< unsigned int, unsigned int > uintPairType;
	typedef std::pair< std::wstring, uintPairType > geometryHashType;
	typedef std::vector< geometryHashType > geometryHashVectorType;
	
	geometryHashVectorType m_geometryHashes;

	// Version number stored in files
	static const Geo::u32 s_versionNumber;

protected:
	bool DoOpenStream();
	void DoCloseStream();
	LoadingResult DoLoad();
	bool DoPrepare();

	IBlueStreamPtr m_dataStream;
	void* m_data;
	uint32_t m_dataSize;
	unsigned m_systemSize;

	// Can't use GeoAutoPtr, because they leak (yes, you read that right)
	Enlighten::RadSystemCore* m_radSystem;
	Enlighten::InputWorkspace* m_inputWorkspace;
	Enlighten::ClusterAlbedoWorkspaceMaterialData* m_albedoWorkspaceData;

	int m_debugChartMapWidth;
	int m_debugChartMapHeight;
	int *m_debugChartMap;
};

TYPEDEF_BLUECLASS_WR_SHUTDOWN( Tr2RadiosityRes );
#endif //Tr2RadiosityRes_h
