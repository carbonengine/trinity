// Copyright © 2013 CCP ehf.

#pragma once
#ifndef ITr2RenderContextEvents_H
#define ITr2RenderContextEvents_H

class Tr2PrimaryRenderContextAL;
class Tr2TextureAL;

// --------------------------------------------------------------------------------------
// Description:
//   A set of callbacks from Tr2RenderContextAL.
// --------------------------------------------------------------------------------------
struct ITr2RenderContextEvents
{
	virtual void OnContextCreated( Tr2PrimaryRenderContextAL& renderContext ) = 0;
};

#endif