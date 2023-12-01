////////////////////////////////////////////////////////////
//
//    Created:   September 2016
//    Copyright: CCP 2016
//

#include "StdAfx.h"
#include "Tr2ImpostorManager.h"
#include "Tr2Renderer.h"
#include "Tr2RenderTarget.h"
#include "Tr2DepthStencil.h"
#include "Shader/Tr2Effect.h"
#include "Resources/TriTextureRes.h"


namespace
{

// Per-instance data used by impostor billboards
struct ImposterVertex
{
	Vector4 position; // w - bounding sphere radius
	Vector2_16 texCoord;
};

}


Tr2ImpostorManager::ImpostorAtlas::ImpostorAtlas()
	:m_free( "Tr2ImpostorManager::ImpostorAtlas::m_free" )
{
}

// --------------------------------------------------------------------------------------
// Description:
//   Resizes the atlas. Invalidates all texcoords and creates new free texcoords.
// Arguments:
//   width - Atlas width
//   height - Atlas height
//   itemWidth - Billboard width
//   itemHeight - Billboard height
// --------------------------------------------------------------------------------------
void Tr2ImpostorManager::ImpostorAtlas::Resize( uint32_t width, uint32_t height, uint32_t itemWidth, uint32_t itemHeight )
{
	m_free.clear();

	uint32_t xCount = width / itemWidth;
	uint32_t yCount = height / itemHeight;
	m_free.reserve( xCount * yCount );
	for( uint32_t j = 0; j < yCount; ++j )
	{
		for( uint32_t i = 0; i < xCount; ++i )
		{

			m_free.push_back( Vector2_16( float( i * itemWidth + 0.5f ) / width, float( j * itemHeight + 0.5f ) / height ) );
		}
	}
}

// --------------------------------------------------------------------------------------
// Description:
//   Reserves a new texcoord from the atlas.
// Arguments:
//   coord - (out) New texcoord
// Return Value:
//   true If texcoord was reserved
//   false If the atlas is full
// --------------------------------------------------------------------------------------
bool Tr2ImpostorManager::ImpostorAtlas::Reserve( Vector2_16& coord )
{
	if( m_free.empty() )
	{
		return false;
	}
	coord = m_free.back();
	m_free.pop_back();
	return true;
}

// --------------------------------------------------------------------------------------
// Description:
//   Returns the given texcoord to the free pool.
// Arguments:
//   coord - Returned texcoord
// --------------------------------------------------------------------------------------
void Tr2ImpostorManager::ImpostorAtlas::Drop( Vector2_16 coord )
{
	m_free.push_back( coord );
}


Tr2ImpostorManager::Tr2ImpostorManager()
	:m_width( 1024 ),
	m_height( 1024 ),
	m_itemWidth( 32 ),
	m_itemHeight( 32 ),
	m_maxUpdates( 16 ),
	m_effectKey( -1 ),
	m_atlasDirty( true )
{
	m_rt.CreateInstance();
	m_itemRt.CreateInstance();
	m_ds.CreateInstance();

	Tr2Variable altas( "ImposterAtlasMap", m_rt );
	Tr2Variable itemSizeVar( "ImposterItemSize", Vector4() );
	
	m_effect.CreateInstance();
	m_effect->SetEffectPathName( "res:/graphics/effect/managed/space/system/impostor.fx" );

	m_effectKey = m_effect->GetHashValue() + ( uint64_t( static_cast<Tr2Effect*>( m_effect ) ) << 32 );

	static Tr2VertexDefinition def;
	if( def.m_items.empty() )
	{
		def.Add( def.FLOAT32_1, def.TEXCOORD, 5 );
		def.Add( def.FLOAT32_4, def.POSITION, 0, 1, 1 );
		def.Add( def.FLOAT16_2, def.TEXCOORD, 0, 1, 1 );
	}

	Tr2QuadRenderer::Instance()->RegisterEffect( m_effectKey, TRIBATCHTYPE_OPAQUE, sizeof( ImposterVertex ), 1, def, m_effect );
}

// --------------------------------------------------------------------------------------
// Description:
//   Implements IInitialize interface. Re-creates the atlas and rendering resources.
// Return Value:
//   true always
// --------------------------------------------------------------------------------------
bool Tr2ImpostorManager::Initialize()
{
	Reset();
	PrepareResources();

	return true;
}

