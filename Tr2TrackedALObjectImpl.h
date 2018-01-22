////////////////////////////////////////////////////////////
//
//    Created:   May 2012
//    Copyright: CCP 2012
//

// This is a template implementation file for Tr2TrackedALObject.h
// header.

// --------------------------------------------------------------------------------------
// Description:
//   Description structure of AL object type. This is a generic fallback structure, 
//   specialized templates provide more per-AL-class information.
// See Also:
//   Tr2TrackedALObjectBase
// --------------------------------------------------------------------------------------
template<Tr2RenderContextEnum::ObjectType Type>
struct Tr2TrackedALObjectBase::ObjectInfo
{
	// ----------------------------------------------------------------------------------
	// Description:
	//   AL object type this information structure is applied to.
	// ----------------------------------------------------------------------------------
	typedef Tr2TrackedALObjectBase ObjectType;

	// ----------------------------------------------------------------------------------
	// Description:
	//   Returns name of AL type this structure applies to.
	// Return Value:
	//   AL type name
	// ----------------------------------------------------------------------------------
	static const char* GetName()
	{
		static char name[] = "Unknown(   )";
		name[9] = '0' + Type;
		return name;
	}
};

// --------------------------------------------------------------------------------------
// Description:
//   Description structure for Tr2RenderContextAL. 
// See Also:
//   Tr2TrackedALObjectBase
// --------------------------------------------------------------------------------------
template<>
struct Tr2TrackedALObjectBase::ObjectInfo<Tr2RenderContextEnum::OT_RENDER_CONTEXT>
{
	// ----------------------------------------------------------------------------------
	// Description:
	//   AL object type this information structure is applied to.
	// ----------------------------------------------------------------------------------
	typedef Tr2TrackedALObjectBase ObjectType;

	static const char* GetName();
	static bool GetDescription( Tr2ALMemoryTypes flags, ObjectType* object, std::map<std::string, uint32_t>& description );
	static void DestroyObject( Tr2ALMemoryTypes flags, ObjectType* object );
};

class Tr2ConstantBufferAL;
class Tr2ShaderAL;
class Tr2SamplerStateAL;
class Tr2TextureAL;
class Tr2VertexLayoutAL;
class Tr2OcclusionQueryAL;
class Tr2SwapChainAL;
class Tr2FenceAL;
class Tr2GpuTimerAL;
class Tr2ShaderProgramAL;
namespace TrinityALImpl
{
	class Tr2SamplerStateAL;
	class Tr2ResourceSetAL;
	class Tr2BufferAL;
	class Tr2TextureAL;
}

// --------------------------------------------------------------------------------------
// Description:
//   Description structure for Tr2ConstantBufferAL. 
// See Also:
//   Tr2TrackedALObjectBase
// --------------------------------------------------------------------------------------
template<>
struct Tr2TrackedALObjectBase::ObjectInfo<Tr2RenderContextEnum::OT_CONSTANT_BUFFER>
{
	// ----------------------------------------------------------------------------------
	// Description:
	//   AL object type this information structure is applied to.
	// ----------------------------------------------------------------------------------
	typedef Tr2ConstantBufferAL ObjectType;

	static const char* GetName();
	static bool GetDescription( Tr2ALMemoryTypes flags, ObjectType* object, std::map<std::string, uint32_t>& description );
	static void DestroyObject( Tr2ALMemoryTypes flags, ObjectType* object );
};

// --------------------------------------------------------------------------------------
// Description:
//   Description structure for Tr2ShaderAL. 
// See Also:
//   Tr2TrackedALObjectBase
// --------------------------------------------------------------------------------------
template<>
struct Tr2TrackedALObjectBase::ObjectInfo<Tr2RenderContextEnum::OT_SHADER>
{
	// ----------------------------------------------------------------------------------
	// Description:
	//   AL object type this information structure is applied to.
	// ----------------------------------------------------------------------------------
	typedef Tr2ShaderAL ObjectType;

	static const char* GetName();
	static bool GetDescription( Tr2ALMemoryTypes flags, ObjectType* object, std::map<std::string, uint32_t>& description );
	static void DestroyObject( Tr2ALMemoryTypes flags, ObjectType* object );
};

// --------------------------------------------------------------------------------------
// Description:
//   Description structure for Tr2SamplerStateAL. 
// See Also:
//   Tr2TrackedALObjectBase
// --------------------------------------------------------------------------------------
template<>
struct Tr2TrackedALObjectBase::ObjectInfo<Tr2RenderContextEnum::OT_SAMPLER_STATE>
{
	// ----------------------------------------------------------------------------------
	// Description:
	//   AL object type this information structure is applied to.
	// ----------------------------------------------------------------------------------
	typedef TrinityALImpl::Tr2SamplerStateAL ObjectType;

