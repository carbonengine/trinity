#pragma once
#ifndef Tr2PushPopRT_H
#define Tr2PushPopRT_H

// -------------------------------------------------------------
// Description:
// RAII for push+pop of a renderTarget
// -------------------------------------------------------------
struct Tr2PushPopRT
{
	// -------------------------------------------------------------
	// Description:
	// push/pop the current renderTarget, but don't change it.
	// uses the main render context.
	// -------------------------------------------------------------
	Tr2PushPopRT( uint32_t slot = 0 );

	// -------------------------------------------------------------
	// Description:
	// push the given renderTarget, pop when going out of scope.
	// -------------------------------------------------------------
	Tr2PushPopRT( const Tr2TextureAL& rt, uint32_t slot = 0 );

	// -------------------------------------------------------------
	// Description:
	// push/pop the current renderTarget, but don't change it
	// -------------------------------------------------------------
	Tr2PushPopRT( Tr2RenderContext& renderContext, uint32_t slot = 0 );

	// -------------------------------------------------------------
	// Description:
	// push the given renderTarget, pop when out of scope.
	// -------------------------------------------------------------
	Tr2PushPopRT( const Tr2TextureAL& rt, Tr2RenderContext& renderContext, uint32_t slot = 0 );

	~Tr2PushPopRT();

private:
	Tr2RenderContext* m_renderContext;
	uint32_t	m_slot;

	Tr2PushPopRT( Tr2PushPopRT& );
	Tr2PushPopRT& operator=( Tr2PushPopRT& );
};

#endif