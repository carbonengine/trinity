////////////////////////////////////////////////////////////
//
//    Created:   May 2012
//    Copyright: CCP 2012
//

#include "StdAfx.h"
#include "Tr2TrackedALObject.h"

#include "include/TrinityAL.h"

#if TRACK_AL_RESOURCES

#include "ALLog.h"

#if AL_TACK_RESOURCE_USAGE
uint64_t g_trackCurrentFrame = 0;

#define REPORT_LAST_FRAME_USED description["lastFrameUsed"] = uint32_t( g_trackCurrentFrame - object->m_trackFrameUsed );

#else

#define REPORT_LAST_FRAME_USED

#endif

#endif

CcpAtomic<uint32_t> Tr2TrackedALObjectBase::s_nextObjectId( 1 );


Tr2TrackedALObjectBase::Tr2TrackedALObjectBase()
	:m_objectId( 0 )
#if AL_TACK_RESOURCE_USAGE
	, m_trackFrameUsed( 0 )
#endif
{
}

Tr2TrackedALObjectBase::Tr2TrackedALObjectBase( Tr2TrackedALObjectBase&& )
	:m_objectId( 0 )
#if AL_TACK_RESOURCE_USAGE
	, m_trackFrameUsed( 0 )
#endif
{
}

Tr2ObjectIdAL Tr2TrackedALObjectBase::GetObjectId() const
{
	return m_objectId;
}

void Tr2TrackedALObjectBase::UpdateFrameUsed() const
{
#if AL_TACK_RESOURCE_USAGE && TRACK_AL_RESOURCES
	extern uint64_t g_trackCurrentFrame; 
	m_trackFrameUsed = g_trackCurrentFrame;
#endif
}

void Tr2TrackedALObjectBase::ChangeObjectId()
{
	m_objectId = s_nextObjectId++;
}

// --------------------------------------------------------------------------------------
// Description:
//   Fallback comparison operator for AL objects. Individual AL classes provide more
//   sensible operators.
// Arguments:
//   other - AL resource to compare to
// Return value:
//   true If objects are the same
//   false Otherwise
// --------------------------------------------------------------------------------------
bool Tr2TrackedALObjectBase::operator==( const Tr2TrackedALObjectBase& other ) const
{
	return this == &other;
}


#if TRACK_AL_RESOURCES

// --------------------------------------------------------------------------------------
// Description:
//   Returns a static set of live AL objects of given type.
// Arguments:
//   type - type of AL objects
// Return value:
//   set of live AL objects
// --------------------------------------------------------------------------------------
std::set<Tr2TrackedALObjectBase*>& Tr2TrackedALObjectBase::GetLiveObjects( Tr2RenderContextEnum::ObjectType type )
{
	static std::set<Tr2TrackedALObjectBase*> s_liveObjects[Tr2RenderContextEnum::OBJECT_TYPE_COUNT];
	return s_liveObjects[type];
}

// --------------------------------------------------------------------------------------
// Description:
//   Returns a mutex shared by all Tr2TrackedALObjectBase instances and used to access
//   GetLiveObjects.
// Return value:
//   Mutex to be used to access GetLiveObjects set
// --------------------------------------------------------------------------------------
CcpMutex& Tr2TrackedALObjectBase::GetLiveObjectMutex()
{
	static CcpMutex s_mutex( "Tr2TrackedALObjectBase", "s_mutex" );
	return s_mutex;
}

// ----------------------------------------------------------------------------------
// Description:
//   Returns name of AL type this structure applies to.
// Return Value:
//   AL type name
// ----------------------------------------------------------------------------------
const char* Tr2TrackedALObjectBase::ObjectInfo<Tr2RenderContextEnum::OT_RENDER_CONTEXT>::GetName()
{
	return "Tr2RenderContextAL";
}

// ----------------------------------------------------------------------------------
// Description:
//   Returns information about the AL object (as string -> int values).
// Arguments:
//   flags - Resource memory class filter
//   object - AL object
//   description - (out) information on AL object (as string -> int values)
// Return value:
//   true If object is alive and matches memory filter
//   false Otherwise
// ----------------------------------------------------------------------------------
bool Tr2TrackedALObjectBase::ObjectInfo<Tr2RenderContextEnum::OT_RENDER_CONTEXT>::GetDescription( 
	Tr2ALMemoryTypes, 
	ObjectType*, 
	std::map<std::string, uint32_t>& )
{
	return false;
}