// --------------------------------------------------------------------------------------
// Description:
//   Implements INotify interface. Resets the manager after attribute changes.  
// Return Value:
//   true always
// --------------------------------------------------------------------------------------
bool Tr2ImpostorManager::OnModified( Be::Var* )
{
	Initialize();
	return true;
}

// --------------------------------------------------------------------------------------
// Description:
//   Implements Tr2DeviceResource method. Doesn't really do anything.  
// --------------------------------------------------------------------------------------
void Tr2ImpostorManager::ReleaseResources( TriStorage )
{
}

// --------------------------------------------------------------------------------------
// Description:
//   Implements Tr2DeviceResource method. Creates rendering resources.  
// Return Value:
//   true always
// --------------------------------------------------------------------------------------
bool Tr2ImpostorManager::OnPrepareResources()
{
	m_rt->Create( m_width, m_height, 1, Tr2RenderContextEnum::PIXEL_FORMAT_B8G8R8A8_UNORM );
	m_itemRt->Create( uint32_t( m_itemWidth * m_maxUpdates ), m_itemHeight, 1, Tr2RenderContextEnum::PIXEL_FORMAT_B8G8R8A8_UNORM );
	m_ds->Create( uint32_t( m_itemWidth * m_maxUpdates ), m_itemHeight, Tr2Renderer::GetShaderModel() >= TR2SM_3_0_DEPTH ? Tr2RenderContextEnum::DSFMT_READABLE : Tr2RenderContextEnum::DSFMT_D24S8, 1, 0 );
	m_atlasDirty = true;
	return true;
}

// --------------------------------------------------------------------------------------
// Description:
//   Script-exposed create method. Re-creates the atlas and rendering resources.
// Arguments:
//   width - Atlas width
//   height - Atlas height
//   itemWidth - Billboard width
//   itemHeight - Billboard height
// --------------------------------------------------------------------------------------
void Tr2ImpostorManager::Create( 
		Be::OptionalWithDefaultValue<uint32_t, 1024> width, 
		Be::OptionalWithDefaultValue<uint32_t, 1024> height, 
		Be::OptionalWithDefaultValue<uint32_t, 16> itemWidth, 
		Be::OptionalWithDefaultValue<uint32_t, 16> itemHeight )
{
	m_width = width;
	m_height = height;
	m_itemWidth = itemWidth;
	m_itemHeight = itemHeight;

	Initialize();
}

// --------------------------------------------------------------------------------------
// Description:
//   Sets billboard size.
// Arguments:
//   width - Billboard width
//   height - Billboard height
// --------------------------------------------------------------------------------------
void Tr2ImpostorManager::SetItemSize( uint32_t width, uint32_t height )
{
	if( m_itemWidth == width && m_itemHeight == height )
	{
		return;
	}
	m_itemWidth = width;
	m_itemHeight = height;

	Initialize();
}


// --------------------------------------------------------------------------------------
// Description:
//   Resets the manager: deletes all billboards.  
// --------------------------------------------------------------------------------------
void Tr2ImpostorManager::Reset()
{
	m_objects.clear();
	m_atlas.Resize( m_width, m_height, m_itemWidth, m_itemHeight );
}

// --------------------------------------------------------------------------------------
// Description:
//   Adds a new object to be an impostor.
// Arguments:
//   object - Object to be an impostor
//   hash - Current object hash
// Return Value:
//   true If the object was added (or was already added before)
//   false If the billboard atlas is full of the max number of re-renderings is exceeded
// --------------------------------------------------------------------------------------
bool Tr2ImpostorManager::Add( ITr2ImpostorSource* object, const ITr2ImpostorSource::ImpostorHash& hash )
{
	if( !m_rt || !m_rt->IsValid() )
	{
		return false;
	}

	Vector3 up = TransformNormal( Vector3( 0, 1, 0 ), Tr2Renderer::GetInverseViewTransform() );

	auto found = m_objects.find( object );
	if( found != m_objects.end() )
	{
		found->second.hash = hash;
		found->second.renderPriority = object->GetRenderPriority( found->second.oldHash, hash );
		found->second.used = true;
		return true;
	}

	if( m_renderQueue.size() >= m_maxUpdates )
	{
		return false;
	}

	Impostor impostor;
	if( !m_atlas.Reserve( impostor.texcoord ) )
	{
		return false;
	}

	impostor.hash = hash;
	impostor.used = true;
	impostor.renderPriority = std::numeric_limits<float>::max();
	impostor.render = true;

	m_objects[object] = impostor;

	m_renderQueue.push_back( object );
	return true;
}

