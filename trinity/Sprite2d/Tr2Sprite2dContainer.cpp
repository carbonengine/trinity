#include "StdAfx.h"
#include "Tr2Sprite2dContainer.h"
#include "Tr2Sprite2dScene.h"
#include "Tr2Sprite2dPickingMask.h"


Tr2Sprite2dContainerBase::Tr2Sprite2dContainerBase( IRoot* lockobj ) :
	PARENTLOCK( m_children ),
	PARENTLOCK( m_background ),
	m_opacity( 1.0f )
{
	m_children.SetNotify( this );
	m_background.SetNotify( this );
}

Tr2Sprite2dContainerBase::~Tr2Sprite2dContainerBase()
{
	for( auto it = m_background.begin(); it != m_background.end(); ++it )
	{
		if( *it )
		{
			(*it)->SetParent( nullptr );
		}
	}
	for( auto it = m_children.begin(); it != m_children.end(); ++it )
	{
		if( *it )
		{
			(*it)->SetParent( nullptr );
		}
	}
}
void Tr2Sprite2dContainerBase::SetChildDirty( ITr2SpriteObject* child )
{
	m_isDirty = true;
	if( m_parent )
	{
		m_parent->SetChildDirty( this );
	}
}

void Tr2Sprite2dContainerBase::OnListModified( long event, /* BLUELISTEVENT values */ ssize_t key, ssize_t key2, IRoot* value, const IList* theList )
{
	switch( event )
	{
	case BELIST_INSERTED:
		{
			ITr2SpriteObjectPtr spriteObj( BlueCastPtr( value ) );
			if( spriteObj )
			{
				spriteObj->SetParent( this );
				SetChildDirty( spriteObj );
			}
		}
		break;

	case BELIST_REMOVED:
		{
			ITr2SpriteObjectPtr spriteObj( BlueCastPtr( value ) );
			if( spriteObj )
			{
				spriteObj->SetParent( nullptr );
				SetChildDirty( spriteObj );
			}
		}
		break;

	case BELIST_UNLOADSTART:
		{
			ssize_t size = theList->GetSize();
			for( ssize_t i = 0; i < size; ++i )
			{
				ITr2SpriteObjectPtr child = BlueCastPtr( theList->GetAt( i ) );
				if( child )
				{
					child->SetParent( nullptr );
				}
			}
		}
		break;
	}
}

Tr2Sprite2dContainer::Tr2Sprite2dContainer( IRoot* lockobj ) :
	Tr2Sprite2dContainerBase( lockobj ),
	m_depthMin( -1.0f ),
	m_depthMax( 1.0f ),
	m_absoluteCoordinates( false ),
	m_clip( false ),
	m_pickRadius( 0.0f ),
	m_cacheContents( true ),
	m_cacheContentsHint( false ),
	m_dirtyFrameCount( 0 ),
	m_cleanFrameCount( 0 ),
	m_cachedDisplayList( nullptr )
{
	m_children.SetNotify( this );
	m_background.SetNotify( this );
}

Tr2Sprite2dContainer::~Tr2Sprite2dContainer()
{
	if( m_cachedDisplayList )
	{
		CCP_DELETE m_cachedDisplayList;
	}
}

void Tr2Sprite2dContainer::GatherSprites( Tr2Sprite2dScene* renderer )
{
	if( !m_display || (m_children.empty() && m_background.empty()) )
	{
		return;
	}

	if( m_displayWidth <= 0.0f || m_displayHeight <= 0.0f )
	{
		return;
	}

	if( m_cacheContentsHint )
	{
		if( m_isDirty )
		{
			m_cleanFrameCount = 0;
			if( m_dirtyFrameCount > 10 )
			{
				m_cacheContents = false;
				if( m_cachedDisplayList )
				{
					CCP_DELETE m_cachedDisplayList;
					m_cachedDisplayList = nullptr;
				}
			}
			else
			{
				m_dirtyFrameCount++;
			}
		}
		else
		{
			m_dirtyFrameCount = 0;
			if( m_cleanFrameCount > 3 )
			{
				m_cacheContents = true;
			}
			else
			{
				m_cleanFrameCount++;
			}
		}
	}
	else
	{
		m_dirtyFrameCount = 0;
		m_cleanFrameCount = 0;
		m_cacheContents = false;
		if( m_cachedDisplayList )
		{
			CCP_DELETE m_cachedDisplayList;
			m_cachedDisplayList = nullptr;
		}
	}

	if( m_cacheContents && !renderer->IsCapturing() )
	{
		if( m_isDirty || !m_cachedDisplayList )
		{
			auto oldCachedDisplayList = m_cachedDisplayList;
			m_cachedDisplayList = nullptr;

			// StartCapture can fail, if for some reason a vertex buffer
			// can't be created.
			bool isCapturing = renderer->StartCapture( this );

			// We perform the work of gathering sprites regardless of
			// success or failure - if StartCapture failed, we still attempt
			// to render directly.
			GatherSpritesHelper( renderer );

			if( isCapturing )
			{
				m_cachedDisplayList = renderer->EndCapture( oldCachedDisplayList );

				// Clear the dirty flag only if we successfully captured.
				// Otherwise we wouldn't try again next frame.
				m_isDirty = false;
			}
			if( oldCachedDisplayList )
			{
				CCP_DELETE oldCachedDisplayList;
			}
		}

		if( m_cachedDisplayList )
		{
			renderer->ReplayCapture( m_cachedDisplayList );
		}
	}
	else
	{
		GatherSpritesHelper(renderer);
		m_isDirty = false;
	}
}

