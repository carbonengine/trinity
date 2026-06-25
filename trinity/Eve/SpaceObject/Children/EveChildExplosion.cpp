// Copyright © 2016 CCP ehf.

#include "StdAfx.h"
#include "EveChildExplosion.h"
#include "Eve/EveUpdateContext.h"
#include "Particle/Tr2SphereShapeAttributeGenerator.h"

EveChildExplosion::EveChildExplosion( IRoot* lockobj ) :
	EveChildContainer( lockobj ),
	PARENTLOCK( m_localExplosions ),
	PARENTLOCK( m_globalExplosions ),
	PARENTLOCK( m_globalExplosionInstances ),
	m_localExplosionDelay( 0.f ),
	m_localExplosionInterval( 1.f ),
	m_localExplosionIntervalFactor( 1.f ),
	m_globalExplosionDelay( 0.f ),
	m_wreckSwitchTime( 0.f ),
	m_wreckSwitchOffsetFromGlobalStart( 0.f ),
	m_localDuration( 0.f ),
	m_globalDuration( 0.f ),
	m_totalDuration( 0.f ),
	m_localExplosionTransforms( "EveExplosion::m_localExplosionOrigins" ),
	m_sharedObjects( "EveExplosion::m_sharedObjects" ),
	m_playTime( 0.f ),
	m_nextLocalExplosionTime( 0.f ),
	m_globalExplosionTime( 0.f ),
	m_countdownToGlobalExplosionStart( 0.f ),
	m_nextLocalExplosion( 0 ),
	m_isPlaying( false ),
	m_globalExplosionScaling( 1.0f, 1.0f, 1.0f ),
	m_localExplosionScaling( 1.0f, 1.0f, 1.0f ),
	m_globalExplosionOffset( 0.0f, 0.0f, 0.0f )
{
}

EveChildExplosion::~EveChildExplosion()
{
}

// --------------------------------------------------------------------------------
// Description:
//    Registers itself and its children with the scene registration container.
//    This is so we don't have to traverse the tree every frame
// --------------------------------------------------------------------------------
void EveChildExplosion::RegisterComponents()
{
	EveChildContainer::RegisterComponents();
}

// --------------------------------------------------------------------------------
// Description:
//    Unregisters itself and its children with the scene registration container.
// --------------------------------------------------------------------------------
void EveChildExplosion::UnRegisterComponents()
{
	// manually unregister the global explosion container
	if( m_globalExplosionContainer )
	{
		if( EveEntityPtr entity = BlueCastPtr( m_globalExplosionContainer ) )
		{
			entity->UnRegister( GetComponentRegistry() );
		}
	}
	EveChildContainer::UnRegisterComponents();
}

// --------------------------------------------------------------------------------------
// Description:
//   Starts explosion playback
// --------------------------------------------------------------------------------------
void EveChildExplosion::Play()
{
	Stop();
	if( !m_localExplosion && !m_globalExplosion && m_localExplosions.empty() && m_globalExplosions.empty() )
	{
		return;
	}

	m_nextLocalExplosionTime = m_localExplosionDelay;
	m_nextLocalExplosion = 0;
	m_objects.Append( m_localExplosionShared );
	FindSharedObjects();
	CalculateExplosionTimes( uint32_t( m_localExplosionTransforms.size() ) );
	m_playTime = 0;
	m_countdownToGlobalExplosionStart = m_globalExplosionTime;

	RebuildLocalTransform();
	m_isPlaying = true;
}

// --------------------------------------------------------------------------------------
// Description:
//   Stops explosion playback
// --------------------------------------------------------------------------------------
void EveChildExplosion::Stop()
{
	// manually unregister the global explosion container
	if( this->IsInRegistry() )
	{
		UnRegisterComponents();
	}
	m_objects.Clear();
	m_isPlaying = false;
	m_sharedObjects.clear();
	m_globalExplosionInstances.Clear();
	m_globalExplosionContainer.Unlock();
}

