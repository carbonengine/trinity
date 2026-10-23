// Copyright © 2026 CCP ehf.

#pragma once
#ifndef _Tr2ProceduralCube_H_
#define _Tr2ProceduralCube_H_

#include "Tr2ProceduralGeometryProvider.h"


struct Tr2ProceduralCubeBuilder : public Tr2ProceduralGeometryBuilder
{
	bool BuildCmf( cmf::MemoryAllocator& allocator, cmf::BufferManager& buffers, const std::atomic<bool>& cancel, cmf::Data& data ) override;

	float m_extent = .5f;
};


BLUE_CLASS_IMPL( Tr2ProceduralCube )
class Tr2ProceduralCube : public Tr2ProceduralGeometryProvider
{
public:
	EXPOSE_TO_BLUE();

	float GetExtent();
	void SetExtent( float extent );

protected:
	std::unique_ptr<Tr2ProceduralGeometryBuilder> CreateBuilder() const override;

private:
	float m_extent = .5f;
};

TYPEDEF_BLUECLASS_WR_SHUTDOWN( Tr2ProceduralCube );

#endif
