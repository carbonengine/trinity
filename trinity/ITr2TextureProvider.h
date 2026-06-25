// Copyright © 2023 CCP ehf.

#pragma once

#ifndef ITr2TextureProvider_h
#define ITr2TextureProvider_h

#include "Tr2Event.h"


BLUE_INTERFACE( ITr2TextureProvider ) :
	public IRoot
{
	using OnTextureChangeEvent = Tr2Event<>;
	virtual Tr2TextureAL* GetTexture() = 0;
	virtual OnTextureChangeEvent& OnTextureChange() = 0;
};

#endif