// ----------------------------------------------------------------------------------
// Description:
//   Returns name of AL type this structure applies to.
// Return Value:
//   AL type name
// ----------------------------------------------------------------------------------
const char* Tr2TrackedALObjectBase::ObjectInfo<Tr2RenderContextEnum::OT_CONSTANT_BUFFER>::GetName()
{
	return "Tr2ConstantBufferAL";
}

// ----------------------------------------------------------------------------------
// Description:
//   Returns information about the AL object (as string -> int values).
// Arguments:
//   flags - Resource memory class filter
//   object - AL object
//   description - (out) information on AL object (as string -> int values)
// Return value:
//   true If object is alive and matches memory filter
//   false Otherwise
// ----------------------------------------------------------------------------------
bool Tr2TrackedALObjectBase::ObjectInfo<Tr2RenderContextEnum::OT_CONSTANT_BUFFER>::GetDescription( 
	Tr2ALMemoryTypes flags, 
	ObjectType* object, 
	std::map<std::string, uint32_t>& description )
{
	if( object->IsValid() && ( object->GetMemoryClass() & flags ) != 0 )
	{
		REPORT_LAST_FRAME_USED;
		description["memory"] = object->GetMemoryClass();
		description["size"] = (uint32_t)object->GetSize();
		return true;
	}
	return false;
}


// ----------------------------------------------------------------------------------
// Description:
//   Returns name of AL type this structure applies to.
// Return Value:
//   AL type name
// ----------------------------------------------------------------------------------
const char* Tr2TrackedALObjectBase::ObjectInfo<Tr2RenderContextEnum::OT_DEPTH_STENCIL>::GetName()
{
	return "Tr2DepthStencilAL";
}

// ----------------------------------------------------------------------------------
// Description:
//   Returns information about the AL object (as string -> int values).
// Arguments:
//   flags - Resource memory class filter
//   object - AL object
//   description - (out) information on AL object (as string -> int values)
// Return value:
//   true If object is alive and matches memory filter
//   false Otherwise
// ----------------------------------------------------------------------------------
bool Tr2TrackedALObjectBase::ObjectInfo<Tr2RenderContextEnum::OT_DEPTH_STENCIL>::GetDescription( 
	Tr2ALMemoryTypes flags, 
	ObjectType* object, 
	std::map<std::string, uint32_t>& description )
{
	if( object->IsValid() && ( object->GetMemoryClass() & flags ) != 0 )
	{
		REPORT_LAST_FRAME_USED;
		description["memory"] = object->GetMemoryClass();
		description["size"] = GetSizeEstimate( object );
		description["width"] = object->GetWidth();
		description["height"] = object->GetHeight();
		description["format"] = uint32_t( object->GetFormat() );
		description["msaaType"] = object->GetMsaaDesc().samples;
		description["msaaQuality"] = object->GetMsaaDesc().quality;
		return true;
	}
	return false;
}

// ----------------------------------------------------------------------------------
// Description:
//   Returns estimated size of depth buffer.
// Arguments:
//   object - AL object
// Return value:
//   estimated size of depth buffer
// ----------------------------------------------------------------------------------
unsigned Tr2TrackedALObjectBase::ObjectInfo<Tr2RenderContextEnum::OT_DEPTH_STENCIL>::GetSizeEstimate( ObjectType* object )
{
	unsigned bpp = 0;
	switch( object->GetFormat() )
	{
	case Tr2RenderContextEnum::DSFMT_D24S8:
	case Tr2RenderContextEnum::DSFMT_D24X8:
	case Tr2RenderContextEnum::DSFMT_D24FS8:
	case Tr2RenderContextEnum::DSFMT_D32F:
	case Tr2RenderContextEnum::DSFMT_D32:
	case Tr2RenderContextEnum::DSFMT_READABLE:
	case Tr2RenderContextEnum::DSFMT_AUTO:
	case Tr2RenderContextEnum::DSFMT_D24X4S4:
	case Tr2RenderContextEnum::DSFMT_D32F_LOCKABLE:
		bpp = 4;
		break;
		
	case Tr2RenderContextEnum::DSFMT_D16_LOCKABLE:
	case Tr2RenderContextEnum::DSFMT_D15S1:	
	case Tr2RenderContextEnum::DSFMT_D16:
		bpp = 2;
		break;

	default:
		bpp = 0;
		break;
	}
	return object->GetWidth() * object->GetHeight() * bpp;
}


