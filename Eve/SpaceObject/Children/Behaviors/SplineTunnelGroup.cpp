#include "StdAfx.h"
#include "SplineTunnelGroup.h"
#include "BehaviorGroup.h"


SplineTunnelGroup::SplineTunnelGroup( IRoot* lockobj ) :
	PARENTLOCK( m_curveSets ),
	m_numBreakPoints( 2 ),
	m_tunnelWidth( 15 ),
	m_entrancePullSize( 50 ),
	m_entrySize( 20 ),
	m_tunnelGroupType( OTHER_TUNNELS )
{
	m_curveSets.SetNotify( this );
}

SplineTunnelGroup::~SplineTunnelGroup()
{
}

void SplineTunnelGroup::SetSystemTunnelFunctionReference( const std::function<void( void )>& F )
{
	m_changeSystemTunnelRegistry = F;
	createSplineTunnels();
}

void SplineTunnelGroup::createSplineTunnels()
{
	m_tunnels.clear();
	for ( auto cSet = m_curveSets.begin(); cSet != m_curveSets.end(); ++cSet )
	{
		float curveDur = (*cSet)->Length();
		int breakPoints = GetNumBreakPoints();		
		std::vector < Vector3 > Positions;
		float stepSize = curveDur / (breakPoints + 1);

		for( int i = 0; i < ( breakPoints + 2 ); i++)
		{
			Positions.push_back( (*cSet)->GetValue( i * stepSize ) );
		}

		SplineTunnelPoint point;
		std::vector<SplineTunnelPoint> splinePoints;

		for (int i = 0; i < (breakPoints + 1); i++)
		{
			point.pos = Positions[i];
			point.rot = Positions[i + 1] - Positions[i];
			splinePoints.push_back( point );
		}
		
		point.pos = Positions[breakPoints + 1];
		point.rot = Positions[breakPoints + 1] - Positions[breakPoints];
		splinePoints.push_back( point );
		
		SplineTunnel tunnel;
		tunnel.splinePoints = splinePoints;
		tunnel.cylWidth = m_tunnelWidth;
		tunnel.pullSize = m_entrancePullSize;
		tunnel.pointOfNoReturnSize = m_entrySize;
		tunnel.tunnelGroupID = m_tunnelGroupType;
		m_tunnels.push_back( tunnel );
	}

	if ( m_changeSystemTunnelRegistry )
	{
		( m_changeSystemTunnelRegistry )();
	}
}

std::vector<SplineTunnel>  SplineTunnelGroup::GetTunnels() const
{
	return m_tunnels;
}


void SplineTunnelGroup::SetNumBreakPoints( int val )
{
	m_numBreakPoints = val;
}

int SplineTunnelGroup::GetNumBreakPoints() const
{
	return max(m_numBreakPoints, 0);
}

/////////////////////////////////////////////////////////////////////////////////////
// IInitialize

bool SplineTunnelGroup::Initialize()
{
	return true;
}

/////////////////////////////////////////////////////////////////////////////////////
// IListNotify & INotify

bool SplineTunnelGroup::OnModified( Be::Var* value )
{
	//if (IsMatch( value, m_numBreakPoints ))
	createSplineTunnels();
	return true;
}

void SplineTunnelGroup::OnListModified( long event, ssize_t key, ssize_t key2, IRoot* value, const struct IList* theList )
{
	if (theList == &m_curveSets)
	{
		switch (event & BELIST_EVENTMASK)
		{
		case BELIST_INSERTED:
				createSplineTunnels();
			break;
		case BELIST_REMOVED:
				createSplineTunnels();
			break;
		case BELIST_LOADFINISHED:
				createSplineTunnels();
			break;
		default:
			break;
		}
	}
}

/////////////////////////////////////////////////////////////////////////////////////
// ITr2DebugRenderable

void SplineTunnelGroup::GetDebugOptions( Tr2DebugRendererOptions& options )
{
	options.insert( "splineTunnels" );
}

void SplineTunnelGroup::RenderDebugInfo( Tr2DebugRenderer& renderer, Matrix& parentWorldLocation )
{
	for (auto tunnel = m_tunnels.begin(); tunnel != m_tunnels.end(); ++tunnel)
	{
		// render first Point
		renderer.DrawSphere(this, TranslationMatrix((*tunnel).splinePoints[0].pos) * parentWorldLocation,
		                    (*tunnel).pullSize, 6, Tr2DebugRenderer::Wireframe, 0xff551111);
		renderer.DrawSphere(this, TranslationMatrix((*tunnel).splinePoints[0].pos) * parentWorldLocation,
		                    (*tunnel).pointOfNoReturnSize, 6, Tr2DebugRenderer::Wireframe, 0xff551111);

		// render last point ? probably gonna be removed
		renderer.DrawSphere(
			this, TranslationMatrix((*tunnel).splinePoints[GetNumBreakPoints() + 1].pos) * parentWorldLocation, 5, 6,
			Tr2DebugRenderer::Wireframe, 0xff335555);

		// render the points in-between
		auto pnts = (*tunnel).splinePoints;
		for (auto point = pnts.begin(); point != pnts.end(); ++point)
		{
			renderer.DrawSphere(this, TranslationMatrix(point->pos) * parentWorldLocation, ( *tunnel ).cylWidth, 6,
			                    Tr2DebugRenderer::Wireframe, 0xff555555);

			renderer.DrawCylinder( this, (TranslationMatrix( point->pos ) * parentWorldLocation).GetTranslation(),
				(TranslationMatrix(point->pos + point->rot) * parentWorldLocation).GetTranslation(), (*tunnel).cylWidth,
				8, Tr2DebugRenderer::Wireframe, 0xffffff00 );
		}
	}
}