// --------------------------------------------------------------------------------------
// Description:
//   Starts the manager update. Should be called before any Add calls.
// --------------------------------------------------------------------------------------
void Tr2ImpostorManager::BeginUpdate()
{
	m_renderQueue.clear();

	for( auto it = m_objects.begin(); it != m_objects.end(); ++it )
	{
		it->second.used = false;
		it->second.render = false;
	}
}

// --------------------------------------------------------------------------------------
// Description:
//   Finishes the manager update. Should be called after all Add calls.
// --------------------------------------------------------------------------------------
void Tr2ImpostorManager::EndUpdate()
{
	std::vector<ITr2ImpostorSource*> remove;
	for( auto it = m_objects.begin(); it != m_objects.end(); ++it )
	{
		if( !it->second.used )
		{
			m_atlas.Drop( it->second.texcoord );
			remove.push_back( it->first );
		}
		else
		{
			Matrix transform;
			Vector4 sphere;
			it->first->GetLocalToWorldTransform( transform );
			it->first->GetImpostorBoundingSphere( sphere );

			ImposterVertex vertex;
			vertex.position = sphere;
			vertex.texCoord = it->second.texcoord;
			Tr2QuadRenderer::Instance()->AddQuads( m_effectKey, &vertex, 1 );
		}
	}
	for( auto it = remove.begin(); it != remove.end(); ++it )
	{
		m_objects.erase( *it );
	}

	if( m_renderQueue.size() < m_maxUpdates )
	{
		std::vector<std::pair<ITr2ImpostorSource*, Impostor*>> all;
		all.reserve( m_objects.size() );
		for( auto it = m_objects.begin(); it != m_objects.end(); ++it )
		{
			if( !it->second.render && it->second.renderPriority > 0.f )
			{
				all.push_back( std::make_pair( it->first, &it->second ) );
			}
		}
		std::sort( all.begin(), all.end(), &CompareImpostors );
		size_t count = std::min( m_maxUpdates - m_renderQueue.size(), all.size() );
		for( size_t i = 0; i < count; ++i )
		{
			m_renderQueue.push_back( all[i].first );
		}
	}

	Tr2Variable altas( "ImposterAtlasMap", m_rt );
	Vector4 itemSize( float( m_itemWidth ) / float( m_width ), float( m_itemHeight ) / float( m_height ), 0.f, 0.f );
	Tr2Variable itemSizeVar( "ImposterItemSize", itemSize );
} 

// --------------------------------------------------------------------------------------
// Description:
//   Returns the number of re-renerings to be done this frame. Should be called after
//   EndUpdate.
// Return Value:
//   Number of re-renerings to be done this frame
// --------------------------------------------------------------------------------------
size_t Tr2ImpostorManager::GetRenderQueueLength() const
{
	return m_renderQueue.size();
}

// --------------------------------------------------------------------------------------
// Description:
//   Static comparison of impostor rendering priorities used for sorting. 
// Arguments:
//   item1 - First impostor
//   item2 - Second impostor
// Return Value:
//   true iff first impostor rendering priority is less than the second's
// --------------------------------------------------------------------------------------
bool Tr2ImpostorManager::CompareImpostors( const std::pair<ITr2ImpostorSource*, Impostor*>& item1, const std::pair<ITr2ImpostorSource*, Impostor*>& item2 )
{
	if( item1.second->renderPriority <= item2.second->renderPriority )
	{
		return false;
	}
	return true;
}

