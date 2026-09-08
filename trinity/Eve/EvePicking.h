// Copyright © 2026 CCP ehf.

#pragma once

#include <vector>

#include "ITr2Renderable.h"
#include "ITr2GpuBuffer.h"
#include "Tr2PickBuffer.h"
#include "ITr2DebugRenderer2.h"



class EvePendingPickingReadback : public Tr2DeviceResource
{
public:
	EvePendingPickingReadback( uint32_t pickedX, uint32_t pickedY );

	uint32_t m_pickedX;
	uint32_t m_pickedY;

	std::vector<IRootPtr> m_blueObjects;

	Tr2ReadbackAL m_readback;


	/////////////////////////////////////////////////////////////
	// ITriDeviceResource
	void ReleaseResources( TriStorage s ) override;

private:
	bool OnPrepareResources() override;
};


BLUE_CLASS( EvePickingContext ) :
	public IRoot
{
public:
	EXPOSE_TO_BLUE();

	EvePickingContext( IRoot* lockobj = NULL );

	void UpdateResult( uint32_t x, uint32_t y, IRootPtr object, uint32_t area );

	IRoot* GetObject();
	uint32_t GetArea();

	std::vector<std::unique_ptr<EvePendingPickingReadback>> m_readbacks;

private:
	uint32_t m_lastPickedX;
	uint32_t m_lastPickedY;
	IRootPtr m_lastPickedObject;
	uint32_t m_lastPickedArea;

	friend class EveSpaceScene;
};

TYPEDEF_BLUECLASS( EvePickingContext );