	static const char* GetName();
	static bool GetDescription( Tr2ALMemoryTypes flags, ObjectType* object, std::map<std::string, uint32_t>& description );
	static void DestroyObject( Tr2ALMemoryTypes flags, ObjectType* object );
};

// --------------------------------------------------------------------------------------
// Description:
//   Description structure for Tr2TextureAL. 
// See Also:
//   Tr2TrackedALObjectBase
// --------------------------------------------------------------------------------------
template<>
struct Tr2TrackedALObjectBase::ObjectInfo<Tr2RenderContextEnum::OT_TEXTURE>
{
	// ----------------------------------------------------------------------------------
	// Description:
	//   AL object type this information structure is applied to.
	// ----------------------------------------------------------------------------------
	typedef TrinityALImpl::Tr2TextureAL ObjectType;

	static const char* GetName();
	static bool GetDescription( Tr2ALMemoryTypes flags, ObjectType* object, std::map<std::string, uint32_t>& description );
	static void DestroyObject( Tr2ALMemoryTypes flags, ObjectType* object );
private:
	static unsigned GetSizeEstimate( ObjectType* object );
};

// --------------------------------------------------------------------------------------
// Description:
//   Description structure for Tr2VertexLayoutAL. 
// See Also:
//   Tr2TrackedALObjectBase
// --------------------------------------------------------------------------------------
template<>
struct Tr2TrackedALObjectBase::ObjectInfo<Tr2RenderContextEnum::OT_VERTEX_LAYOUT>
{
	// ----------------------------------------------------------------------------------
	// Description:
	//   AL object type this information structure is applied to.
	// ----------------------------------------------------------------------------------
	typedef Tr2VertexLayoutAL ObjectType;

	static const char* GetName();
	static bool GetDescription( Tr2ALMemoryTypes flags, ObjectType* object, std::map<std::string, uint32_t>& description );
	static void DestroyObject( Tr2ALMemoryTypes flags, ObjectType* object );
};

// --------------------------------------------------------------------------------------
// Description:
//   Description structure for Tr2OcclusionQueryAL. 
// See Also:
//   Tr2TrackedALObjectBase
// --------------------------------------------------------------------------------------
template<>
struct Tr2TrackedALObjectBase::ObjectInfo<Tr2RenderContextEnum::OT_OCCLUSION_QUERY>
{
	// ----------------------------------------------------------------------------------
	// Description:
	//   AL object type this information structure is applied to.
	// ----------------------------------------------------------------------------------
	typedef Tr2OcclusionQueryAL ObjectType;

	static const char* GetName();
	static bool GetDescription( Tr2ALMemoryTypes flags, ObjectType* object, std::map<std::string, uint32_t>& description );
	static void DestroyObject( Tr2ALMemoryTypes flags, ObjectType* object );
};

// --------------------------------------------------------------------------------------
// Description:
//   Description structure for Tr2SwapChainAL. 
// See Also:
//   Tr2TrackedALObjectBase
// --------------------------------------------------------------------------------------
template<>
struct Tr2TrackedALObjectBase::ObjectInfo<Tr2RenderContextEnum::OT_SWAP_CHAIN>
{
	// ----------------------------------------------------------------------------------
	// Description:
	//   AL object type this information structure is applied to.
	// ----------------------------------------------------------------------------------
	typedef Tr2SwapChainAL ObjectType;

	static const char* GetName();
	static bool GetDescription( Tr2ALMemoryTypes flags, ObjectType* object, std::map<std::string, uint32_t>& description );
	static void DestroyObject( Tr2ALMemoryTypes flags, ObjectType* object );
};

// --------------------------------------------------------------------------------------
// Description:
//   Description structure for Tr2FenceAL. 
// See Also:
//   Tr2TrackedALObjectBase
// --------------------------------------------------------------------------------------
template<>
struct Tr2TrackedALObjectBase::ObjectInfo<Tr2RenderContextEnum::OT_FENCE>
{
	// ----------------------------------------------------------------------------------
	// Description:
	//   AL object type this information structure is applied to.
	// ----------------------------------------------------------------------------------
	typedef Tr2FenceAL ObjectType;

	static const char* GetName();
	static bool GetDescription( Tr2ALMemoryTypes flags, ObjectType* object, std::map<std::string, uint32_t>& description );
	static void DestroyObject( Tr2ALMemoryTypes flags, ObjectType* object );
};

