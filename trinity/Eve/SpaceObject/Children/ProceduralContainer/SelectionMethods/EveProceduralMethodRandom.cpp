// Copyright © 2021 CCP ehf.

#include "EveProceduralMethodRandom.h"


EveProceduralMethodRandom::EveProceduralMethodRandom( IRoot* lockobj ) :
	PARENTLOCK( m_parameters ),
	PARENTLOCK( m_debugVolumes ),
	m_seed( -1 ),
	m_selectedChildIndex( -1 ),
	m_totalWeight( 0 ),
	m_selectedChildModified( false )
{
	m_parameters.SetNotify( this );
}

EveProceduralMethodRandom::~EveProceduralMethodRandom()
{
}

bool EveProceduralMethodRandom::Initialize()
{
	GenerateParameterMapping();
	return true;
}

bool EveProceduralMethodRandom::OnModified( Be::Var* value )
{
	if( IsMatch( value, m_seed ) )
	{
		SelectARandomParameter();
	}

	return true;
}

void EveProceduralMethodRandom::OnListModified( long event, ssize_t key, ssize_t key2, IRoot* value, const struct IList* list )
{
	if( list == &m_parameters && ( event & BELIST_LOADING ) == 0 )
	{
		GenerateParameterMapping();
	}
}

void EveProceduralMethodRandom::GenerateParameterMapping()
{
	m_parameterMapping.clear();
	m_parameterMapping.reserve( m_parameters.size() );
	m_totalWeight = 0;
	for( auto it = begin( m_parameters ); it != end( m_parameters ); ++it )
	{
		m_totalWeight += ( *it )->GetWeighting();
		m_parameterMapping.push_back( m_totalWeight );
	}
}

void EveProceduralMethodRandom::SelectARandomParameter()
{
	if( m_parameterMapping.empty() )
	{
		return;
	}

	int currentChild = m_selectedChildIndex;
	srand( (int)m_seed );
	int rnd = (int)rand() % m_totalWeight;

	m_selectedChildIndex = -1;
	for( int i = 0; i < (int)m_parameterMapping.size(); i++ )
	{
		if( rnd < m_parameterMapping[i] )
		{
			m_selectedChildIndex = i;
			break;
		}
	}

	if( currentChild != m_selectedChildIndex )
	{
		m_selectedChildModified = true;
	}
}

bool EveProceduralMethodRandom::IsSelectedChildModified() const
{
	return m_selectedChildModified;
}

EveChildRefPtr EveProceduralMethodRandom::GetSelectedChild()
{
	if( m_selectedChildIndex < 0 || m_selectedChildIndex > ( m_parameters.size() - 1 ) )
	{
		return nullptr;
	}

	m_selectedChildModified = false;
	EveProceduralMethodRandomParameterPtr param = BlueCastPtr( m_parameters.GetAt( m_selectedChildIndex ) );

	if( param != nullptr )
	{
		auto child = param->GetChild();
		if( child != nullptr && strlen( child->GetResPath() ) != 0 )
		{
			param->Load();
			return child;
		}
	}
	return nullptr;
}

void EveProceduralMethodRandom::UpdateAsyncronous( const EveUpdateContext& updateContext, const EveChildUpdateParams& params )
{
	bool regenerateParameterMap = false;
	for( auto it = begin( m_parameters ); it != end( m_parameters ); ++it )
	{
		if( ( *it )->IsModified() )
		{
			regenerateParameterMap = true;
			( *it )->SetModified( false );
		}
	}
	if( regenerateParameterMap )
	{
		GenerateParameterMapping();
		SelectARandomParameter();
	}
}

IEveVolumeVector* EveProceduralMethodRandom::GetDebugVolumes()
{
	return &m_debugVolumes;
}

void EveProceduralMethodRandom::SetProceduralMethodVariable( const char* name, float value )
{
	if( strcmp( name, m_seedName.c_str() ) == 0 )
	{
		if( m_seed != value )
		{
			m_seed = value;
			SelectARandomParameter();
		}
	}
}

const char* EveProceduralMethodRandom::GetProceduralMethodVariable()
{
	if( strcmp( "", m_seedName.c_str() ) == 0 )
	{
		return "nameMissing";
	}

	return m_seedName.c_str();
}