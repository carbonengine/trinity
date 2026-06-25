// Copyright © 2021 CCP ehf.

#include "EveProceduralMethodAttributeMap.h"


EveProceduralMethodAttributeMap::EveProceduralMethodAttributeMap( IRoot* lockobj ) :
	PARENTLOCK( m_parameters ),
	PARENTLOCK( m_debugVolumes ),
	m_selectedChildIndex( -1 ),
	m_selectedChildModified( false )
{
}

EveProceduralMethodAttributeMap::~EveProceduralMethodAttributeMap()
{
}


bool EveProceduralMethodAttributeMap::OnModified( Be::Var* value )
{
	if( IsMatch( value, m_seed ) )
	{
		SelectParameter();
	}

	return true;
}

void EveProceduralMethodAttributeMap::SelectParameter()
{
	int currentChild = m_selectedChildIndex;
	int index = 0;
	for( auto it = begin( m_parameters ); it != end( m_parameters ); ++it )
	{
		if( strcmp( ( *it )->GetName(), m_seed.c_str() ) == 0 )
		{
			m_selectedChildIndex = index;
			break;
		}
		index++;
	}

	if( currentChild != m_selectedChildIndex )
	{
		m_selectedChildModified = true;
	}
}

bool EveProceduralMethodAttributeMap::IsSelectedChildModified() const
{
	return m_selectedChildModified;
}

EveChildRefPtr EveProceduralMethodAttributeMap::GetSelectedChild()
{
	if( m_selectedChildIndex < 0 || m_selectedChildIndex > ( m_parameters.size() - 1 ) )
	{
		return nullptr;
	}

	m_selectedChildModified = false;
	EveProceduralMethodAttributeMapParameterPtr param = BlueCastPtr( m_parameters.GetAt( m_selectedChildIndex ) );

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

void EveProceduralMethodAttributeMap::UpdateAsyncronous( const EveUpdateContext& updateContext, const EveChildUpdateParams& params )
{
	bool reselect = false;
	for( auto it = begin( m_parameters ); it != end( m_parameters ); ++it )
	{
		if( ( *it )->IsModified() )
		{
			reselect = true;
			( *it )->SetModified( false );
		}
	}
	if( reselect )
	{
		SelectParameter();
	}
}

IEveVolumeVector* EveProceduralMethodAttributeMap::GetDebugVolumes()
{
	return &m_debugVolumes;
}