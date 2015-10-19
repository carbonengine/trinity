////////////////////////////////////////////////////////////
//
//    Created:   October 2015
//    Copyright: CCP 2015
//

#pragma once
#ifndef Tr2DataTextureManager_H
#define Tr2DataTextureManager_H

BLUE_DECLARE( EveUpdateContext );

BLUE_CLASS( Tr2DataTextureManager ) :
	public IInitialize,
	public Tr2DeviceResource
{
public:
	EXPOSE_TO_BLUE();

	// block data
	struct BlockData
	{
		std::vector<Vector4> header;
		size_t blockLength;
		std::vector<Vector4> data;
	};

	Tr2DataTextureManager( IRoot* lockobj = NULL );
	~Tr2DataTextureManager();

	//////////////////////////////////////////////////////////////////////////////////////
	// IInitialize
	bool Initialize();

	/////////////////////////////////////////////////////////////
	// Tr2DeviceResource
	virtual void ReleaseResources( TriStorage s );
private:
	bool OnPrepareResources();
public:
#if TRINITYDEV
	void GetDescription( std::string& desc );
#endif

	/////////////////////////////////////////////////////////////////////////////////////
	// Update
	void Update( EveUpdateContext& updateContext );

	// access block IDs
	int32_t requestBlockData( const Vector4* headerData, size_t blockLength, const Vector4* blockData );
	
private:
	// general data
	BlueSharedString m_name;
	uint32_t m_maxDataSize;
	uint32_t m_textureHeight;

	// the data
	int32_t m_blockDataNextIdx;
	std::map<int32_t, BlockData> m_blockData;

	// the texture
	Tr2TextureAL m_dataTexture;
};

TYPEDEF_BLUECLASS( Tr2DataTextureManager );

#endif