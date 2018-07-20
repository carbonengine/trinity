////////////////////////////////////////////////////////////
//
//    Created:   July 2018
//    Copyright: CCP 2018
//

#pragma once

BLUE_CLASS( Tr2CairoScriptSourceRes ) :
	public BlueAsyncRes,
	public ICacheable
{
public:
	EXPOSE_TO_BLUE();

	Tr2CairoScriptSourceRes( IRoot* lockobj = nullptr );

	virtual bool IsMemoryUsageKnown();
	virtual size_t GetMemoryUsage();

	uint32_t GetWidth() const;
	uint32_t GetHeight() const;
	const CcpMallocBuffer& GetContents() const;

	bool ApplyColor( std::string& dest, const Color& color ) const;
protected:
	virtual LoadingResult DoLoad();
	virtual bool DoPrepare();
private:
	CcpMallocBuffer m_contents;
	uint32_t m_width;
	uint32_t m_height;
};

TYPEDEF_BLUECLASS_WR_SHUTDOWN( Tr2CairoScriptSourceRes );
