// Copyright © 2023 CCP ehf.

#pragma once


// An event type (aka multi-cast delegate). Allows registering multiple listeners
// and broadcasting an event to them. Broadcasting is done using Broadcast or () operator.
// Currently for simplicity only methods of blue objects (IRoot descendants) are supported.
// Listeners can unregister themselves manually through UnRegister methods or on
// descruction (via blue weak pointer).
// It is prohibited to register/unregister listeners inside listener callbacks.
template <typename... Args>
struct Tr2Event
{
	template <typename C, void ( C::*Function )( Args... )>
	void RegisterListener( C* owner );

	template <typename C, void ( C::*Function )( Args... ) const>
	void RegisterListener( C* owner );

	template <typename C, void ( C::*Function )( Args... )>
	void UnregisterListener( C* owner );

	template <typename C, void ( C::*Function )( Args... ) const>
	void UnregisterListener( C* owner );

	template <typename C>
	void UnregisterListener( C* owner );

	void Broadcast( Args... args );
	void operator()();

private:
	using CallbackFunc = void ( * )( IRoot*, Args... );

	struct Listener
	{
		BlueWeakRefBase owner;
		CallbackFunc callback;
	};

	void Register( IRoot* owner, CallbackFunc func );
	void Unregister( IRoot* owner, CallbackFunc func );

	template <typename C, void ( C::*Function )( Args... )>
	static void MethodWrapper( IRoot* owner, Args... args );

	template <typename C, void ( C::*Function )( Args... ) const>
	static void ConstMethodWrapper( IRoot* owner, Args... args );

	std::vector<Listener> m_listeners;
#if CCP_ASSERT_ENABLED
	bool m_inBroadcast = false;
#endif
};




template <typename... Args>
template <typename C, void ( C::*Function )( Args... )>
void Tr2Event<Args...>::RegisterListener( C* owner )
{
	CCP_ASSERT_M( !m_inBroadcast, "Trying to register a listener while broadcasting an event" );
	Register( owner->GetRawRoot(), &MethodWrapper<C, Function> );
}

template <typename... Args>
template <typename C, void ( C::*Function )( Args... ) const>
void Tr2Event<Args...>::RegisterListener( C* owner )
{
	CCP_ASSERT_M( !m_inBroadcast, "Trying to register a listener while broadcasting an event" );
	Register( owner->GetRawRoot(), &ConstMethodWrapper<C, Function> );
}

template <typename... Args>
template <typename C, void ( C::*Function )( Args... )>
void Tr2Event<Args...>::UnregisterListener( C* owner )
{
	CCP_ASSERT_M( !m_inBroadcast, "Trying to unregister a listener while broadcasting an event" );
	Unregister( owner->GetRawRoot(), &MethodWrapper<C, Function> );
}

template <typename... Args>
template <typename C, void ( C::*Function )( Args... ) const>
void Tr2Event<Args...>::UnregisterListener( C* owner )
{
	CCP_ASSERT_M( !m_inBroadcast, "Trying to unregister a listener while broadcasting an event" );
	Unregister( owner->GetRawRoot(), &ConstMethodWrapper<C, Function> );
}

template <typename... Args>
template <typename C>
void Tr2Event<Args...>::UnregisterListener( C* owner )
{
	CCP_ASSERT_M( !m_inBroadcast, "Trying to unregister a listener while broadcasting an event" );
	auto iroot = owner->GetRawRoot();
	auto removed = remove_if( begin( m_listeners ), end( m_listeners ), [iroot]( auto& x ) { return x.owner == iroot; } );
	CCP_ASSERT_M( removed != end( m_listeners ) || iroot->GetRefCount() == 0,
				  "Trying to unregister a listener to the event when it was not registered" );
	m_listeners.erase( removed, end( m_listeners ) );
}

template <typename... Args>
void Tr2Event<Args...>::Broadcast( Args... args )
{
#if CCP_ASSERT_ENABLED
	m_inBroadcast = true;
	ON_BLOCK_EXIT( [&] { m_inBroadcast = false; } );
#endif
	m_listeners.erase( std::remove_if( begin( m_listeners ), end( m_listeners ), []( auto& x ) { return !x.owner; } ), end( m_listeners ) );
	std::for_each( begin( m_listeners ), end( m_listeners ), [&args...]( auto& x ) {
		IRoot* owner = x.owner;
		owner->Lock();
		( *x.callback )( owner, args... );
		owner->Unlock();
	} );
}

template <typename... Args>
void Tr2Event<Args...>::operator()()
{
	Broadcast();
}

template <typename... Args>
void Tr2Event<Args...>::Register( IRoot* owner, CallbackFunc func )
{
	CCP_ASSERT_M( find_if( begin( m_listeners ), end( m_listeners ), [owner, func]( auto& x ) { return x.owner == owner && x.callback == func; } ) == end( m_listeners ),
				  "Trying to register a listener to the event when it is already registered" );
	m_listeners.push_back( { owner, func } );
}

template <typename... Args>
void Tr2Event<Args...>::Unregister( IRoot* owner, CallbackFunc func )
{
	auto found = find_if( begin( m_listeners ), end( m_listeners ), [owner, func]( auto& x ) { return x.owner == owner && x.callback == func; } );
	CCP_ASSERT_M( found != end( m_listeners ),
				  "Trying to unregister a listener to the event when it was not registered" );
	m_listeners.erase( found );
}

template <typename... Args>
template <typename C, void ( C::*Function )( Args... )>
void Tr2Event<Args...>::MethodWrapper( IRoot* owner, Args... args )
{
	( reinterpret_cast<C*>( owner )->*Function )( args... );
}

template <typename... Args>
template <typename C, void ( C::*Function )( Args... ) const>
void Tr2Event<Args...>::ConstMethodWrapper( IRoot* owner, Args... args )
{
	( reinterpret_cast<C*>( owner )->*Function )( args... );
}