const char* Tr2TrackedALObjectBase::ObjectInfo<Tr2RenderContextEnum::OT_INDEX_BUFFER>::GetName()
{
	return "Tr2IndexBufferAL";
}

// ----------------------------------------------------------------------------------
// Description:
//   Returns information about the AL object (as string -> int values).
// Arguments:
//   flags - Resource memory class filter
//   object - AL object
//   description - (out) information on AL object (as string -> int values)
// Return value:
//   true If object is alive and matches memory filter
//   false Otherwise
// ----------------------------------------------------------------------------------
bool Tr2TrackedALObjectBase::ObjectInfo<Tr2RenderContextEnum::OT_INDEX_BUFFER>::GetDescription( 
	Tr2ALMemoryTypes flags, 
	ObjectType* object, 
	std::map<std::string, uint32_t>& description )
{
	if( object->IsValid() && ( object->GetMemoryClass() & flags ) != 0 )
	{
		REPORT_LAST_FRAME_USED;
		description["memory"] = object->GetMemoryClass();
		description["size"] = object->GetTotalSizeInBytes();
		return true;
	}
	return false;
}


// ----------------------------------------------------------------------------------
// Description:
//   Returns name of AL type this structure applies to.
// Return Value:
//   AL type name
// ----------------------------------------------------------------------------------
const char* Tr2TrackedALObjectBase::ObjectInfo<Tr2RenderContextEnum::OT_RENDER_TARGET>::GetName()
{
	return "Tr2RenderTargetAL";
}

// ----------------------------------------------------------------------------------
// Description:
//   Returns information about the AL object (as string -> int values).
// Arguments:
//   flags - Resource memory class filter
//   object - AL object
//   description - (out) information on AL object (as string -> int values)
// Return value:
//   true If object is alive and matches memory filter
//   false Otherwise
// ----------------------------------------------------------------------------------
bool Tr2TrackedALObjectBase::ObjectInfo<Tr2RenderContextEnum::OT_RENDER_TARGET>::GetDescription( 
	Tr2ALMemoryTypes flags, 
	ObjectType* object, 
	std::map<std::string, uint32_t>& description )
{
	if( object->IsValid() && ( object->GetMemoryClass() & flags ) != 0 )
	{
		REPORT_LAST_FRAME_USED;
		description["memory"] = object->GetMemoryClass();
		description["size"] = GetSizeEstimate( object );
		description["width"] = object->GetWidth();
		description["height"] = object->GetHeight();
		description["mipLevels"] = object->GetMipCount();
		description["trueMipLevels"] = object->GetTrueMipCount();
		description["format"] = uint32_t( object->GetFormat() );
		description["msaaType"] = object->GetMsaaDesc().samples;
		description["msaaQuality"] = object->GetMsaaDesc().quality;
		return true;
	}
	return false;
}

// ----------------------------------------------------------------------------------
// Description:
//   Returns estimated size of render target.
// Arguments:
//   object - AL object
// Return value:
//   estimated size of render target
// ----------------------------------------------------------------------------------
unsigned Tr2TrackedALObjectBase::ObjectInfo<Tr2RenderContextEnum::OT_RENDER_TARGET>::GetSizeEstimate( ObjectType* object )
{
	unsigned size = 0;
	for( unsigned i = 0; i < object->GetTrueMipCount(); ++i )
	{
		size += object->GetMipSize( i );
	}
	return size;
}


// ----------------------------------------------------------------------------------
// Description:
//   Returns name of AL type this structure applies to.
// Return Value:
//   AL type name
// ----------------------------------------------------------------------------------
const char* Tr2TrackedALObjectBase::ObjectInfo<Tr2RenderContextEnum::OT_SHADER>::GetName()
{
	return "Tr2ShaderAL";
}

