////////////////////////////////////////////////////////////
//
//    Created:   November 2010
//    Copyright: CCP 2010
//

#pragma once
#ifndef Tr2InteriorSHLightingSolver_H
#define Tr2InteriorSHLightingSolver_H

#include "include/ITr2Interior.h"
#include "Tr2DeviceResource.h"

// -------------------------------------------------------------
// Description:
//   Tr2InteriorSHLightingSolver computes SH lighting coefficients
//   for transparent areas provided. Its Solve method performs
//   a render pass similar to lighting pass to compute 
//   SH coefficients on GPU.
// -------------------------------------------------------------
class Tr2InteriorSHLightingSolver: 
	public ITr2InteriorSHLightingSolver,
	public Tr2DeviceResource
{
public:
	Tr2InteriorSHLightingSolver();
	~Tr2InteriorSHLightingSolver();

	void AddVolume( const Vector3& min, const Vector3& max, const Matrix& transform, Tr2PerObjectDataPSBuffer* perAreaData );
	void Solve( const ITr2InteriorLightVector& visibleLights, Tr2RenderContext& renderContext );
	void Clear();

	void ReleaseResources( TriStorage s );
private:
	// -------------------------------------------------------------
	// Description:
	//   Data for an area that requires SH lighting computed.
	// -------------------------------------------------------------
	struct SampleData
	{
		// Min bounds of area in local space
		Vector3 min;
		// Max bounds of area in local space
		Vector3 max;
		// Transform from local area space to world space
		Matrix transform;
		// Per-area data that receives SH lighting data
		Tr2PerObjectDataPSBuffer* perAreaData;
	};

	bool OnPrepareResources();

	static void FillPixel( int x, int y, void* data, unsigned pitch, const Vector3& point, int shIndex );
	static void FillCoefficients( int x, int y, void* data, unsigned pitch, const Vector3& point, int xIncrement );

	// Data for areas that require SH lighting
	std::vector<SampleData> m_samples;
	// Texture containing probe positions
	Tr2TextureAL m_sampleTexture;
	// Resulting texture containing SH coefficients
	Tr2RenderTargetAL m_shTexture;
	// Render batch accumulator for light sources
	ITriRenderBatchAccumulator* m_shRenderBatches;

	CcpMallocBuffer m_sampleTextureMirror;

	// Sample and SH texture resolution
	unsigned int m_textureSize;
	// Number of pixels in textures per 1 sample
	static const unsigned int PIXELS_PER_SAMPLE;
};

#endif // Tr2InteriorSHLightingSolver_H