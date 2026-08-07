// Copyright © 2026 CCP ehf.

#include "StdAfx.h"
#include "EveSpaceObjectChild.h"


EveSpaceObjectChild::EveSpaceObjectChild( IRoot* )
{
}

const char* EveSpaceObjectChild::GetName() const
{
	return m_name.c_str();
}

void EveSpaceObjectChild::SetName( const char* name )
{
	m_name = BlueSharedString( name );
}

void EveSpaceObjectChild::UpdateVisibility( const EveUpdateContext&, const Matrix&, Tr2Lod )
{
}

void EveSpaceObjectChild::GetRenderables( std::vector<ITr2Renderable*>& )
{
}

bool EveSpaceObjectChild::GetBoundingSphere( Vector4&, BoundingSphereQuery ) const
{
	return false;
}

void EveSpaceObjectChild::RegisterWithQuadRenderer( Tr2QuadRenderer& )
{
}

void EveSpaceObjectChild::AddQuadsToQuadRenderer( const TriFrustum&, Tr2QuadRenderer& ) const
{
}

void EveSpaceObjectChild::UpdateSyncronous( const EveUpdateContext&, const EveChildUpdateParams& )
{
}

void EveSpaceObjectChild::UpdateAsyncronous( const EveUpdateContext&, const EveChildUpdateParams& )
{
}

void EveSpaceObjectChild::GetLocalToWorldTransform( Matrix& ) const
{
}

bool EveSpaceObjectChild::IsAlwaysOn() const
{
	return false;
};

void EveSpaceObjectChild::Setup( const Vector3*, const Quaternion*, const Vector3*, Tr2Lod )
{
}

void EveSpaceObjectChild::ChangeLOD( Tr2Lod )
{
}

void EveSpaceObjectChild::SetControllerVariable( const char*, float )
{
}

void EveSpaceObjectChild::HandleControllerEvent( const char* )
{
}

void EveSpaceObjectChild::StartControllers()
{
}

void EveSpaceObjectChild::SetProceduralContainerVariable( const char*, float )
{
}

void EveSpaceObjectChild::SetShaderOption( const BlueSharedString&, const BlueSharedString& )
{
}

void EveSpaceObjectChild::SetOrigin( Origin )
{
}

void EveSpaceObjectChild::AddTransformModifier( IEveChildTransformModifier* )
{
}

void EveSpaceObjectChild::SetMute( bool isMuted )
{
}

EveSpaceObjectChild::PartTag EveSpaceObjectChild::GetPartTag() const
{
	return m_partTag;
}

void EveSpaceObjectChild::SetPartTag( PartTag tag )
{
	m_partTag = tag;
}

void EveSpaceObjectChild::SetOwner( IEveSpaceObject2* owner )
{
	m_owner = owner;
}

IEveSpaceObject2* EveSpaceObjectChild::GetOwner() const
{
	return m_owner;
}

void EveSpaceObjectChild::SetParent( EveSpaceObjectChild* parent )
{
	m_parent = parent;
}

EveSpaceObjectChild* EveSpaceObjectChild::GetParent() const
{
	return m_parent;
}

void EveSpaceObjectChild::RegisterChild( EveSpaceObjectChild* child )
{
	if( child )
	{
		child->SetParent( this );
		child->SetOwner( m_owner );
		if( m_partTag != EveSpaceObjectChild::NO_PART_TAG )
		{
			child->SetPartTag( m_partTag );
		}
	}
}

void EveSpaceObjectChild::UnregisterChild( EveSpaceObjectChild* child )
{
	if( child )
	{
		CCP_ASSERT( child->GetParent() == this );
		child->SetParent( nullptr );
		child->SetOwner( nullptr );
		// No reason to reset the part tag as it is meaningless outside the hierarchy of the parent object
	}
}
