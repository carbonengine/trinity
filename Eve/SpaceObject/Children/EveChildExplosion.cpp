////////////////////////////////////////////////////////////
//
//    Created:   March 2016
//    Copyright: CCP 2016
//

#include "StdAfx.h"
#include "EveChildExplosion.h"
#include "Eve/EveUpdateContext.h"
#include "Particle/Tr2SphereShapeAttributeGenerator.h"

EveChildExplosion::EveChildExplosion( IRoot* lockobj )
	:EveChildContainer( lockobj ),
	PARENTLOCK( m_localExplosions ),
	m_localExplosionDelay( 0.f ),
	m_localExplosionInterval( 1.f ),
	m_localExplosionIntervalFactor( 1.f ),
	m_globalExplosionDelay( 0.f ),
	m_wreckSwitchTime( 0.f ),
	m_localDuration( 0.f ),
	m_globalDuration( 0.f ),
	m_localExplosionTransforms( "EveExplosion::m_localExplosionOrigins" ),
	m_sharedObjects( "EveExplosion::m_sharedObjects" ),
	m_playTime( 0.f ),
	m_nextLocalExplosionTime( 0.f ),
	m_globalExplosionTime( 0.f ),
	m_nextLocalExplosion( 0 ),
	m_isPlaying( false )
{
}

EveChildExplosion::~EveChildExplosion()
{
}

// --------------------------------------------------------------------------------------
// Description:
//   Starts explosion playback
// --------------------------------------------------------------------------------------
void EveChildExplosion::Play()
{
	Stop();
	if( !m_localExplosion && !m_globalExplosion && m_localExplosions.empty() )
	{
		return;
	}

	m_nextLocalExplosionTime = m_localExplosionDelay;
	m_nextLocalExplosion = 0;
	m_globalExplosionTime = m_globalExplosionDelay;
	m_objects.Append( m_localExplosionShared );
	FindSharedObjects();
	m_isPlaying = true;
	m_playTime = 0;
}

// --------------------------------------------------------------------------------------
// Description:
//   Stops explosion playback
// --------------------------------------------------------------------------------------
void EveChildExplosion::Stop()
{
	m_objects.Clear();
	m_isPlaying = false;
	m_sharedObjects.clear();
	m_globalExplosionInstance.Unlock();
}

// --------------------------------------------------------------------------------------
// Description:
//   Assigns transforms to be used for local explosions
// Arguments:
//   transforms - Transforms for local explosions
// --------------------------------------------------------------------------------------
void EveChildExplosion::SetLocalExplosionTransforms( const std::vector<Matrix>& transforms )
{
	m_localExplosionTransforms.clear();
	m_localExplosionTransforms.insert( 
		m_localExplosionTransforms.begin(), 
		transforms.begin(), 
		transforms.end() );
}

// --------------------------------------------------------------------------------------
// Description:
//   Implements IEveSpaceObjectChild interface. If the effect is playing this function
//   spawns explosions.
// Arguments:
//   updateContext - Update context
//   spaceObjectParent - Space object parent
//   childParent - Child parent
// --------------------------------------------------------------------------------------
void EveChildExplosion::UpdateSyncronous( 
	EveUpdateContext& updateContext, 
	IEveSpaceObject2* spaceObjectParent, 
	IEveSpaceObjectChild* childParent )
{
	if( m_isPlaying )
	{
		auto dt = updateContext.GetDeltaT();
		m_playTime += dt;
		if( m_localExplosion || !m_localExplosions.empty() )
		{
			if( m_wreckSwitchTime > 0 && m_playTime > m_wreckSwitchTime )
			{
				m_nextLocalExplosion = m_localExplosionTransforms.size();
				for( size_t i = 0; i < m_objects.size(); )
				{
					if( m_objects[i] != m_globalExplosionInstance && m_objects[i] != m_localExplosionShared )
					{
						m_objects.Remove( i );
					}
					else
					{
						++i;
					}
				}
			}
			m_nextLocalExplosionTime -= dt;
			if( m_nextLocalExplosionTime < 0 )
			{
				if( m_nextLocalExplosion < m_localExplosionTransforms.size() )
				{
					XMVECTOR det;
					Matrix transform = m_localExplosionTransforms[m_nextLocalExplosion];
					transform.GetTranslation() = XMVector3TransformCoord( transform.GetTranslation(), XMMatrixInverse( &det, m_localTransform ) );
					SpawnLocalExplosion( transform );
					++m_nextLocalExplosion;
					m_nextLocalExplosionTime = std::pow( m_localExplosionIntervalFactor, float( m_nextLocalExplosion ) ) * 
						m_localExplosionInterval * float( rand() ) / float( RAND_MAX );
				}
				else if( -m_nextLocalExplosionTime > m_localDuration && 
					( !m_globalExplosion || -m_globalExplosionTime > m_globalDuration ) )
				{
					Stop();
				}
			}
		}
		if( m_globalExplosion )
		{
			m_globalExplosionTime -= dt;
			if( m_globalExplosionTime < 0 )
			{
				if( !m_globalExplosionInstance )
				{
					m_globalExplosionInstance.Unlock();
					if( BeClasses->CopyTo( m_globalExplosion, (IRoot**)&m_globalExplosionInstance.p ) )
					{
						m_objects.Append( m_globalExplosionInstance );
					}
				}
				else if( !m_localExplosion && m_localExplosions.empty() && -m_globalExplosionTime > m_globalDuration )
				{
					Stop();
				}
			}
		}
	}
	EveChildContainer::UpdateSyncronous( updateContext, spaceObjectParent, childParent );
}

