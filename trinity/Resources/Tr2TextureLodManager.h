////////////////////////////////////////////////////////////////////////////////
//
// Created:		February 2022
// Copyright:	CCP 2022
//

#pragma once


BLUE_DECLARE( TriTextureRes );


struct Tr2TextureLodUpdateRequest
{
	uint64_t frameNumber;
	int32_t mipChange;
	bool cachedInRam;
};


BLUE_CLASS( Tr2TextureLodManager ) :
	public IRoot, 
	public IBlueEvents
{
public:
	EXPOSE_TO_BLUE();

	static Tr2TextureLodManager& Instance();

	void RegisterTexture( TriTextureRes * texture );
	void UnregisterTexture( TriTextureRes * texture );

	bool CanUploadToGpu( const Tr2BitmapDimensions& desc ) const;
	bool NeedToTrimGpuTexture() const;
	bool NeedToTrimCpuTexture() const;
	void GpuTextureCreated( const Tr2BitmapDimensions& desc );
	void GpuTextureDestroyed( const Tr2BitmapDimensions& desc );
	void CpuTextureCreated( const Tr2BitmapDimensions& desc );
	void CpuTextureDestroyed( const Tr2BitmapDimensions& desc );

	struct Stats
	{
		Stats();

		size_t gpuMemoryUsed;
		size_t gpuMemoryAllocated;

		size_t cpuMemoryUsed;
		size_t cpuMemoryAllocated;

		size_t gpuUploadSize;
	};

	Stats GetStats() const;

	std::vector<TriTextureRes*> GetManagedTextures() const;

	void SetUseLowResVtaFilesSetting( bool setting );
	bool GetUseLowResVtaFilesSetting() const;

protected:
	Tr2TextureLodManager();

	void OnTick( Be::Time realTime, Be::Time simTime, void* cookie ) override;

private:
	CcpAtomic<uint32_t> m_gpuMemorySize;
	CcpAtomic<uint32_t> m_cpuMemorySize;

	std::vector<std::pair<TriTextureRes*, Tr2TextureLodUpdateRequest>> m_textures;
	Stats m_currentStats;
	bool m_lowDetailVtaFiles;
};
TYPEDEF_BLUECLASS( Tr2TextureLodManager );
