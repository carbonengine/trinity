// Copyright © 2005 CCP ehf.

#ifndef ITr2EffectParameter_h
#define ITr2EffectParameter_h

#include "ITr2EffectValue.h"

BLUE_DECLARE( Tr2Shader );

BLUE_INTERFACE( ITriEffectParameter ) :
	public ITr2EffectValue
{
	virtual const char* GetParameterName() const = 0;

	virtual void RebuildEffectHandles( Tr2Shader * effectRes ) = 0;

	virtual unsigned GetHashValue( unsigned startingHash ) const = 0;

	virtual bool SupportsDirtyNotification() const
	{
		return false;
	}
};
BLUE_DECLARE_IVECTOR( ITriEffectParameter );
typedef BlueDict<ITriEffectParameter> ITriEffectParameterDict;
TYPEDEF_BLUECLASS( ITriEffectParameterDict );

BLUE_INTERFACE( ITriEffectResourceParameter ) :
	public ITriEffectParameter{
		virtual void OnAddedToMaterial( Tr2Material * material ){} virtual void OnRemovedFromMaterial( Tr2Material * material ){}
	};
BLUE_DECLARE_IVECTOR( ITriEffectResourceParameter );

BLUE_INTERFACE( ITriEffectTextureParameter ) :
	public ITriEffectResourceParameter
{
	static const size_t UV_SET_MAX_COUNT = 8;
	virtual void UsedWithScreenSize( float screenSize, float worldRadius, const std::vector<float>& uvDensities ) = 0;
	virtual void EnableTextureLoding( const std::array<float, UV_SET_MAX_COUNT>& uvDensityScale ) = 0;
	virtual void DisableTextureLoding() = 0;
};
BLUE_DECLARE_IVECTOR( ITriEffectTextureParameter );


#endif