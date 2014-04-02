#pragma once

#ifndef Tr2ApexRes_h
#define Tr2ApexRes_h

#if APEX_ENABLED

BLUE_DECLARE( Tr2ApexRes );
namespace physx {
namespace apex {
class NxApexAsset;
}
}

BLUE_CLASS( Tr2ApexRes ):
	public BlueAsyncRes,
	public ICacheable
{
public:
    EXPOSE_TO_BLUE();

	using BlueAsyncRes::Lock;
	using BlueAsyncRes::Unlock;

	Tr2ApexRes( IRoot* lockobj = NULL );
	~Tr2ApexRes();

	physx::apex::NxApexAsset* GetAsset() const;

	//////////////////////////////////////////////////////////////////////////
	// ICacheable
	bool IsMemoryUsageKnown();
	size_t GetMemoryUsage();

protected:
	bool DoOpenStream();
	void DoCloseStream();
	LoadingResult DoLoad();
	bool DoPrepare();

protected:
	IBlueStreamPtr				m_dataStream;
	physx::apex::NxApexAsset	*m_asset;
	uint32_t					m_dataSize;
	std::string					m_apexName;
};

TYPEDEF_BLUECLASS_WR_SHUTDOWN( Tr2ApexRes );

#endif // APEX_ENABLED

#endif