// ----------------------------------------------------------------------------------
// Description:
//   Returns information about the AL object (as string -> int values).
// Arguments:
//   flags - Resource memory class filter
//   object - AL object
//   description - (out) information on AL object (as string -> int values)
// Return value:
//   true If object is alive and matches memory filter
//   false Otherwise
// ----------------------------------------------------------------------------------
bool Tr2TrackedALObjectBase::ObjectInfo<Tr2RenderContextEnum::OT_SHADER>::GetDescription( 
	Tr2ALMemoryTypes flags, 
	ObjectType* object, 
	std::map<std::string, uint32_t>& description )
{
	if( object->IsValid() && ( object->GetMemoryClass() & flags ) != 0 )
	{
		REPORT_LAST_FRAME_USED;
		description["type"] = object->GetType();
		const void* bytecode;
		unsigned size;
		object->GetBytecode( bytecode, size );
		description["size"] = size;
		return true;
	}
	return false;
}


// ----------------------------------------------------------------------------------
// Description:
//   Returns name of AL type this structure applies to.
// Return Value:
//   AL type name
// ----------------------------------------------------------------------------------
const char* Tr2TrackedALObjectBase::ObjectInfo<Tr2RenderContextEnum::OT_SAMPLER_STATE>::GetName()
{
	return "Tr2SamplerStateAL";
}

// ----------------------------------------------------------------------------------
// Description:
//   Returns information about the AL object (as string -> int values).
// Arguments:
//   flags - Resource memory class filter
//   object - AL object
//   description - (out) information on AL object (as string -> int values)
// Return value:
//   true If object is alive and matches memory filter
//   false Otherwise
// ----------------------------------------------------------------------------------
bool Tr2TrackedALObjectBase::ObjectInfo<Tr2RenderContextEnum::OT_SAMPLER_STATE>::GetDescription( 
	Tr2ALMemoryTypes flags, 
	ObjectType* object, 
	std::map<std::string, uint32_t>& description )
{
	if( object->IsValid() && ( object->GetMemoryClass() & flags ) )
	{
		REPORT_LAST_FRAME_USED;
		return true;
	}
	return false;
}


// ----------------------------------------------------------------------------------
// Description:
//   Returns name of AL type this structure applies to.
// Return Value:
//   AL type name
// ----------------------------------------------------------------------------------
const char* Tr2TrackedALObjectBase::ObjectInfo<Tr2RenderContextEnum::OT_TEXTURE>::GetName()
{
	return "Tr2TextureAL";
}

// ----------------------------------------------------------------------------------
// Description:
//   Returns information about the AL object (as string -> int values).
// Arguments:
//   flags - Resource memory class filter
//   object - AL object
//   description - (out) information on AL object (as string -> int values)
// Return value:
//   true If object is alive and matches memory filter
//   false Otherwise
// ----------------------------------------------------------------------------------
bool Tr2TrackedALObjectBase::ObjectInfo<Tr2RenderContextEnum::OT_TEXTURE>::GetDescription( 
	Tr2ALMemoryTypes flags, 
	ObjectType* object, 
	std::map<std::string, uint32_t>& description )
{
	if( object->IsValid() && ( object->GetMemoryClass() & flags ) != 0 )
	{
		REPORT_LAST_FRAME_USED;
		description["memory"] = object->GetMemoryClass();
		description["size"] = GetSizeEstimate( object );
		description["width"] = object->GetWidth();
		description["height"] = object->GetHeight();
		description["mipLevels"] = object->GetMipCount();
		description["trueMipLevels"] = object->GetTrueMipCount();
		description["format"] = uint32_t( object->GetFormat() );
		return true;
	}
	return false;
}

// ----------------------------------------------------------------------------------
// Description:
//   Returns estimated size of a texture.
// Arguments:
//   object - AL object
// Return value:
//   estimated size of a texture
// ----------------------------------------------------------------------------------
unsigned Tr2TrackedALObjectBase::ObjectInfo<Tr2RenderContextEnum::OT_TEXTURE>::GetSizeEstimate( ObjectType* object )
{
	unsigned size = 0;
	for( unsigned i = 0; i < object->GetTrueMipCount(); ++i )
	{
		size += object->GetMipSize( i );
	}
	return size;
}


// ----------------------------------------------------------------------------------
// Description:
//   Returns name of AL type this structure applies to.
// Return Value:
//   AL type name
// ----------------------------------------------------------------------------------
const char* Tr2TrackedALObjectBase::ObjectInfo<Tr2RenderContextEnum::OT_VERTEX_BUFFER>::GetName()
{
	return "Tr2VertexBufferAL";
}