// --------------------------------------------------------------------------------------
// Description:
//   Searches for all IRoot descendants in m_localExplosionShared and puts them into
//   m_sharedObjects vector. I wish we had a helper function for this in blue.
// --------------------------------------------------------------------------------------
void EveChildExplosion::FindSharedObjects()
{
	m_sharedObjects.clear();

	if( !m_localExplosionShared )
	{
		return;
	}
	std::vector<IRoot*> stack;

	stack.push_back( m_localExplosionShared->GetRootObject() );
	while( !stack.empty() )
	{
		auto obj = stack.back();
		stack.pop_back();

		auto res = m_sharedObjects.insert( obj );
		if( !res.second )
		{
			continue;
		}
		
		const Be::ClassInfo* type = obj->ClassType();
		for( ssize_t xtraoffs = 0; type; xtraoffs += type->mOffsetToParent, type = type->mParentClassInfo )
		{
			for( const Be::VarEntry *entry = type->mMemberTable; entry->mName; entry++ )
			{
				if( ( entry->mEditFlags & Be::PERSIST ) && ( entry->mType == Be::IROOT || entry->mType == Be::IROOTPTR ) )
				{
					Be::Var* value = BLUEMAPMEMBEROFFSET( obj, entry, type, xtraoffs );
					if( entry->mType == Be::IROOTPTR ) 
					{
						if( value->mIRootPtr )
						{
							stack.push_back( value->mIRootPtr );
						}
					} 
					else 
					{
						stack.push_back( reinterpret_cast<IRoot*>( value ) );
					}
					stack.push_back( obj );
				}
			}
		}
		if( IListPtr list = BlueCastPtr( obj ) )
		{
			ListInfo info;
			list->GetInfo( &info );
			for( ssize_t i = 0; i < list->GetSize(); i++ ) 
			{
				if( IRoot *item = list->GetAt(i) )
				{
					stack.push_back( item );
				}
			}
		}
		if( IBlueDictPtr dict = BlueCastPtr( obj ) )
		{
			size_t n = dict->GetLength();
			for( size_t i = 0; i < n; ++i )
			{
				if( IRoot* item = dict->Subscript( dict->GetKey( i ) ) )
				{
					stack.push_back( item );
				}
			}
		}
	}
}

// --------------------------------------------------------------------------------------
// Description:
//   Callback for BeClasses->CopyTo. When a local explosion instance is copied we need to
//   retain all references to the shared part of the local explosion. This function 
//   provides a custom copy for such references.
// Arguments:
//   source - Source object to copy
//   dest - Pointer to the destination object
//   copier - ICopier instance (unused)
//   context - Callback context (EveExplosion pointer)
// --------------------------------------------------------------------------------------
ICopier::OverrideResult EveChildExplosion::CopyElement( IRoot* source, IRoot** dest, ICopier* copier, void* context )
{
	source = source ? source->GetRootObject() : nullptr;
	auto self = static_cast<EveChildExplosion*>( context );
	auto found = self->m_sharedObjects.find( source );
	if( found == self->m_sharedObjects.end() )
	{
		// if not a shared object, let the ICopier handle it
		return ICopier::FALLBACK;
	}
	// otherwise leave it untouched
	if( *dest )
	{
		( *dest )->Unlock();
	}
	*dest = source;
	if( source )
	{
		source->Lock();
	}
	return ICopier::SUCCESS;
}

// --------------------------------------------------------------------------------------
// Description:
//   Post-copy callback for BeClasses->CopyTo. The function updates positions of newly
//   copied Tr2SphereShapeAttributeGenerator objects so that particle emitters can still
//   work with shared particle systems.
// Arguments:
//   source - Source object to copy (unused)
//   dest - Pointer to the destination object
//   copier - ICopier instance (unused)
//   context - Callback context (EveExplosion::Transform pointer)
// --------------------------------------------------------------------------------------
void EveChildExplosion::UpdateEmitter( IRoot* source, IRoot** dest, ICopier* copier, void* context )
{
	IRootPtr obj( *dest );
	if( Tr2SphereShapeAttributeGeneratorPtr generator = BlueCastPtr( *dest ) )
	{
		auto& transform = *static_cast<Transform*>( context );
		Vector3 position;
		Quaternion rotation;
		generator->GetTransform( position, rotation );
		position = XMQuaternionMultiply( transform.rotation, 
			XMQuaternionMultiply( position, XMQuaternionConjugate( transform.rotation ) ) );
		rotation = XMQuaternionMultiply( transform.rotation, rotation );
		generator->SetTransform( position + transform.position, rotation );
	}
}

// --------------------------------------------------------------------------------------
// Description:
//   Creates a new local explosion.
// Arguments:
//   transform - Local explosion transform
// --------------------------------------------------------------------------------------
void EveChildExplosion::SpawnLocalExplosion( const Matrix& transform )
{
	if( !m_localExplosion && m_localExplosions.empty() )
	{
		return;
	}
	IEveSpaceObjectChildPtr instance;

	Transform t;
	Vector3 scale;
	D3DXMatrixDecompose( &scale, &t.rotation, &t.position, &transform );

	auto localExplosion = m_localExplosion;
	if( !m_localExplosions.empty() )
	{
		localExplosion = m_localExplosions[rand() % m_localExplosions.size()];
	}
	if( !BeClasses->CopyTo( localExplosion, (IRoot**)&instance.p, &CopyElement, this, &UpdateEmitter, &t ) )
	{
		return;
	}
	instance->Setup( &scale, &t.rotation, &t.position, TR2_LOD_LOW );
	m_objects.Append( instance );
}