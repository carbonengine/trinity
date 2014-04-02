#pragma once
#ifndef Tr2SHProbeRes_h
#define Tr2SHProbeRes_h

namespace Geo
{
	class IGeoStream;
}

BLUE_CLASS( Tr2SHProbeRes ):
	public BlueAsyncRes,
	public ICacheable
{
public:
	EXPOSE_TO_BLUE();
	Tr2SHProbeRes( IRoot* lockobj = NULL );
	~Tr2SHProbeRes();

	//////////////////////////////////////////////////////////////////////////
	// ICacheable
	bool IsMemoryUsageKnown();
	size_t GetMemoryUsage();

	Enlighten::RadProbeSetCore* GetSHLightProbes();
	size_t GetAdditionalProbeSetCount() const;
	bool GetAdditionalProbeSet(size_t index, Enlighten::RadProbeSetCore* &set, int &resolutionX, int &resolutionY, int &resolutionZ, Matrix &transform);
	void ReleaseProbeSet();
	int GetXResolution();
	int GetYResolution();
	int GetZResolution();

	static bool WriteSHProbeSetToDisk( const Enlighten::RadProbeSetCore* probeSet, Geo::IGeoStream& stream, int xCount, int yCount, int zCount );

	// Version number stored in files
	static const Geo::u32 s_versionNumber;
protected:
	bool DoOpenStream();
	void DoCloseStream();
	LoadingResult DoLoad();
	bool DoPrepare();


protected:
	IBlueStreamPtr m_dataStream;
	void* m_data;
	uint32_t m_dataSize;
	unsigned m_probeSize;
	struct ProbeSet
	{
		Enlighten::RadProbeSetCore* m_probeSet;
		int	m_xRes;
		int m_yRes;
		int m_zRes;
		Matrix m_transform;
	};
	std::vector<ProbeSet> m_probeSets;
};

TYPEDEF_BLUECLASS_WR_SHUTDOWN( Tr2SHProbeRes );
#endif //Tr2SHProbeRes_h
