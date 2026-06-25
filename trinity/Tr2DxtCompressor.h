// Copyright © 2023 CCP ehf.

#pragma once
#ifndef tr2dxtcompressor_h
#define tr2dxtcompressor_h

enum Tr2DxtCompressionFormat
{
	TR2DXT_COMPRESS_RT_DXT1 = 0,
	TR2DXT_COMPRESS_RT_DXT5 = 1,
	TR2DXT_COMPRESS_RT_DXT5N = 2,
	TR2DXT_COMPRESS_RT_YCOCGDXT5 = 3,
	TR2DXT_COMPRESS_RT_3DC = 4,
	TR2DXT_COMPRESS_SQUISH_DXT1 = 5,
	TR2DXT_COMPRESS_SQUISH_DXT3 = 6,
	TR2DXT_COMPRESS_SQUISH_DXT5 = 7,
	TR2DXT_COMPRESS_SQUISH_KBC4 = 8,
	TR2DXT_COMPRESS_SQUISH_KBC5 = 9,

	TR2DXT_COMPRESS_COUNT
};

enum Tr2DxtCompressionSquishQuality
{
	TR2DXT_COMPRESS_SQ_ITER_CLUSTER_FIT = 0,
	TR2DXT_COMPRESS_SQ_CLUSTER_FIT = 1,
	TR2DXT_COMPRESS_SQ_RANGE_FIT = 2,

	TR2DXT_COMPRESS_SQ_COUNT
};

// --------------------------------------------------------------------------------------
// Description:
//   Tr2DxtCompressControl is a small class to control/inspect the process of asyncronous
//   texture compression. It allows to check if the compression is finish and also allows
//   canceling the compression in progress.
// See Also:
//   Tr2DxtCompressSurfaceAsync
// --------------------------------------------------------------------------------------
class Tr2DxtCompressControl
{
public:
	Tr2DxtCompressControl();

	void Cancel();
	bool IsDone() const;
	const bool& IsCanceling() const;
	void Done();

	friend bool Tr2DxtCompressSurfaceAsync( Tr2DxtCompressionFormat eCompressFmt,
											const unsigned char* inBuf,
											unsigned width,
											unsigned height,
											unsigned char* outBuf,
											size_t outputPitch,
											Tr2DxtCompressControl* control,
											int squishQuality );

private:
	// A flag to cancel compression
	bool m_cancel;
	// A flag indicating that compression is finished
	bool m_isDone;
	// Compression task ID
	CcpAtomic<uint32_t> m_id;
};

enum
{
	COMPRESS_SQUISH_QUALITY_DEFAULT = -1
};

bool Tr2DxtCompressSurface( Tr2DxtCompressionFormat eCompressFmt,
							const uint8_t* inBuf,
							unsigned width,
							unsigned height,
							uint8_t* outBuf,
							size_t outputPitch,
							volatile const bool& cancel,
							Tr2DxtCompressionSquishQuality squishQuality );

bool Tr2DxtCompressSurfaceAsync( Tr2DxtCompressionFormat eCompressFmt,
								 const uint8_t* inBuf,
								 unsigned width,
								 unsigned height,
								 uint8_t* outBuf,
								 size_t outputPitch,
								 Tr2DxtCompressControl* control,
								 int squishQuality );

#endif // tr2dxtcompressor_h