// ----------------------------------------------------------------------------------
// Description:
//   Returns information about the AL object (as string -> int values).
// Arguments:
//   flags - Resource memory class filter
//   object - AL object
//   description - (out) information on AL object (as string -> int values)
// Return value:
//   true If object is alive and matches memory filter
//   false Otherwise
// ----------------------------------------------------------------------------------
bool Tr2TrackedALObjectBase::ObjectInfo<Tr2RenderContextEnum::OT_VERTEX_BUFFER>::GetDescription( 
	Tr2ALMemoryTypes flags, 
	ObjectType* object, 
	std::map<std::string, uint32_t>& description )
{
	if( object->IsValid() && ( object->GetMemoryClass() & flags ) != 0 )
	{
		REPORT_LAST_FRAME_USED;
		description["memory"] = object->GetMemoryClass();
		description["size"] = object->GetTotalSizeInBytes();
		return true;
	}
	return false;
}


// ----------------------------------------------------------------------------------
// Description:
//   Returns name of AL type this structure applies to.
// Return Value:
//   AL type name
// ----------------------------------------------------------------------------------
const char* Tr2TrackedALObjectBase::ObjectInfo<Tr2RenderContextEnum::OT_VERTEX_LAYOUT>::GetName()
{
	return "Tr2VertexLayoutAL";
}

// ----------------------------------------------------------------------------------
// Description:
//   Returns information about the AL object (as string -> int values).
// Arguments:
//   flags - Resource memory class filter
//   object - AL object
//   description - (out) information on AL object (as string -> int values)
// Return value:
//   true If object is alive and matches memory filter
//   false Otherwise
// ----------------------------------------------------------------------------------
bool Tr2TrackedALObjectBase::ObjectInfo<Tr2RenderContextEnum::OT_VERTEX_LAYOUT>::GetDescription( 
	Tr2ALMemoryTypes flags, 
	ObjectType* object, 
	std::map<std::string, uint32_t>& description )
{
	if( object->IsValid() && ( object->GetMemoryClass() & flags ) )
	{
		REPORT_LAST_FRAME_USED;
		return true;
	}
	return false;
}


// ----------------------------------------------------------------------------------
// Description:
//   Returns name of AL type this structure applies to.
// Return Value:
//   AL type name
// ----------------------------------------------------------------------------------
const char* Tr2TrackedALObjectBase::ObjectInfo<Tr2RenderContextEnum::OT_OCCLUSION_QUERY>::GetName()
{
	return "Tr2OcclusionQueryAL";
}

// ----------------------------------------------------------------------------------
// Description:
//   Returns information about the AL object (as string -> int values).
// Arguments:
//   flags - Resource memory class filter
//   object - AL object
//   description - (out) information on AL object (as string -> int values)
// Return value:
//   true If object is alive and matches memory filter
//   false Otherwise
// ----------------------------------------------------------------------------------
bool Tr2TrackedALObjectBase::ObjectInfo<Tr2RenderContextEnum::OT_OCCLUSION_QUERY>::GetDescription( 
	Tr2ALMemoryTypes flags, 
	ObjectType* object, 
	std::map<std::string, uint32_t>& description )
{
	if( object->IsValid() && ( object->GetMemoryClass() & flags ) != 0 )
	{
		REPORT_LAST_FRAME_USED;
		description["memory"] = object->GetMemoryClass();
		return true;
	}
	return false;
}


// ----------------------------------------------------------------------------------
// Description:
//   Returns name of AL type this structure applies to.
// Return Value:
//   AL type name
// ----------------------------------------------------------------------------------
const char* Tr2TrackedALObjectBase::ObjectInfo<Tr2RenderContextEnum::OT_SWAP_CHAIN>::GetName()
{
	return "Tr2SwapChainAL";
}

