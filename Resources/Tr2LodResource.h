////////////////////////////////////////////////////////////////////////////////
//
// Created:		August 2014
// Copyright:	CCP 2014
//

#pragma once
#ifndef Tr2LodResource_h
#define Tr2LodResource_h

enum Tr2Lod {
	TR2_LOD_UNSPECIFIED = -1,

	TR2_LOD_HIGH = 0,
	TR2_LOD_MEDIUM,
	TR2_LOD_LOW,

	TR2_LOD_COUNT
};

BLUE_CLASS( Tr2LodResource ) : public IRoot
{
public:
	EXPOSE_TO_BLUE();

	Tr2LodResource( IRoot* lockobj = nullptr );

	// access
	void SetResourcePath( Tr2Lod lod, const char* resPath );
	IBlueResource* GetResource();

	void SelectLod( Tr2Lod lod );

protected:
	std::string m_resPath[TR2_LOD_COUNT];
	Tr2Lod m_currentLod;

	IBlueResourcePtr m_active;
	IBlueResourcePtr m_requested;
};

TYPEDEF_BLUECLASS( Tr2LodResource );

#endif // Tr2LodResource_h
