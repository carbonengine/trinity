#include "StdAfx.h"
#include "Tr2SkinnedObjectLOD.h"
#include "TriSettingsRegistrar.h"
#include "Tr2SkinnedModel.h"

// threshold for switching to different lod
float g_skinnedLowDetailThreshold = 150.0f;
float g_skinnedMediumDetailThreshold = 500.0f;
// if frametime is more than this, don't unload
float g_unloadLODMaxFrametime = 0.1f;

float g_skinnedHighMediumMargin = 0.0f;
float g_skinnedMediumLowMargin  = 0.0f;

bool g_debugLODShader = false;
TRI_REGISTER_SETTING( "debugLODShader", g_debugLODShader );

// lod resource unloading
static const double AVATAR_RESOURCE_UNLOADING_TIME_THRESHOLD = 10.0;

TRI_REGISTER_SETTING( "skinnedLowDetailThreshold",		g_skinnedLowDetailThreshold );
TRI_REGISTER_SETTING( "skinnedMediumDetailThreshold",	g_skinnedMediumDetailThreshold );
TRI_REGISTER_SETTING( "skinnedHighMediumMargin",		g_skinnedHighMediumMargin );
TRI_REGISTER_SETTING( "skinnedMediumLowMargin",			g_skinnedMediumLowMargin  );
TRI_REGISTER_SETTING( "unloadLODMaxFrametime",			g_unloadLODMaxFrametime );

CCP_STATS_DECLARE( countSkinnedLowLOD,			"Trinity/SkinnedObject/LowLOD", true, CST_COUNTER_LOW, "Skinned object count in Low LOD.");
CCP_STATS_DECLARE( countSkinnedMediumLOD,		"Trinity/SkinnedObject/MediumLOD", true, CST_COUNTER_LOW, "Skinned object count in Medium LOD.");
CCP_STATS_DECLARE( countSkinnedHighLOD,			"Trinity/SkinnedObject/HighLOD", true, CST_COUNTER_LOW, "Skinned object count in High LOD.");
CCP_STATS_DECLARE( countUnderConstructionLOD,	"Trinity/SkinnedObject/UnderConstructionLOD", true, CST_COUNTER_LOW, "Skinned object whose LOD is under construction.");


Tr2SkinnedObjectLod::Tr2SkinnedObjectLod(/*IRoot* lockobj*/) :
	m_allowLodSelection( false ),
	m_currentLod( -1 )
{
}

Tr2SkinnedObjectLod::~Tr2SkinnedObjectLod()
{
}

bool Tr2SkinnedObjectLod::OnModified( Be::Var* value )
{
	if( IsMatch( value, m_highDetailProxy ) || IsMatch( value, m_mediumDetailProxy ) || IsMatch( value, m_lowDetailProxy ) )
	{
		// if one of the lod models changed re-set all lods
		PopulateLods();
	}
	return true;
}

void Tr2SkinnedObjectLod::PopulateLods()
{
	// More clear and explicit to have some LODs available, some not, and then
	// be smart about where visualModel can point at	
	if( m_highDetailProxy || m_mediumDetailProxy || m_lowDetailProxy )
	{
		m_allowLodSelection = true;
		//m_visualModel = NULL;
	}
	else
	{
		m_allowLodSelection = false;	
	}
}

bool Tr2SkinnedObjectLod::UnloadLodIfNeeded( const Be::Time time, const Be::Time deltaTime )
{
	// only unload LODs during normal operations, ie. _not_ when the engine is stalling due to
	// heavy loads; else unload-but-oops-we're-still-using-it will trigger a reload and trash even
	// worse.
	if( !m_allowLodSelection || TimeAsFloat( deltaTime ) > g_unloadLODMaxFrametime )
	{
		return false;
	}

	Be::Time timeout = TimeFromDouble( AVATAR_RESOURCE_UNLOADING_TIME_THRESHOLD );

	// Never unload the currently used proxy
	if( m_highDetailProxy )
	{
		m_highDetailProxy->Update( time, (m_currentLod == 0) ? 0 : timeout );
	}
	if( m_mediumDetailProxy )
	{
		m_mediumDetailProxy->Update( time, (m_currentLod == 1) ? 0 : timeout );
	}
	if( m_lowDetailProxy )
	{
		m_lowDetailProxy->Update( time, (m_currentLod == 2) ? 0 : timeout );
	}

	return false;
}

Tr2SkinnedModel* Tr2SkinnedObjectLod::GetHighDetailModel()
{
	return m_highDetailProxy ? dynamic_cast<Tr2SkinnedModel*>( m_highDetailProxy->GetObject( ) ) : 0;
}

Tr2SkinnedModel* Tr2SkinnedObjectLod::GetMediumDetailModel()
{
	return m_mediumDetailProxy ? dynamic_cast<Tr2SkinnedModel*>( m_mediumDetailProxy->GetObject( ) ) : 0;
}

Tr2SkinnedModel* Tr2SkinnedObjectLod::GetLowDetailModel()
{
	return m_lowDetailProxy ? dynamic_cast<Tr2SkinnedModel*>( m_lowDetailProxy->GetObject( ) ) : 0;
}

void Tr2SkinnedObjectLod::OnModelChanged( Tr2SkinnedModel* model )
{
	if( !model )
	{
		return;
	}

	switch( m_currentLod )
	{
	case 0:
		if( m_highDetailProxy )
		{
			m_highDetailProxy->SetObjectFromBuilder( model->GetRawRoot() );
			return;
		}

	case 1:
		if( m_mediumDetailProxy )
		{
			m_mediumDetailProxy->SetObjectFromBuilder( model->GetRawRoot() );
			return;
		}

	case 2:
		if( m_lowDetailProxy )
		{
			m_lowDetailProxy->SetObjectFromBuilder( model->GetRawRoot() );
			return;
		}
	}
}

