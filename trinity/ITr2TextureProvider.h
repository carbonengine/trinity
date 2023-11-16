#pragma once

#ifndef ITr2TextureProvider_h
#define ITr2TextureProvider_h

BLUE_INTERFACE( ITr2TextureProvider ) : public IRoot
{
	virtual Tr2TextureAL* GetTexture() = 0;
};

#endif
