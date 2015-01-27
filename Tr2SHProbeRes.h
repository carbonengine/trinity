#pragma once
#ifndef Tr2SHProbeRes_h
#define Tr2SHProbeRes_h

BLUE_CLASS( Tr2SHProbeRes ):
	public BlueAsyncRes,
	public ICacheable
{
public:
	EXPOSE_TO_BLUE();
	Tr2SHProbeRes( IRoot* lockobj = NULL );

	//////////////////////////////////////////////////////////////////////////
	// ICacheable
	bool IsMemoryUsageKnown();
	size_t GetMemoryUsage();

	size_t GetProbeSetCount() const;
	bool GetProbeSet( size_t index, int &resolutionX, int &resolutionY, int &resolutionZ, Matrix &transform, const Matrix*& shData );
	void ReleaseProbeSet();

	static const uint32_t s_versionNumber;
protected:
	LoadingResult DoLoad();
	bool DoPrepare();


protected:
	CcpMallocBuffer m_data;
	struct ProbeSet
	{
		uint32_t m_xRes;
		uint32_t m_yRes;
		uint32_t m_zRes;
		Matrix m_transform;
		Matrix m_shData[1];
	};
	std::vector<ProbeSet*> m_probeSets;
};

TYPEDEF_BLUECLASS_WR_SHUTDOWN( Tr2SHProbeRes );
#endif
