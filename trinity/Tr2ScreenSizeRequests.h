// Copyright © 2026 CCP ehf.

#pragma once
#ifndef Tr2ScreenSizeRequests_H
#define Tr2ScreenSizeRequests_H

#include <atomic>
#include <limits>
#include <vector>

class TriTextureRes;

// The texture LOD requests one UseWithScreenSize walk produced, plus the screen-size range over which
// every request stays the same, so later frames can replay them instead of re-walking the materials.
struct Tr2ScreenSizeRequests
{
	struct Request
	{
		TriTextureRes* texture;
		uint32_t lod;
	};

	std::vector<Request> requests;
	float minScreenSize = 0.0f;
	float maxScreenSize = std::numeric_limits<float>::max();
	bool valid = true;

	void Reset()
	{
		requests.clear();
		minScreenSize = 0.0f;
		maxScreenSize = std::numeric_limits<float>::max();
		valid = true;
	}

	void Add( TriTextureRes* texture, uint32_t lod )
	{
		requests.push_back( { texture, lod } );
	}

	void Constrain( float lo, float hi )
	{
		minScreenSize = std::max( minScreenSize, lo );
		maxScreenSize = std::min( maxScreenSize, hi );
	}

	void Invalidate()
	{
		valid = false;
	}

	bool Covers( float screenSize ) const
	{
		return valid && screenSize >= minScreenSize && screenSize < maxScreenSize;
	}

	// Bumped whenever anything a cached request depends on changes (texture assigned/loaded/destroyed, LOD parameters).
	static uint32_t Generation()
	{
		return s_generation.load( std::memory_order_acquire );
	}

	static void BumpGeneration()
	{
		s_generation.fetch_add( 1, std::memory_order_acq_rel );
	}

private:
	inline static std::atomic<uint32_t> s_generation{ 1 };
};

#endif