// --------------------------------------------------------------------------------------
// Description:
//   Starts re-rendering of impostor billboards. Should be called before any 
//   BeginImpostorUpdate.
// Arguments:
//   renderContext - Current render context
// --------------------------------------------------------------------------------------
void Tr2ImpostorManager::BeginUpdateAtlas( Tr2RenderContext& renderContext )
{
	if( !m_rt || !m_rt->IsValid() || m_renderQueue.empty() )
	{
		return;
	}
	
	if( m_atlasDirty )
	{
		renderContext.m_esm.PushDepthStencilBuffer( Tr2TextureAL() );
		renderContext.m_esm.PushRenderTarget( *m_rt );
		
		renderContext.Clear( Tr2RenderContextEnum::CLEARFLAGS_TARGET, 0, 0 );
		
		renderContext.m_esm.PopRenderTarget();
		renderContext.m_esm.PopDepthStencilBuffer();
		m_atlasDirty = false;
	}

	renderContext.m_esm.PushRenderTarget( *m_itemRt );
	renderContext.m_esm.PushDepthStencilBuffer( *m_ds );

	renderContext.Clear( Tr2RenderContextEnum::CLEARFLAGS_TARGET | Tr2RenderContextEnum::CLEARFLAGS_ZBUFFER, 0, 0.f );
}

// --------------------------------------------------------------------------------------
// Description:
//   Finishes re-rendering of impostor billboards. Should be called before any 
//   EndImpostorUpdate. Copies updated impostor images to the atlas.
// Arguments:
//   renderContext - Current render context
// --------------------------------------------------------------------------------------
void Tr2ImpostorManager::EndUpdateAtlas( Tr2RenderContext& renderContext )
{
	if( !m_rt || !m_rt->IsValid() || m_renderQueue.empty() )
	{
		return;
	}

	renderContext.m_esm.PopDepthStencilBuffer();
	renderContext.m_esm.PopRenderTarget();

	for( size_t i = 0; i < m_renderQueue.size(); ++i )
	{
		auto impostor = m_objects.find( m_renderQueue[i] );

		impostor->second.oldHash = impostor->second.hash;

		auto x = uint32_t( float( impostor->second.texcoord.x ) * m_width );
		auto y = uint32_t( float( impostor->second.texcoord.y ) * m_height );
		Tr2TextureSubresource dst( 0 );
		dst.SetRect( x, y, x + m_itemWidth, y + m_itemHeight );
		Tr2TextureSubresource src( 0 );
		src.SetRect( uint32_t( i ) * m_itemWidth, 0, uint32_t( i + 1 ) * m_itemWidth, m_itemHeight );

		m_rt->GetRenderTarget().CopySubresourceRegion( dst, *m_itemRt, src, renderContext );
	}
}

// --------------------------------------------------------------------------------------
// Description:
//   Sets up rendering of an impostor.
// Arguments:
//   index - Impostor index in the rendering queue
//   renderContext - Current render context
// Return Value:
//   Impostor object at the given index
// --------------------------------------------------------------------------------------
ITr2ImpostorSource* Tr2ImpostorManager::BeginImpostorUpdate( size_t index, Tr2RenderContext& renderContext )
{
	if( !m_rt || !m_rt->IsValid() || index > m_renderQueue.size() )
	{
		return nullptr;
	}
	renderContext.m_esm.SetViewport( m_itemWidth, m_itemHeight, uint32_t( index ) * m_itemWidth, 0, 0.f, 1.f );
	return m_renderQueue[index];
}

// --------------------------------------------------------------------------------------
// Description:
//   Finishes rendering of an impostor, currently unused.
// Arguments:
//   index - Impostor index in the rendering queue
//   renderContext - Current render context
// --------------------------------------------------------------------------------------
void Tr2ImpostorManager::EndImpostorUpdate( size_t, Tr2RenderContext& )
{
}

// --------------------------------------------------------------------------------------
// Description:
//   Returns the number of impostors registered with the manager.
// Return Value:
//   Number of impostors registered with the manager
// --------------------------------------------------------------------------------------
size_t Tr2ImpostorManager::GetImpostorCount() const
{
	return m_objects.size();
}

// --------------------------------------------------------------------------------------
// Description:
//   Returns depth buffer used for rendering of impostors.
// Return Value:
//   Depth buffer used for rendering of impostors
// --------------------------------------------------------------------------------------
Tr2DepthStencil* Tr2ImpostorManager::GetItemDepthStencil() const
{
	return m_ds;
}
