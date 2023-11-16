////////////////////////////////////////////////////////////
//
//    Created:   March 2023
//    Copyright: CCP 2023
//

#pragma once
#include <VtaHandler.h>


BLUE_CLASS( Tr2TextureAnimation ) :
	public IInitialize,
	public INotify
{
public:
	EXPOSE_TO_BLUE();

	~Tr2TextureAnimation();

	bool Initialize() override;
	bool OnModified( Be::Var * value ) override;

	Tr2TextureAL GetTexture( const BlueSharedString& channel ) const;
	void AdvanceTime( float dt );
	void RestartAnimation();

	std::vector<BlueSharedString> GetChannelNames() const;

	bool UpdateOnlyWhenRendered() const;

private:
	struct MemoryReadStream : public ICcpStream
	{
		ptrdiff_t Read( void* dest, ptrdiff_t count ) override;
		ptrdiff_t Write( const void* source, size_t count ) override;
		ptrdiff_t Seek( ptrdiff_t distance, SeekOrigin method ) override;
		ptrdiff_t GetPosition() override;
		ptrdiff_t GetSize() override;

		std::vector<uint8_t> m_data;
		ptrdiff_t m_offset = 0;
	};
	struct Grid
	{
		BlueSharedString name;
		Tr2TextureAL frame;
	};
	struct AsyncState
	{
		MemoryReadStream stream;
		ImageIO::Vta::FileReader reader;
		std::vector<ImageIO::Vta::FrameDecoder> decoders;
		std::atomic<bool> cancel = false;
		std::atomic<bool> bitmapsReady = false;
	};
	struct AsyncRequest
	{
		std::wstring filename;
		std::shared_ptr<AsyncState> state;
	};
	enum RestartState
	{
		NotRestarting,
		WaitingToRestart,
		WaitingForFrame,
	};

	static void ReadFile( void* ctx );
	static void DecodeFirstFrame( void* ctx );
	static void DecodeNextFrame( void* ctx );
	static void RestartAndDecodeFrame( void* ctx );

	void ReadData();
	AsyncRequest* MakeRequest();
	void UpdateGrids();

	std::shared_ptr<AsyncState> m_asyncState;
	std::vector<Grid> m_grids;

	std::wstring m_filename;
	float m_fps = 1;
	float m_time = 0;
	uint32_t m_frame = 0;
	RestartState m_restartState = NotRestarting;
	bool m_paused = false;
	bool m_looped = true;
	bool m_updateOnlyWhenRendered = true;
};
TYPEDEF_BLUECLASS( Tr2TextureAnimation );
