// Copyright © 2026 CCP ehf.

#pragma once
#ifndef Tr2ProceduralSierpinski_h
#define Tr2ProceduralSierpinski_h

#include "Tr2ProceduralGeometryProvider.h"


struct Tr2ProceduralSierpinskiBuilder : public Tr2ProceduralGeometryBuilder
{
	bool BuildCmf( cmf::MemoryAllocator& allocator, cmf::BufferManager& buffers, const std::atomic<bool>& cancel, cmf::Data& data ) override;

	float m_size = 1.f;
	int32_t m_depth = 1;
};


BLUE_CLASS_IMPL( Tr2ProceduralSierpinski )
class Tr2ProceduralSierpinski : public Tr2ProceduralGeometryProvider
{
public:
	EXPOSE_TO_BLUE();

	float GetSize();
	void SetSize( float size );
	int32_t GetDepth();
	void SetDepth( int32_t depth );

protected:
	std::unique_ptr<Tr2ProceduralGeometryBuilder> CreateBuilder() const override;

private:
	float m_size = 1.f;
	int32_t m_depth = 1;
};

TYPEDEF_BLUECLASS_WR_SHUTDOWN( Tr2ProceduralSierpinski );

#endif
