// Copyright © 2026 CCP ehf.

#pragma once

#include "Tr2GpuResourcePool.h"

class Tr2ProfileTimer;


BLUE_INTERFACE( ITr2RenderNode ) :
	public IRoot
{
	struct TempOutput
	{
		BlueSharedString name;
		Tr2GpuResourcePool::Texture texture;
	};
	template <typename T>
	struct Span
	{
		T* data = nullptr;
		size_t size = 0;

		T& operator[]( size_t index )
		{
			return data[index];
		}
		const T& operator[]( size_t index ) const
		{
			return data[index];
		}

		T* begin() const
		{
			return data;
		};
		T* end() const
		{
			return data + size;
		};
	};

	virtual bool Validate( const Span<const Tr2BitmapDimensions>& destDimensions, const Span<const BlueSharedString>& outputs, Be::Time realTime, Be::Time simTime ) = 0;
	virtual void Execute( const Span<const Tr2TextureAL>& destinations, const Span<TempOutput>& outputs, Be::Time realTime, Be::Time simTime, const Tr2ProfileTimer& rootTimer, Tr2RenderContext& renderContext ) = 0;
};

BLUE_DECLARE_IVECTOR( ITr2RenderNode );