// --------------------------------------------------------------------------------------
// Description:
//   Description structure for Tr2GpuTimer. 
// See Also:
//   Tr2TrackedALObjectBase
// --------------------------------------------------------------------------------------
template<>
struct Tr2TrackedALObjectBase::ObjectInfo<Tr2RenderContextEnum::OT_TIMER>
{
	// ----------------------------------------------------------------------------------
	// Description:
	//   AL object type this information structure is applied to.
	// ----------------------------------------------------------------------------------
	typedef Tr2GpuTimerAL ObjectType;

	static const char* GetName();
	static bool GetDescription( Tr2ALMemoryTypes flags, ObjectType* object, std::map<std::string, uint32_t>& description );
	static void DestroyObject( Tr2ALMemoryTypes flags, ObjectType* object );
};


// --------------------------------------------------------------------------------------
template<>
struct Tr2TrackedALObjectBase::ObjectInfo<Tr2RenderContextEnum::OT_SHADER_PROGRAM>
{
	// ----------------------------------------------------------------------------------
	// Description:
	//   AL object type this information structure is applied to.
	// ----------------------------------------------------------------------------------
	typedef Tr2ShaderProgramAL ObjectType;

	static const char* GetName();
	static bool GetDescription( Tr2ALMemoryTypes flags, ObjectType* object, std::map<std::string, uint32_t>& description );
	static void DestroyObject( Tr2ALMemoryTypes flags, ObjectType* object );
};


// --------------------------------------------------------------------------------------
template<>
struct Tr2TrackedALObjectBase::ObjectInfo<Tr2RenderContextEnum::OT_RESOURCE_SET>
{
	// ----------------------------------------------------------------------------------
	// Description:
	//   AL object type this information structure is applied to.
	// ----------------------------------------------------------------------------------
	typedef TrinityALImpl::Tr2ResourceSetAL ObjectType;

	static const char* GetName();
	static bool GetDescription( Tr2ALMemoryTypes flags, ObjectType* object, std::map<std::string, uint32_t>& description );
	static void DestroyObject( Tr2ALMemoryTypes flags, ObjectType* object );
};


// --------------------------------------------------------------------------------------
template<>
struct Tr2TrackedALObjectBase::ObjectInfo<Tr2RenderContextEnum::OT_BUFFER>
{
	// ----------------------------------------------------------------------------------
	// Description:
	//   AL object type this information structure is applied to.
	// ----------------------------------------------------------------------------------
	typedef TrinityALImpl::Tr2BufferAL ObjectType;

	static const char* GetName();
	static bool GetDescription( Tr2ALMemoryTypes flags, ObjectType* object, std::map<std::string, uint32_t>& description );
	static void DestroyObject( Tr2ALMemoryTypes flags, ObjectType* object );
};


// --------------------------------------------------------------------------------------
// Description:
//   Performs a given operation on all live AL resources.
// Arguments:
//   operation - Operation to perform on a resource
// --------------------------------------------------------------------------------------
template<typename Operation> 
void Tr2TrackedALObjectBase::GetAllObjectDescriptions( Tr2ALMemoryTypes flags, Operation& operation )
{
	GetAllObjectDescriptionsHelper<Tr2RenderContextEnum::ObjectType( Tr2RenderContextEnum::OBJECT_TYPE_COUNT - 1 ), Operation>( flags, operation ).GetDescriptions();
}

// --------------------------------------------------------------------------------------
// Description:
//   Recursive (on Type parameter) template structure for enumerating live AL object 
//   descriptions. 
// --------------------------------------------------------------------------------------
template<Tr2RenderContextEnum::ObjectType Type, typename Operation> 
class Tr2TrackedALObjectBase::GetAllObjectDescriptionsHelper
{
public:
	GetAllObjectDescriptionsHelper( Tr2ALMemoryTypes flags, Operation& operation )
		:m_operation( operation ),
		m_flags( flags )
	{
	}
	// ----------------------------------------------------------------------------------
	// Description:
	//   Performs operation on all live resources of the given type and recursively on
	//   all types smaller than this type.
	// ----------------------------------------------------------------------------------
	void GetDescriptions()
	{
		GetAllObjectDescriptionsHelper<Tr2RenderContextEnum::ObjectType( Type - 1 ), Operation>( m_flags, m_operation ).GetDescriptions();
		Tr2TrackedALObject<Type>::EnumerateResources( *this );
	}
	// ----------------------------------------------------------------------------------
	// Description:
	//   Called by Tr2TrackedALObject<Type>::EnumerateResources. Gets object description
	//   and calls given operation with object description.
	// Arguments:
	//   object - Live AL object
	// ----------------------------------------------------------------------------------
	void operator()( typename Tr2TrackedALObjectBase::ObjectInfo<Type>::ObjectType* object )
	{
		std::map<std::string, uint32_t> description;
		if( Tr2TrackedALObjectBase::ObjectInfo<Type>::GetDescription( m_flags, object, description ) )
		{
			m_operation( 
				Type, 
				Tr2TrackedALObjectBase::ObjectInfo<Type>::GetName(), 
				object, 
				description );
		}
	}
private:
	Tr2ALMemoryTypes m_flags;
	Operation& m_operation;
};

