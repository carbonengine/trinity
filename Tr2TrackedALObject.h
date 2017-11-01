////////////////////////////////////////////////////////////
//
//    Created:   May 2012
//    Copyright: CCP 2012
//

#pragma once
#ifndef Tr2TrackedALObject_H
#define Tr2TrackedALObject_H

#include "Tr2RenderContextEnum.h"

// Define TRACK_AL_RESOURCES as 0 to disable tracking
#ifndef TRACK_AL_RESOURCES
#define TRACK_AL_RESOURCES 1
#endif

enum Tr2ALMemoryType
{
	AL_MEMORY_VIDEO = 1 << 0,  // resources created on video card memory
	AL_MEMORY_MANAGED = 1 << 1, // resources created in device memory
};

typedef int Tr2ALMemoryTypes;

#if AL_TACK_RESOURCE_USAGE && TRACK_AL_RESOURCES
#define AL_UPDATE_RESOURCE_FRAME_USAGE( resource )	\
	{ if( ( resource ).IsValid() ) ( resource ).UpdateFrameUsed(); };
#else
#define AL_UPDATE_RESOURCE_FRAME_USAGE( resource ) ((void)(resource));
#endif

typedef uint32_t Tr2ObjectIdAL;

// --------------------------------------------------------------------------------------
// Description:
//   Base class of AL tracked objects. Maintains lists of live AL objects per object
//   type. Contains static functions to log all live AL objects and to return that
//   information as Python structure.
//   Individual AL classes descend from Tr2TrackedALObjectBase through class-specific
//   Tr2TrackedALObject proxies.
// See Also:
//   Tr2TrackedALObject
// --------------------------------------------------------------------------------------
class Tr2TrackedALObjectBase
{
public:
	Tr2TrackedALObjectBase();
	Tr2TrackedALObjectBase( Tr2TrackedALObjectBase&& );

	bool operator==( const Tr2TrackedALObjectBase& ) const;

	Tr2ObjectIdAL GetObjectId() const;

	void UpdateFrameUsed() const;

	template<typename Operation> static void GetAllObjectDescriptions( Tr2ALMemoryTypes flags, Operation& operation );
protected:
	void ChangeObjectId();

#if TRACK_AL_RESOURCES
	static CcpMutex& GetLiveObjectMutex();
	static std::set<Tr2TrackedALObjectBase*>& GetLiveObjects( Tr2RenderContextEnum::ObjectType type );

	template<Tr2RenderContextEnum::ObjectType Type> struct ObjectInfo;

	template<Tr2RenderContextEnum::ObjectType Type, typename Operation> class GetAllObjectDescriptionsHelper;
#endif

private:
	Tr2TrackedALObjectBase( const Tr2TrackedALObjectBase& ) /* = delete */;
	Tr2TrackedALObjectBase& operator=( const Tr2TrackedALObjectBase& ) /* = delete */;

#if TRACK_AL_RESOURCES && AL_TACK_RESOURCE_USAGE
	mutable uint64_t m_trackFrameUsed;
#endif

	Tr2ObjectIdAL m_objectId;
	static CcpAtomic<uint32_t> s_nextObjectId;
};

class Tr2ObjectALOpaquePointer
{
public:
	Tr2ObjectALOpaquePointer()
		:m_object( nullptr ),
		m_objectId( 0 )
	{
	}

	explicit Tr2ObjectALOpaquePointer( const Tr2TrackedALObjectBase* object )
		:m_object( object ),
		m_objectId( object ? object->GetObjectId() : 0 )
	{
	}

	Tr2ObjectALOpaquePointer& operator=( const Tr2TrackedALObjectBase* object )
	{
		m_object = object;
		m_objectId = object ? object->GetObjectId() : 0;
		return *this;
	}

	bool operator==( const Tr2ObjectALOpaquePointer& object ) const
	{
		return m_object == object.m_object && m_objectId == object.m_objectId;
	}

	bool operator==( const Tr2TrackedALObjectBase* object ) const
	{
		return m_object == object && ( !object || m_objectId == object->GetObjectId() );
	}

private:
	const Tr2TrackedALObjectBase* m_object;
	Tr2ObjectIdAL m_objectId;
};

inline bool operator==( const Tr2TrackedALObjectBase* object, const Tr2ObjectALOpaquePointer& pointer )
{
	return pointer == object;
}

#if( TRACK_AL_RESOURCES == 0 )

template<Tr2RenderContextEnum::ObjectType Type> 
class Tr2TrackedALObject: public Tr2TrackedALObjectBase
{
public:
	template<typename Operation> static void EnumerateResources( Operation& operation )
	{
	}
};

#else

// --------------------------------------------------------------------------------------
// Description:
//   Template object used for tracking live AL objects. Registers/unregisters itself
//   on construction/destruction.
// See Also:
//   Tr2TrackedALObjectBase
// --------------------------------------------------------------------------------------
template<Tr2RenderContextEnum::ObjectType Type> 
class Tr2TrackedALObject: public Tr2TrackedALObjectBase
{
public:
	template<typename Operation> static void EnumerateResources( Operation& operation );
protected:
	Tr2TrackedALObject();
	Tr2TrackedALObject( Tr2TrackedALObject<Type>&& other );
	~Tr2TrackedALObject();
};

#endif

#include "Tr2TrackedALObjectImpl.h"

#endif // Tr2TrackedALObject_H