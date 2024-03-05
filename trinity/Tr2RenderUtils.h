#pragma once
#ifndef Tr2RenderUtils_h_
#define Tr2RenderUtils_h_

// This file collects freestanding utility functions that do not rely on private state.
// Most of this was TriDevice/Tr2Renderer but works just fine with the public interfaces.
// So put those guys here to lighten up those classes.


// Screen quad vertex format
struct Tr2ScreenVertex
{
	Vector4 p; // position
	Vector2 t; // texture coordinate 1
};

// Sets up vertices for drawing a screen quad. Pass in an array with 4 verts and optionally
// width and height of the target. If 0 is given for width/height, the current render target
// is assumed. The verts are set up in clip space, with proper offsets applied to get perfect
// pixel/texel alignment.
void SetupScreenQuad(	Tr2ScreenVertex quad[4],
						const Vector2& tlTexCoord		= Vector2( 0.0f, 0.0f ), 
						const Vector2& brTexCoord		= Vector2( 1.0f, 1.0f ),
						const Vector2& tlVertexCoord	= Vector2( 0.0f, 0.0f ), 
						const Vector2& brVertexCoord	= Vector2( 1.0f, 1.0f )
						);

// Same as SetupScreenQuad except in camera space.
void SetupScreenQuadInCameraSpace( Tr2ScreenVertex quad[4], int width = 0, int height = 0 );




bool FillAndSetConstants(	Tr2ConstantBufferAL& buffer, 
							const void* const data, const size_t dataSize, 
							const unsigned constantTypeMask,
							const unsigned registerIndex, 
							Tr2RenderContext& renderContext );

// Helper function to take care of a repeating pattern:
// - if needed, create a constant buffer with this dataSize
// - lock it, copy the given data to it if OK, and unlock it again
// - set it to the context at registerIndex.
inline bool FillAndSetConstants(	Tr2ConstantBufferAL& buffer, 
							const void* const data, const size_t dataSize, 
							const Tr2RenderContextEnum::ShaderType constantType, 
							const unsigned registerIndex, 
							Tr2RenderContext& renderContext )
{ return FillAndSetConstants( buffer, data, dataSize, 1 << constantType, registerIndex, renderContext ); }

// Helper function to take care of a repeating pattern:
// - if needed, create a constant buffer with that can fit 'data'
// - lock it, copy the given data to it if OK, and unlock it again
// - set it to the context at registerIndex.
template<typename T>
bool FillAndSetConstants(	Tr2ConstantBufferAL& buffer, 
							const T& data,
							const Tr2RenderContextEnum::ShaderType constantType, 
							const unsigned registerIndex, 
							Tr2RenderContext& renderContext )
{ return FillAndSetConstants( buffer, &data, sizeof( data ), constantType, registerIndex, renderContext ); }

// inhibit this
template<typename T>
bool FillAndSetConstants(	Tr2ConstantBufferAL& buffer, 
							const T*& data,
							const Tr2RenderContextEnum::ShaderType constantType, 
							const unsigned registerIndex, 
							Tr2RenderContext& renderContext );

void SetConstants(			Tr2ConstantBufferAL& buffer, 
							const unsigned constantTypeMask, 
							const unsigned registerIndex, 
							Tr2RenderContext& renderContext );

	
#endif // Tr2RenderUtils_h_
