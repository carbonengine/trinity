#ifndef TR2PICKBUFFER_H
#define TR2PICKBUFFER_H

#include "Tr2DeviceResource.h"

// Forward declarations
class Tr2Effect;

class Tr2PickBuffer :
	public Tr2DeviceResource
{
public:
	Tr2PickBuffer( IRoot* lockobj = 0, Tr2RenderContextEnum::PixelFormat format = Tr2RenderContextEnum::PIXEL_FORMAT_UNKNOWN, int size = 1 );
	~Tr2PickBuffer();

	/////////////////////////////////////////////////////////////
	// ITriDeviceResource
	virtual void ReleaseResources( TriStorage s );
private:
	bool OnPrepareResources();
public:	
#ifdef TRINITYDEV
	void GetDescription( std::string& desc ) { desc = "Tr2PickBuffer"; }
#endif

	// Change picking behavior
	void SetSize( int size );
	unsigned int GetSize() const			{	return m_size;			}
	void SetClearColor( uint32_t val )			{	m_clearColor = val;		};

	// Rendering
	bool BeginRendering( float initialDepth, Tr2RenderContext& renderContext );
	bool EndRendering( Tr2RenderContext& renderContext );

	bool PrepareGetResults( const void*& data, uint32_t& pitch, Tr2RenderContext& renderContext );
	void UnlockBuffer( Tr2RenderContext& renderContext );

protected:
	int m_size;		            // Pickbuffer size
	Tr2RenderContextEnum::PixelFormat m_format;			// Pickbuffer format
	uint32_t m_clearColor;			// Pickbuffer "nothing there" color

	// Pick buffers
	Tr2TextureAL m_pickTarget;
	Tr2TextureAL m_depthBuffer;
};

#endif