// --------------------------------------------------------------------------------------
// Description:
//   Calculates the local explosion and global explosion start times
// --------------------------------------------------------------------------------------
void EveChildExplosion::CalculateExplosionTimes( uint32_t localExplosionCount )
{
	m_localExplosionTimes.clear();
	float timeUntilLastLocalExplosion = 0.f;
	m_globalExplosionTime = 0.f;
	m_totalDuration = 0.f;

	if( localExplosionCount != 0 )
	{
		timeUntilLastLocalExplosion += m_localExplosionDelay;
		m_globalExplosionTime += m_globalExplosionDelay;
	}

	for( uint32_t i = 0; i < localExplosionCount; i++ )
	{
		auto explosionTime = std::pow( m_localExplosionIntervalFactor, float( i ) ) *
			m_localExplosionInterval * float( rand() ) / float( RAND_MAX );

		m_localExplosionTimes.push_back( explosionTime );
		timeUntilLastLocalExplosion += explosionTime;
	}
	float totalLocalDuration = m_localDuration + timeUntilLastLocalExplosion;
	m_globalExplosionTime += timeUntilLastLocalExplosion;

	// max this because we might have explosions that do not have a global explosion
	m_totalDuration = max( totalLocalDuration, m_globalExplosionTime + m_globalDuration );

	m_wreckSwitchTime = m_globalExplosionTime + m_wreckSwitchOffsetFromGlobalStart;
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
//   Assigns a offset to use as an offset for the global explosion
//   This offset will be scaled by the inverse of the parent scale, so it is positioned
//	 correctly
// Arguments:
//   offset - The offset to use
// --------------------------------------------------------------------------------------
void EveChildExplosion::SetGlobalExplosionOffset( const Vector3& offset )
{
	m_globalExplosionOffset = offset;
	m_globalExplosionOffset.x /= this->m_scaling.x;
	m_globalExplosionOffset.y /= this->m_scaling.y;
	m_globalExplosionOffset.z /= this->m_scaling.z;
}

// --------------------------------------------------------------------------------------
// Description:
//   Implements IEveSpaceObjectChild interface. If the effect is playing this function
//   spawns explosions.
// --------------------------------------------------------------------------------------
void EveChildExplosion::UpdateSyncronous( const EveUpdateContext& updateContext, const EveChildUpdateParams& params )
{
	if( m_isPlaying )
	{
		auto dt = updateContext.GetDeltaT();
		m_playTime += dt;
		if( m_localExplosion || !m_localExplosions.empty() )
		{
			// we only want to remove the small explosions if there is a big explosion
			if( m_wreckSwitchTime > 0 && m_playTime > m_wreckSwitchTime && m_globalDuration > 0 )
			{
				m_nextLocalExplosion = m_localExplosionTransforms.size();
				for( size_t i = 0; i < m_objects.size(); )
				{
					if( m_objects[i] != m_globalExplosionContainer && m_objects[i] != m_localExplosionShared )
					{
						m_objects.Remove( i );
					}
					else
					{
						++i;
					}
				}
			}

			while( m_nextLocalExplosionTime < dt && m_nextLocalExplosion < m_localExplosionTransforms.size() )
			{
				XMVECTOR det;
				Matrix transform = m_localExplosionTransforms[m_nextLocalExplosion];
				transform.GetTranslation() = XMVector3TransformCoord( transform.GetTranslation(), XMMatrixInverse( &det, m_localTransform ) );
				SpawnLocalExplosion( transform );
				m_nextLocalExplosion++;
				if( m_nextLocalExplosion < m_localExplosionTransforms.size() )
				{
					m_nextLocalExplosionTime = m_localExplosionTimes[m_nextLocalExplosion];
				}
			}

			m_nextLocalExplosionTime -= dt;
		}
		if( m_globalExplosion )
		{
			m_countdownToGlobalExplosionStart -= dt;
			if( m_countdownToGlobalExplosionStart < 0 )
			{
				if( m_globalExplosionInstances.empty() )
				{
					IEveSpaceObjectChildPtr instance;
					if( BeClasses->CopyTo( m_globalExplosion, (IRoot**)&instance.p ) )
					{
						Quaternion rotation = Quaternion( 0.0, 0.0, 0.0, 1.0 );

						m_globalExplosionContainer.CreateInstance();
						m_globalExplosionContainer->SetupWithStaticRotation( &m_globalExplosionScaling, &rotation, &m_globalExplosionOffset, TR2_LOD_LOW );

						m_globalExplosionContainer->m_objects.Append( instance );

						m_objects.Append( m_globalExplosionContainer->GetRawRoot() );
						m_globalExplosionInstances.Append( instance );
					}

					// manually register the container, since it has now been created
					if( this->IsInRegistry() && !m_globalExplosionContainer->IsInRegistry() )
					{
						if( EveEntityPtr entity = BlueCastPtr( m_globalExplosionContainer ) )
						{
							entity->Register( this->GetComponentRegistry() );
						}
					}
				}
			}
		}
		if( !m_globalExplosions.empty() )
		{
			m_countdownToGlobalExplosionStart -= dt;
			if( m_countdownToGlobalExplosionStart < 0 )
			{
				if( m_globalExplosionInstances.empty() )
				{
					m_globalExplosionContainer.CreateInstance();

					for( auto it = m_globalExplosions.begin(); it != m_globalExplosions.end(); ++it )
					{
						auto globalExplosion = ( *it );
						IEveSpaceObjectChildPtr instance;
						if( BeClasses->CopyTo( globalExplosion, (IRoot**)&instance.p ) )
						{
							m_globalExplosionContainer->m_objects.Append( instance );
							m_globalExplosionInstances.Append( instance );
						}
					}

					// manually register the container, since it has now been created
					if( this->IsInRegistry() && !m_globalExplosionContainer->IsInRegistry() )
					{
						if( EveEntityPtr entity = BlueCastPtr( m_globalExplosionContainer ) )
						{
							entity->Register( this->GetComponentRegistry() );
						}
					}

					m_objects.Append( m_globalExplosionContainer->GetRawRoot() );
				}
			}
		}

		if( m_playTime > m_totalDuration )
		{
			Stop();
		}
	}
	EveChildContainer::UpdateSyncronous( updateContext, params );
}

void EveChildExplosion::RegisterWithQuadRenderer( Tr2QuadRenderer& quadRenderer )
{
	for( auto it = m_localExplosions.begin(); it != m_localExplosions.end(); ++it )
	{
		( *it )->RegisterWithQuadRenderer( quadRenderer );
	}
	if( m_localExplosionShared )
	{
		m_localExplosionShared->RegisterWithQuadRenderer( quadRenderer );
	}
	if( m_globalExplosion )
	{
		m_globalExplosion->RegisterWithQuadRenderer( quadRenderer );
	}
	for( auto it = m_globalExplosions.begin(); it != m_globalExplosions.end(); ++it )
	{
		( *it )->RegisterWithQuadRenderer( quadRenderer );
	}
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
			for( const Be::VarEntry* entry = type->mMemberTable; entry->mName; entry++ )
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
				if( IRoot* item = list->GetAt( i ) )
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
	Decompose( scale, t.rotation, t.position, transform );

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