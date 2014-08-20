////////////////////////////////////////////////////////////////////////////////
//
// Created:		August 2014
// Copyright:	CCP 2014
//

#pragma once
#ifndef Tr2Texture2dLodParameter_h
#define Tr2Texture2dLodParameter_h

#include "TriTexture2DParameter.h"

BLUE_DECLARE( Tr2LodResource );

BLUE_CLASS( Tr2Texture2dLodParameter ) : public TriTexture2DParameter
{
public:
	EXPOSE_TO_BLUE();

	// setup
	void SetLodResource( Tr2LodResourcePtr newLodResource );

	Tr2Texture2dLodParameter( IRoot* lockobj = nullptr );
	virtual TriTextureRes* GetResource() const;

protected:
	Tr2LodResourcePtr m_lodResource;
};

TYPEDEF_BLUECLASS( Tr2Texture2dLodParameter );

#endif // Tr2Texture2dLodParameter_h