// ----------------------------------------------------------------------------------
// Description:
//   Returns information about the AL object (as string -> int values).
// Arguments:
//   flags - Resource memory class filter
//   object - AL object
//   description - (out) information on AL object (as string -> int values)
// Return value:
//   true If object is alive and matches memory filter
//   false Otherwise
// ----------------------------------------------------------------------------------
bool Tr2TrackedALObjectBase::ObjectInfo<Tr2RenderContextEnum::OT_SWAP_CHAIN>::GetDescription( 
	Tr2ALMemoryTypes flags, 
	ObjectType* object, 
	std::map<std::string, uint32_t>& description )
{
	if( object->IsValid() && ( object->GetMemoryClass() & flags ) != 0 )
	{
		REPORT_LAST_FRAME_USED;
		description["memory"] = object->GetMemoryClass();
		description["width"] = object->GetWidth();
		description["height"] = object->GetHeight();
		return true;
	}
	return false;
}


// ----------------------------------------------------------------------------------
// Description:
//   Returns name of AL type this structure applies to.
// Return Value:
//   AL type name
// ----------------------------------------------------------------------------------
const char* Tr2TrackedALObjectBase::ObjectInfo<Tr2RenderContextEnum::OT_GPU_BUFFER>::GetName()
{
	return "Tr2GpuBufferAL";
}

// ----------------------------------------------------------------------------------
// Description:
//   Returns information about the AL object (as string -> int values).
// Arguments:
//   flags - Resource memory class filter
//   object - AL object
//   description - (out) information on AL object (as string -> int values)
// Return value:
//   true If object is alive and matches memory filter
//   false Otherwise
// ----------------------------------------------------------------------------------
bool Tr2TrackedALObjectBase::ObjectInfo<Tr2RenderContextEnum::OT_GPU_BUFFER>::GetDescription( 
	Tr2ALMemoryTypes flags, 
	ObjectType* object, 
	std::map<std::string, uint32_t>& description )
{
	if( object->IsValid() && ( object->GetMemoryClass() & flags ) != 0 )
	{
		REPORT_LAST_FRAME_USED;
		description["memory"] = object->GetMemoryClass();
		description["format"] = object->GetFormat();
		description["size"] = object->GetTotalSizeInBytes();
		return true;
	}
	return false;
}


// ----------------------------------------------------------------------------------
// Description:
//   Returns name of AL type this structure applies to.
// Return Value:
//   AL type name
// ----------------------------------------------------------------------------------
const char* Tr2TrackedALObjectBase::ObjectInfo<Tr2RenderContextEnum::OT_FENCE>::GetName()
{
	return "Tr2FenceAL";
}

// ----------------------------------------------------------------------------------
// Description:
//   Returns information about the AL object (as string -> int values).
// Arguments:
//   flags - Resource memory class filter
//   object - AL object
//   description - (out) information on AL object (as string -> int values)
// Return value:
//   true If object is alive and matches memory filter
//   false Otherwise
// ----------------------------------------------------------------------------------
bool Tr2TrackedALObjectBase::ObjectInfo<Tr2RenderContextEnum::OT_FENCE>::GetDescription( 
	Tr2ALMemoryTypes flags, 
	ObjectType* object, 
	std::map<std::string, uint32_t>& description )
{
	if( object->IsValid() && ( object->GetMemoryClass() & flags ) != 0 )
	{
		REPORT_LAST_FRAME_USED;
		description["memory"] = object->GetMemoryClass();
		return true;
	}
	return false;
}


// ----------------------------------------------------------------------------------
// Description:
//   Returns name of AL type this structure applies to.
// Return Value:
//   AL type name
// ----------------------------------------------------------------------------------
const char* Tr2TrackedALObjectBase::ObjectInfo<Tr2RenderContextEnum::OT_TIMER>::GetName()
{
	return "Tr2GpuTimerAL";
}

// ----------------------------------------------------------------------------------
// Description:
//   Returns information about the AL object (as string -> int values).
// Arguments:
//   flags - Resource memory class filter
//   object - AL object
//   description - (out) information on AL object (as string -> int values)
// Return value:
//   true If object is alive and matches memory filter
//   false Otherwise
// ----------------------------------------------------------------------------------
bool Tr2TrackedALObjectBase::ObjectInfo<Tr2RenderContextEnum::OT_TIMER>::GetDescription( 
	Tr2ALMemoryTypes flags, 
	ObjectType* object, 
	std::map<std::string, uint32_t>& description )
{
	if( object->IsValid() && ( object->GetMemoryClass() & flags ) != 0 )
	{
		REPORT_LAST_FRAME_USED;
		description["memory"] = object->GetMemoryClass();
		return true;
	}
	return false;
}


#endif
