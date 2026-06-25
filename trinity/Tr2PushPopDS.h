// Copyright © 2023 CCP ehf.

#pragma once
#ifndef Tr2PushPopDS_H
#define Tr2PushPopDS_H

// -------------------------------------------------------------
// Description:
//  RAII helper for push+pop of a depthStencil.
struct Tr2PushPopDS
{
	// -------------------------------------------------------------
	// Description:
	// push/pop the current depthStencil, but don't change it.
	// Use the default main context.
	// -------------------------------------------------------------
	Tr2PushPopDS();

	// -------------------------------------------------------------
	// Description:
	// push the given depthStencil, pop the old one back when going out of scope.
	// -------------------------------------------------------------
	Tr2PushPopDS( const Tr2TextureAL& ds );

	// -------------------------------------------------------------
	// Description:
	// push/pop the current depthStencil on the given context, but don't change it.
	// -------------------------------------------------------------
	Tr2PushPopDS( Tr2RenderContext& renderContext );

	// -------------------------------------------------------------
	// Description:
	// push the given depthStencil, pop the old one back when going out of scope.
	// -------------------------------------------------------------
	Tr2PushPopDS( const Tr2TextureAL& ds, Tr2RenderContext& renderContext );

	~Tr2PushPopDS();

private:
	Tr2RenderContext* m_renderContext;

	Tr2PushPopDS( Tr2PushPopDS& );
	Tr2PushPopDS& operator=( Tr2PushPopDS& );
};

#endif