void Tr2SkinnedObjectLod::SetHighDetailModel( Tr2SkinnedModel* model )
{
	if( !m_highDetailProxy )
	{
		Be::Clsid clsid;
		if( BeClasses->FindClsid( clsid, "blue", "BlueObjectProxy" ) )
		{
			BeClasses->CreateInstance( clsid, BlueInterfaceIID<IBlueObjectProxy>(), (void**)&m_highDetailProxy );
		}
	}

	if( m_highDetailProxy && model != NULL )
	{
		m_highDetailProxy->SetObject( model->GetRawRoot() );
	}
}

void Tr2SkinnedObjectLod::SetMediumDetailModel( Tr2SkinnedModel* model )
{
	if( !m_mediumDetailProxy )
	{
		Be::Clsid clsid;
		if( BeClasses->FindClsid( clsid, "blue", "BlueObjectProxy" ) )
		{
			BeClasses->CreateInstance( clsid, BlueInterfaceIID<IBlueObjectProxy>(), (void**)&m_mediumDetailProxy );
		}
	}

	if( m_mediumDetailProxy && model != NULL )
	{
		m_mediumDetailProxy->SetObject( model->GetRawRoot() );
	}
}

void Tr2SkinnedObjectLod::SetLowDetailModel( Tr2SkinnedModel * model )
{
	if( !m_lowDetailProxy )
	{
		Be::Clsid clsid;
		if( BeClasses->FindClsid( clsid, "blue", "BlueObjectProxy" ) )
		{
			BeClasses->CreateInstance( clsid, BlueInterfaceIID<IBlueObjectProxy>(), (void**)&m_lowDetailProxy );
		}
	}

	if( m_lowDetailProxy && model != NULL )
	{
		m_lowDetailProxy->SetObject( model->GetRawRoot() );
	}
}

Tr2SkinnedModel* Tr2SkinnedObjectLod::SetLOD( const TriFrustum* frustum, float estimatedPixelDiameter )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	if( !m_allowLodSelection )
	{
		return NULL;
	}

	IBlueObjectProxyPtr proxy[3] = { m_highDetailProxy, m_mediumDetailProxy, m_lowDetailProxy };

	// are any of the proxies in the middle of building something?  In that case, wait that out so
	// we don't bombard the resource system.
	int stickyLod = -1;
	for( unsigned i = 0; i != 3; ++i )
	{
		if( proxy[i] && proxy[i]->IsTemporary() )
		{
			CCP_STATS_INC( countUnderConstructionLOD );
			stickyLod = m_currentLod;
		}
	}

    const int high   = 0;
    const int medium = 1;
    const int low    = 2;

	int choice[3] = { high, medium, low };	// order of preferred LOD. set up for high lod by default

	if( stickyLod == 2 || 
		// small enough for low lod?
		( m_currentLod >= 2 && estimatedPixelDiameter <= g_skinnedLowDetailThreshold + g_skinnedMediumLowMargin ) ||	// before going up, add some margin
		( m_currentLod <  2 && estimatedPixelDiameter <= g_skinnedLowDetailThreshold - g_skinnedMediumLowMargin ) )
	{
		choice[0] = low;
		choice[1] = medium;
		choice[2] = high;
	}
	else
	if( stickyLod == 1 || 
		// medium lod?
		( m_currentLod >= 1 && estimatedPixelDiameter <= g_skinnedMediumDetailThreshold + g_skinnedHighMediumMargin ) ||	// before going up, add some margin
		( m_currentLod <  1 && estimatedPixelDiameter <= g_skinnedMediumDetailThreshold - g_skinnedHighMediumMargin ) )
	{
		choice[0] = medium;
		choice[1] = low;
		choice[2] = high;
	}

	// Go through our choices of LOD in order, until we find something that's not null.
	// If we got something, but it's still loading or a temp stub, we may still want to switch
	// to something better, _if_ the new choice is already resident (so we don't trigger loads
	// on everything at the same time).

	int selectedLod = -1;
	Tr2SkinnedModel* model = NULL;
	bool modelIsTemporary = true;

	for (unsigned i = 0; i != 3; ++i)
	{
		const int lod = choice[i];
		if( proxy[lod] )
			if ( !model || 
				( modelIsTemporary && proxy[lod]->IsResident() ) )
			{
				model = dynamic_cast<Tr2SkinnedModel*>( proxy[lod]->GetObject( ) );
				selectedLod = lod;
				modelIsTemporary = proxy[lod]->IsTemporary();
			}
	}

	if( model && m_currentLod != selectedLod && selectedLod != -1 )
	{
		m_currentLod = selectedLod;
		proxy[m_currentLod]->OnSelected();
	}

#if CCP_STATS_ENABLED
	switch( m_currentLod )
	{
		case high:
			CCP_STATS_INC( countSkinnedHighLOD );
			break;
		case medium:
			CCP_STATS_INC( countSkinnedMediumLOD );
			break;
		case low:
			CCP_STATS_INC( countSkinnedLowLOD );
			break;

		default:
			CCP_STATS_INC( countSkinnedHighLOD );
	}
#endif

	return model;
}

bool Tr2SkinnedObjectLod::IsCastingShadow() const
{
	// only cast shadow if highest lod
	return !m_allowLodSelection || ( m_currentLod == 0 );
}

bool Tr2SkinnedObjectLod::IsSimulatingCloth( const int maxClothLod ) const
{
	return !m_allowLodSelection || ( m_currentLod <= maxClothLod );
}
