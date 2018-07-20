////////////////////////////////////////////////////////////////////////////////
//
// Created:		August 2014
// Copyright:	CCP 2014
//

#pragma once
#ifndef Tr2Texture2dLodParameter_h
#define Tr2Texture2dLodParameter_h

#include "TriTextureParameter.h"

BLUE_DECLARE( Tr2LodResource );

BLUE_CLASS( Tr2Texture2dLodParameter ) : public TriTextureParameter
{
public:
	EXPOSE_TO_BLUE();

	// setup
	void SetLodResource( Tr2LodResource* newLodResource );

	Tr2Texture2dLodParameter( IRoot* lockobj = nullptr );
	virtual ITr2TextureProvider* GetResource() const;

protected:
	Tr2LodResourcePtr m_lodResource;
};

TYPEDEF_BLUECLASS( Tr2Texture2dLodParameter );

#endif // Tr2Texture2dLodParameter_h