#pragma once

#ifndef Tr2LitPerObjectData_h
#define Tr2LitPerObjectData_h

#include "Tr2PerObjectData.h"

// --------------------------------------------------------------------------------------
// Description:
//   Per-object data for lit objects.
// --------------------------------------------------------------------------------------
class Tr2LitPerObjectData : public Tr2PerObjectDataStandard
{
public:
	// Constructor
	Tr2LitPerObjectData() : m_numPointLights( 0 ){}

	// --------------------------------------------------------------------------------------
	// Description:
	//   Updates constant buffer with per-object data for specified input stage (shader 
	//   type). Based on updateDestination parameter the function either updates buffer's
	//   mirror or commits data to the render context. This function is used for nested (for
	//   example per-area) data.
	//   Sets number of lights for pixel shader constant buffer.
	// Arguments:
	//   type - stage input type
	//   buffer - constant buffer to update
	//   updateDestination - what to update
	//   renderContext - current render context
	// --------------------------------------------------------------------------------------
	void UpdateConstantBuffer( Tr2RenderContextEnum::ShaderType type, 
							   Tr2ConstantBufferAL& buffer, 
							   UpdateDestination updateDestination,
							   unsigned constantTypeMask, 
							   Tr2RenderContext& renderContext ) const
	{
		Tr2PerObjectDataStandard::UpdateConstantBuffer( type, buffer, updateDestination, constantTypeMask, renderContext );
		if( type == Tr2RenderContextEnum::PIXEL_SHADER )
		{
			renderContext.SetNumberOfLights( m_numPointLights );
		}
	}

	// ----------------------------------------------------------------------------------
	// Description:
	//   Sets the number of active point and spot lights.
	// Arguments:
	//   numPointLights - The number of active point lights
	//   numSpotLights - The number of active spot lights
	// ----------------------------------------------------------------------------------
	void SetLightsActive( unsigned int numPointLights, unsigned int numSpotLights )
	{
		m_numPointLights = numPointLights;
	}

private:
	// Number of point lights
	int m_numPointLights;
	// Padding
	int m_padding[3];
};

// --------------------------------------------------------------------------------------
// Description:
//   Per-object data for skinned, lit objects
// --------------------------------------------------------------------------------------
class Tr2LitPerObjectDataSkinned : public Tr2PerObjectDataSkinned
{
public:
	// Constructor
	Tr2LitPerObjectDataSkinned() : m_numPointLights( 0 ) {}

	// --------------------------------------------------------------------------------------
	// Description:
	//   Updates constant buffer with per-object data for specified input stage (shader 
	//   type). Based on updateDestination parameter the function either updates buffer's
	//   mirror or commits data to the render context. This function is used for nested (for
	//   example per-area) data.
	//   Sets number of lights for pixel shader constant buffer.
	// Arguments:
	//   type - stage input type
	//   buffer - constant buffer to update
	//   updateDestination - what to update
	//   renderContext - current render context
	// --------------------------------------------------------------------------------------
	void UpdateConstantBuffer( Tr2RenderContextEnum::ShaderType type, 
							   Tr2ConstantBufferAL& buffer, 
							   UpdateDestination updateDestination,
							   unsigned constantTypeMask, 
							   Tr2RenderContext& renderContext ) const
	{
		Tr2PerObjectDataSkinned::UpdateConstantBuffer( type, buffer, updateDestination, constantTypeMask, renderContext );
		if( type == Tr2RenderContextEnum::PIXEL_SHADER )
		{
			renderContext.SetNumberOfLights( m_numPointLights );
		}
	}

	// ----------------------------------------------------------------------------------
	// Description:
	//   Sets the number of active point and spot lights.
	// Arguments:
	//   numPointLights - The number of active point lights
	//   numSpotLights - The number of active spot lights
	// ----------------------------------------------------------------------------------
	void SetLightsActive( unsigned int numPointLights, unsigned int numSpotLights )
	{
		m_numPointLights = numPointLights;
	}

private:
	// Number of point lights
	int m_numPointLights;
	// Padding
	int m_padding[3];
};

// --------------------------------------------------------------------------------------
// Description:
//   Per-object data for skinned objects rendered in light-prepass scene
// --------------------------------------------------------------------------------------
class Tr2PerObjectDataPrePassSkinned : public Tr2PerObjectDataSkinned
{
public:
	// Constructor
	Tr2PerObjectDataPrePassSkinned() {}
};

#endif
