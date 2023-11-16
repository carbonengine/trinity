#pragma once
#ifndef SplineTunnelGroup_H
#define SplineTunnelGroup_H

#include "Tr2DebugRenderer.h"
#include "Curves/Tr2CurveVector3.h"


struct ITr2Renderable;

struct SplineTunnelPoint
{
	friend bool operator==(const SplineTunnelPoint& lhs, const SplineTunnelPoint& rhs)
	{
		return lhs.accelerationMultiplier == rhs.accelerationMultiplier
			&& lhs.pos == rhs.pos
			&& lhs.rot == rhs.rot;
	}

	friend bool operator!=(const SplineTunnelPoint& lhs, const SplineTunnelPoint& rhs)
	{
		return !(lhs == rhs);
	}

	SplineTunnelPoint() :
		accelerationMultiplier( 1.f ),
		pos( 0, 0, 0 ),
		rot( 0, 0, 0 )
	{}

	float accelerationMultiplier;
	Vector3 pos;
	Vector3 rot;
};

struct SplineTunnel
{
	SplineTunnel() :
		pullSize( 50 ),
		pointOfNoReturnSize( 20 ),
		cylWidth( 20 ),
		tunnelID( -1 ),
		tunnelGroupType( 0 )
	{}

	int tunnelID;
	int tunnelGroupType;
	std::vector<SplineTunnelPoint> splinePoints;
	float cylWidth;
	float accelerationMultiplier;
	float pullSize;
	float pointOfNoReturnSize;
};

BLUE_DECLARE( SplineTunnelGroup );
BLUE_DECLARE( Tr2CurveVector3 );
BLUE_DECLARE_VECTOR( Tr2CurveVector3 );

BLUE_CLASS( SplineTunnelGroup ) :
	public IListNotify,
	public INotify
{
public:
	EXPOSE_TO_BLUE();

	enum TunnelGroupType
	{
		EXIT_TUNNELS = 0,
		ENTRANCE_TUNNELS = 1,
		OTHER_TUNNELS = 2,
	};

	SplineTunnelGroup( IRoot* lockobj = nullptr );
	~SplineTunnelGroup();
	TunnelGroupType GetTunnelGroupType() const;
	void SetSystemTunnelFunctionReferenceAndColor(const std::function<void()>& F, uint32_t color );

	// This ( SplineTunnelGroup | special functions )
	void CreateSplineTunnels();
	std::vector<SplineTunnel>* GetTunnels();
	void SetNumBreakPoints(int val);
	int GetNumBreakPoints() const;
	Tr2CurveVector3Vector* GetCurveSets();

	// ITr2DebugRenderable
	virtual void GetDebugOptions( Tr2DebugRendererOptions& options );
	virtual void RenderDebugInfo( ITr2DebugRenderer2& renderer, Matrix& parentWorldLocation );

	// IInitialize
	static bool Initialize();
	void OnListModified(long event, ssize_t key, ssize_t key2, IRoot* value, const IList* theList);
	bool OnModified( Be::Var* value );
	

private:

	TunnelGroupType m_tunnelGroupType;
	int32_t m_numBreakPoints;
	std::vector<SplineTunnel> m_tunnels;
	PTr2CurveVector3Vector m_curveSets;
	std::function<void()> m_changeSystemTunnelRegistry;
	float m_tunnelWidth;
	float m_entrancePullSize;
	float m_entrySize;
	uint32_t m_debugColor;

}; 

TYPEDEF_BLUECLASS( SplineTunnelGroup );

#endif
