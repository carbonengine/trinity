// Copyright © 2023 CCP ehf.

#include "StdAfx.h"

#include "Tr2DxtCompressor.h"

const Be::VarChooser Tr2DxtCompressionChooser[] = {
	// Name									Value										Docstring
	{ "TR2DXT_COMPRESS_RT_DXT1", BeCast( TR2DXT_COMPRESS_RT_DXT1 ), "Real time dxt1 compression" },
	{ "TR2DXT_COMPRESS_RT_DXT5", BeCast( TR2DXT_COMPRESS_RT_DXT5 ), "Real time dxt5 compression" },
	{ "TR2DXT_COMPRESS_RT_DXT5N", BeCast( TR2DXT_COMPRESS_RT_DXT5N ), "Real time dxt5 compression for normal maps" },
	{ "TR2DXT_COMPRESS_RT_YCOCGDXT5", BeCast( TR2DXT_COMPRESS_RT_YCOCGDXT5 ), "High quality dxt5 compression" },
	{ "TR2DXT_COMPRESS_RT_3DC", BeCast( TR2DXT_COMPRESS_RT_3DC ), "Real time normal map compression" },
	{ "TR2DXT_COMPRESS_SQUISH_DXT1", BeCast( TR2DXT_COMPRESS_SQUISH_DXT1 ), "High quality squish dxt1 compression" },
	{ "TR2DXT_COMPRESS_SQUISH_DXT3", BeCast( TR2DXT_COMPRESS_SQUISH_DXT3 ), "High quality squish dxt3 compression" },
	{ "TR2DXT_COMPRESS_SQUISH_DXT5", BeCast( TR2DXT_COMPRESS_SQUISH_DXT5 ), "High quality squish dxt5 compression" },
	{ "TR2DXT_COMPRESS_SQUISH_KBC4", BeCast( TR2DXT_COMPRESS_SQUISH_KBC4 ), "High quality squish kbc4 compression" },
	{ "TR2DXT_COMPRESS_SQUISH_KBC5", BeCast( TR2DXT_COMPRESS_SQUISH_KBC5 ), "High quality squish kbc5 compression" },
	{ 0 }
};

const Be::VarChooser Tr2DxtCompressionSquishQualityChooser[] = {
	// Name									Value										Docstring
	{ "TR2DXT_COMPRESS_SQ_ITER_CLUSTER_FIT", BeCast( TR2DXT_COMPRESS_SQ_ITER_CLUSTER_FIT ), "Use a very slow but very high quality colour compressor." },
	{ "TR2DXT_COMPRESS_SQ_CLUSTER_FIT", BeCast( TR2DXT_COMPRESS_SQ_CLUSTER_FIT ), "Use a slow but high quality colour compressor." },
	{ "TR2DXT_COMPRESS_SQ_RANGE_FIT", BeCast( TR2DXT_COMPRESS_SQ_RANGE_FIT ), "Use a fast but low quality colour compressor." },
	{ 0 }
};


BLUE_REGISTER_ENUM( "COMPRESSION_FORMAT", Tr2DxtCompressionFormat, Tr2DxtCompressionChooser );
BLUE_REGISTER_ENUM( "COMPRESSION_SQUISH_QUALITY", Tr2DxtCompressionSquishQuality, Tr2DxtCompressionSquishQualityChooser );