ITr2SpriteObject* Tr2Sprite2dContainer::PickPoint( float x, float y, Tr2Sprite2dScene* renderer )
{
	if( !m_display )
	{
		return NULL;
	}
	if( m_pickState == TR2_SPS_OFF )
	{
		return NULL;
	}

	if( m_displayWidth <= 0.0f || m_displayHeight <= 0.0f )
	{
		return NULL;
	}

	ITr2SpriteObject* obj = NULL;

	if( m_absoluteCoordinates )
	{
		renderer->PushTransformAbsolute();
	}

	renderer->PushTranslation( m_translation );

	if( renderer->IsInside( Vector2( x, y ), Vector2( 0.0f, 0.0f ), m_displayWidth, m_displayHeight, m_pickRadius ) )
	{
		if( !m_pickingMask || m_pickingMask->SampleMask( renderer->InverseTransformPoint( Vector2( x, y ) ), Vector2( 0.0f, 0.0f ), m_displayWidth, m_displayHeight ) )
		{
			if( m_clip )
			{
				renderer->PushClipRectangle( 0.0f, 0.0f, m_displayWidth, m_displayHeight );
			}

			for( ITr2SpriteObjectVector::iterator it = m_children.begin(); it != m_children.end(); ++it )
			{
				obj = ( *it )->PickPoint( x, y, renderer );
				if( obj )
				{
					break;
				}
			}

			if( m_clip )
			{
				renderer->PopClipRectangle();
			}

			// If container itself is pickable and no child was found
			if( m_pickState == TR2_SPS_ON )
			{
				this->m_auxMouseover = NULL;
				if( !obj )
				{
					obj = this;
				}
				else
				{
					if( obj->IsAuxMouseover() )
					{
						// The auxiliary mouseover can add content to the mouseover text or context menu
						this->m_auxMouseover = obj;
						obj = this;
					}
				}
			}
		}
	}


	renderer->PopTranslation();

	if( m_absoluteCoordinates )
	{
		renderer->PopTransformAbsolute();
	}

	return obj;
}


void Tr2Sprite2dContainer::SetDirty()
{
	Tr2SpriteObjectBase::SetDirty();
}

void Tr2Sprite2dContainer::GatherSpritesHelper( Tr2Sprite2dScene* renderer )
{
	if( m_absoluteCoordinates )
	{
		renderer->PushTransformAbsolute();
	}

	renderer->PushTranslation( m_translation );
	renderer->PushDepthRange( m_depthMin, m_depthMax );
	float oldOpacity = renderer->GetAccumulatedAlpha();
	renderer->SetAccumulatedAlpha( m_opacity * oldOpacity );

	if( m_clip )
	{
		renderer->PushClipRectangle( 0.0f, 0.0f, m_displayWidth, m_displayHeight );
	}

	for( ITr2SpriteObjectVector::reverse_iterator it = m_background.rbegin(); it != m_background.rend(); ++it )
	{
		(*it)->GatherSprites( renderer );
	}
	for( ITr2SpriteObjectVector::reverse_iterator it = m_children.rbegin(); it != m_children.rend(); ++it )
	{
		(*it)->GatherSprites( renderer );
	}

	if( m_clip )
	{
		renderer->PopClipRectangle();
	}

	renderer->SetAccumulatedAlpha( oldOpacity );
	renderer->PopDepthRange();
	renderer->PopTranslation();

	if( m_absoluteCoordinates )
	{
		renderer->PopTransformAbsolute();
	}
}

