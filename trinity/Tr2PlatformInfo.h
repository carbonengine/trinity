// Copyright © 2019 CCP ehf.

#pragma once

BLUE_CLASS( Tr2PlatformInfo ) :
	public IRoot
{
public:
	EXPOSE_TO_BLUE();

	enum StaticCap
	{
		// low-level
		NON_SYNCHRONIZED_LOCKS,
		BUFFER_SHADER_RESOURCES,
		UNORDERED_ACCESS,
		COMPUTE,
		TEXTURE_ARRAYS,
		MSAA_SAMPLE,
		// application
		TAA,
	};

	std::string GetPlatformName() const;
	uint32_t GetPlatformID() const;

	bool GetStaticCap( StaticCap cap ) const;
	bool IsLowPerformance() const;
};

TYPEDEF_BLUECLASS( Tr2PlatformInfo );