// --------------------------------------------------------------------------------------
// Description:
//   Base of recusion (on Type parameter) for GetAllObjectDescriptionsHelper.
// --------------------------------------------------------------------------------------
template<typename Operation> 
class Tr2TrackedALObjectBase::GetAllObjectDescriptionsHelper<Tr2RenderContextEnum::ObjectType( 0 ), Operation>
{	
public:
	static const Tr2RenderContextEnum::ObjectType Type = Tr2RenderContextEnum::ObjectType( 0 );

	GetAllObjectDescriptionsHelper( Tr2ALMemoryTypes flags, Operation& operation )
		:m_operation( operation ),
		m_flags( flags )
	{
	}
	// ----------------------------------------------------------------------------------
	// Description:
	//   Performs operation on all live resources of the given type and recursively on
	//   all types smaller than this type.
	// ----------------------------------------------------------------------------------
	void GetDescriptions()
	{
		Tr2TrackedALObject<Type>::EnumerateResources( *this );
	}
	// ----------------------------------------------------------------------------------
	// Description:
	//   Called by Tr2TrackedALObject<Type>::EnumerateResources. Gets object description
	//   and calls given operation with object description.
	// Arguments:
	//   object - Live AL object
	// ----------------------------------------------------------------------------------
	void operator()( typename Tr2TrackedALObjectBase::ObjectInfo<Type>::ObjectType* object )
	{
		std::map<std::string, uint32_t> description;
		if( Tr2TrackedALObjectBase::ObjectInfo<Type>::GetDescription( m_flags, object, description ) )
		{
			m_operation( 
				Type, 
				Tr2TrackedALObjectBase::ObjectInfo<Type>::GetName(), 
				object, 
				description );
		}
	}
private:
	Tr2ALMemoryTypes m_flags;
	Operation& m_operation;
};



// --------------------------------------------------------------------------------------
// Description:
//   Tr2TrackedALObject default constructor
// --------------------------------------------------------------------------------------
template<Tr2RenderContextEnum::ObjectType Type>
Tr2TrackedALObject<Type>::Tr2TrackedALObject()
{
	CcpAutoMutex autoMutex( Tr2TrackedALObjectBase::GetLiveObjectMutex() );
	GetLiveObjects( Type ).insert( this );
}

// --------------------------------------------------------------------------------------
// Description:
//   Tr2TrackedALObject move constructor
// --------------------------------------------------------------------------------------
template<Tr2RenderContextEnum::ObjectType Type>
Tr2TrackedALObject<Type>::Tr2TrackedALObject( Tr2TrackedALObject<Type>&& other )
{
	CcpAutoMutex autoMutex( Tr2TrackedALObjectBase::GetLiveObjectMutex() );
	GetLiveObjects( Type ).insert( this );
}

// --------------------------------------------------------------------------------------
// Description:
//   Tr2TrackedALObject destructor
// --------------------------------------------------------------------------------------
template<Tr2RenderContextEnum::ObjectType Type>
Tr2TrackedALObject<Type>::~Tr2TrackedALObject()
{
	CcpAutoMutex autoMutex( Tr2TrackedALObjectBase::GetLiveObjectMutex() );
	GetLiveObjects( Type ).erase( this );
}

// --------------------------------------------------------------------------------------
// Description:
//   Helper function that appies given operator to all live objects of the given type.
// --------------------------------------------------------------------------------------
template<Tr2RenderContextEnum::ObjectType Type> template<typename Operation> 
void Tr2TrackedALObject<Type>::EnumerateResources( Operation& operation )
{
	CcpAutoMutex autoMutex( Tr2TrackedALObjectBase::GetLiveObjectMutex() );
	for( auto it = Tr2TrackedALObject<Type>::GetLiveObjects( Type ).begin(); 
		it != Tr2TrackedALObject<Type>::GetLiveObjects( Type ).end(); 
		++it )
	{
		typename ObjectInfo<Type>::ObjectType* object = static_cast<typename ObjectInfo<Type>::ObjectType*>( *it );
		operation( object );
	}
}

