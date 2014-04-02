/* 
	*************************************************************************

	ITriEffectParameter.h

	Created:   June 2006
	OS:        Win32
	Project:   Trinity

	Description:   

		ITriEffectParameter is an interface allowing for a list of ID3DXEffect parameters like float4, float3, float and textures

	Dependencies:

		DirectX 9.0c, Probably more, ytbd.

	(c) CCP 2005

	*************************************************************************
*/

#ifndef ITr2EffectParameter_h
#define ITr2EffectParameter_h

#include "ITr2EffectValue.h"

// Forward declarations
class TriTextureRes;
struct ITr2ShaderState;

BLUE_INTERFACE(ITriEffectParameter) : public ITr2EffectValue
{
	// Only allow texture parameters to be mapped to samplers
	virtual bool IsTextureParameter() { return false; }

	virtual const char* GetParameterName() const = 0;

	virtual bool IsZeroOrNull( void ) const = 0;

	virtual void RebuildEffectHandles( ITr2ShaderState* effectRes ) = 0;

	// --------------------------------------------------------------------------------------
	// Description:
	//   Checks if the parameter is ready to be used. (For texture resource parameters
	//   this means that the texture is prepared)
	// Return value:
	//   true If the parameter is ready to be used
	//	 false Otherwise
	// --------------------------------------------------------------------------------------
	virtual bool IsPrepared() const { return true; }
};
BLUE_DECLARE_IVECTOR( ITriEffectParameter );
typedef BlueDict<ITriEffectParameter> ITriEffectParameterDict;
TYPEDEF_BLUECLASS( ITriEffectParameterDict );

BLUE_INTERFACE(ITriEffectResourceParameter) : public ITriEffectParameter
{

	virtual bool IsTextureParameter() { return true; }

	// Force a reload from disk
	virtual void ReloadResources() = 0; 
	
	// Kick off a load, in case resource has been unloaded
	virtual void LoadResources() 
	{ /*do nothing by default*/ }; 

	// Unload the resource
	virtual void UnloadResources() 
	{ /*do nothing by default*/ }; 

	virtual void* GetResourcePointer() const = 0;

	// Return the size of a texture pointer
	virtual size_t GetValueSize() const
	{
		return sizeof(void*);
	}
};
BLUE_DECLARE_IVECTOR( ITriEffectResourceParameter );

#endif