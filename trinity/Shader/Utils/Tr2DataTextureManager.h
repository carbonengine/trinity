////////////////////////////////////////////////////////////
//
//    Created:   October 2015
//    Copyright: CCP 2015
//

#pragma once
#ifndef Tr2DataTextureManager_H
#define Tr2DataTextureManager_H

BLUE_DECLARE( EveUpdateContext );
BLUE_DECLARE( Tr2TextureReference );

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
		uint32_t blockLength;
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
	int32_t RequestBlockData( const Vector4* headerData, uint32_t blockLength, const Vector4* blockData, float priority );

	// get texture offset
	int32_t GetTextureOffset( int32_t blockID ) const;

	// set variables to the variable store
	void SetVariables();
	
private:
	// general data
	BlueSharedString m_name;
	uint32_t m_textureWidth;
	uint32_t m_textureHeight;

	// the data
	int32_t m_blockDataNextIdx;
	std::map<int32_t, BlockData> m_blockData;
	std::map<float, int32_t> m_blockPriority;

	// the texture
	Tr2TextureReferencePtr m_dataTexture;
	std::map<int32_t, int32_t> m_dataTextureOffsets;

	// debug
	uint32_t m_maxBlockCount;
	uint32_t m_maxPixelCount;
};

TYPEDEF_BLUECLASS( Tr2DataTextureManager );

#endif