// Copyright © 2026 CCP ehf.

#pragma once
#ifndef Tr2ProceduralGeometryProvider_h
#define Tr2ProceduralGeometryProvider_h

#include "Tr2GeometryProvider.h"

struct Tr2ProceduralGeometryBuilder
{
	virtual ~Tr2ProceduralGeometryBuilder() = default;
	virtual bool BuildCmf( cmf::MemoryAllocator& allocator, cmf::BufferManager& buffers, const std::atomic<bool>& cancel, cmf::Data& data ) = 0;
	std::string name;
};

BLUE_CLASS_IMPL( Tr2ProceduralGeometryProvider )
class Tr2ProceduralGeometryProvider : public Tr2GeometryProvider
{
public:
	EXPOSE_TO_BLUE();

	void Rebuild();
	void SetSynchronous( bool synchronous );

protected:
	Tr2ProceduralGeometryProvider( IRoot* lockobj = nullptr );
	virtual ~Tr2ProceduralGeometryProvider();

	virtual std::unique_ptr<Tr2ProceduralGeometryBuilder> CreateBuilder() const = 0;

	bool OnPrepareResources() override;
	void ReleaseResources( TriStorage s ) override;

#if TRINITYDEV
	void GetDescription( std::string& desc ) override;
#endif

private:
	struct Build
	{
		std::atomic<bool> cancel{ false };
		std::vector<uint8_t> bytes;
		std::unique_ptr<Tr2ProceduralGeometryBuilder> builder;
		Tr2ProceduralGeometryProvider* owner = nullptr;
	};

	static void StaticBuild( void* context );
	static void StaticPrepare( void* context );
	static void StaticFail( void* context );
	static bool StaticRunBuild( Build& build );
	void Prepare( Build& build );
	void FailBuild();
	void CancelBuild();

	Build* m_build;
	bool m_synchronous;
};

TYPEDEF_BLUECLASS_WR_SHUTDOWN( Tr2ProceduralGeometryProvider );

#endif
