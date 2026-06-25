// Copyright © 2023 CCP ehf.

#include "StdAfx.h"
#include "Tr2SpriteObject.h"

#include "Tr2Sprite2dScene.h"

Tr2SpriteObjectBase::Tr2SpriteObjectBase() :
	m_isDirty( true ),
	m_parent( nullptr ),
	m_auxMouseover( nullptr ),
	m_translation( 0.0f, 0.0f ),
	m_displayWidth( 0.0f ),
	m_displayHeight( 0.0f ),
	m_display( true ),
	m_pickState( TR2_SPS_ON )
#if BLUE_WITH_PYTHON
	,
	m_associatedObject( nullptr )
#endif
{
}

Tr2SpriteObjectBase::~Tr2SpriteObjectBase()
{
	CCP_ASSERT( !m_parent );
}

void Tr2SpriteObjectBase::SetParent( ITr2SpriteObject* parent )
{
	m_parent = parent;
	SetDirty();
}

void Tr2SpriteObjectBase::SetChildDirty( ITr2SpriteObject* child )
{
	// Base does nothing - override in container classes
}

bool Tr2SpriteObjectBase::IsAuxMouseover()
{
	return false;
}

void Tr2SpriteObjectBase::SetDirty()
{
	m_isDirty = true;
	if( m_parent )
	{
		m_parent->SetChildDirty( this );
	}
}

bool Tr2SpriteObjectBase::GetDisplay() const
{
	return m_display;
}

void Tr2SpriteObjectBase::SetDisplay( bool val )
{
	if( val != m_display )
	{
		m_display = val;
		SetDirty();
	}
}

float Tr2SpriteObjectBase::GetDisplayX() const
{
	return m_translation.x;
}

void Tr2SpriteObjectBase::SetDisplayX( float val )
{
	if( val != m_translation.x )
	{
		m_translation.x = val;
		SetDirty();
	}
}

float Tr2SpriteObjectBase::GetDisplayY() const
{
	return m_translation.y;
}

void Tr2SpriteObjectBase::SetDisplayY( float val )
{
	if( val != m_translation.y )
	{
		m_translation.y = val;
		SetDirty();
	}
}

float Tr2SpriteObjectBase::GetDisplayWidth() const
{
	return m_displayWidth;
}

void Tr2SpriteObjectBase::SetDisplayWidth( float val )
{
	if( val != m_displayWidth )
	{
		m_displayWidth = val;
		SetDirty();
	}
}

float Tr2SpriteObjectBase::GetDisplayHeight() const
{
	return m_displayHeight;
}

void Tr2SpriteObjectBase::SetDisplayHeight( float val )
{
	if( val != m_displayHeight )
	{
		m_displayHeight = val;
		SetDirty();
	}
}

bool Tr2SpriteObjectBase::OnModified( Be::Var* value )
{
	SetDirty();
	return true;
}

#if BLUE_WITH_PYTHON
PyObject* Tr2SpriteObjectBase::GetAssociatedObject() const
{
	if( m_associatedObject )
	{
		return PyWeakref_GetObject( m_associatedObject );
	}

	Py_RETURN_NONE;
}

void Tr2SpriteObjectBase::SetAssociatedObject( PyObject* obj )
{
	if( m_associatedObject )
	{
		Py_DECREF( m_associatedObject );
	}

	if( obj != Py_None )
	{
		m_associatedObject = PyWeakref_NewRef( obj, nullptr );
	}
	else
	{
		m_associatedObject = nullptr;
	}
}
#endif

Tr2SpriteObject::Tr2SpriteObject( IRoot* lockobj ) :
	m_blendMode( TR2_SBM_NONE ),
	m_spriteEffect( TR2_SFX_COPY ),
	m_target( Tr2SpriteTarget::COLOR ),
	m_glowBrightness( 1 ),
	m_color( 1.0f, 1.0f, 1.0f, 1.0f ),
	m_depth( 0.0f ),
	m_shadowOffset( 0.0f, 0.0f ),
	m_shadowColor( 0.0f, 0.0f, 0.0f, 1.0f ),
	m_glowFactor( 0.0f ),
	m_glowExpand( 0.0f ),
	m_glowColor( 1.0f, 1.0f, 1.0f, 1.0f ),
	m_outlineColor( 0.0f, 0.0f, 0.0f, 1.0f ),
	m_outlineThreshold( 0.0f )
{
}

void Tr2SpriteObject::SetRegularRenderState( Tr2Sprite2dScene* renderer )
{
	renderer->SetBlendmode( m_blendMode );
	renderer->SetSpriteTarget( m_target );
	renderer->SetGlowBrightness( m_glowBrightness );
	renderer->SetColor( m_color );
	renderer->SetDepth( m_depth );
}

void Tr2SpriteObject::SetGlowRenderState( Tr2Sprite2dScene* renderer )
{
	Color c = m_glowColor;
	c.a *= m_color.a;
	renderer->SetBlendmode( TR2_SBM_ADDX2 );
	renderer->SetSpriteTarget( m_target );
	renderer->SetGlowBrightness( m_glowBrightness );
	renderer->SetColor( c );
	renderer->SetDepth( m_depth );
}

void Tr2SpriteObject::SetShadowRenderState( Tr2Sprite2dScene* renderer )
{
	Color c = m_shadowColor;
	c.a *= m_color.a;
	renderer->SetBlendmode( m_blendMode );
	renderer->SetSpriteTarget( m_target );
	renderer->SetGlowBrightness( m_glowBrightness );
	renderer->SetColor( c );
	renderer->SetDepth( m_depth );
}

void Tr2SpriteObject::SetOutlineRenderState( Tr2Sprite2dScene* renderer )
{
	Color color = m_outlineColor;
	color.a *= m_color.a;
	renderer->SetOutlineColor( color );
	renderer->SetOutlineThreshold( m_outlineThreshold );
}

Color Tr2SpriteObject::GetColor() const
{
	return m_color;
}

void Tr2SpriteObject::SetColor( Color val )
{
	if( val != m_color )
	{
		m_color = val;
		SetDirty();
	}
}

Tr2TexturedSpriteObject::Tr2TexturedSpriteObject( IRoot* lockobj /*= NULL */ )
{
}

Tr2TexturedSpriteObject::~Tr2TexturedSpriteObject()
{
	if( m_texturePrimary )
	{
		m_texturePrimary->UnregisterForChangeNotification( this );
	}
	if( m_textureSecondary )
	{
		m_textureSecondary->UnregisterForChangeNotification( this );
	}
}

void Tr2TexturedSpriteObject::Sprite2dTextureChanged( ITr2Sprite2dTexture* p )
{
	SetDirty();
}

void Tr2TexturedSpriteObject::SetTexturePrimary( ITr2Sprite2dTexture* t )
{
	if( t != m_texturePrimary )
	{
		if( m_texturePrimary )
		{
			m_texturePrimary->UnregisterForChangeNotification( this );
		}
		m_texturePrimary = t;
		if( m_texturePrimary )
		{
			m_texturePrimary->RegisterForChangeNotification( this );
		}
		SetDirty();
	}
}

ITr2Sprite2dTexture* Tr2TexturedSpriteObject::GetTexturePrimary() const
{
	return m_texturePrimary;
}

void Tr2TexturedSpriteObject::SetTextureSecondary( ITr2Sprite2dTexture* t )
{
	if( t != m_textureSecondary )
	{
		if( m_textureSecondary )
		{
			m_textureSecondary->UnregisterForChangeNotification( this );
		}
		m_textureSecondary = t;
		if( m_textureSecondary )
		{
			m_textureSecondary->RegisterForChangeNotification( this );
		}
		SetDirty();
	}
}

ITr2Sprite2dTexture* Tr2TexturedSpriteObject::GetTextureSecondary() const
{
	return m_textureSecondary;
}

bool Tr2TexturedSpriteObject::ValidateAndSetTextures( Tr2Sprite2dScene* renderer )
{
	if( m_spriteEffect >= TR2_SFX_ONE_TEXTURE )
	{
		if( m_texturePrimary )
		{
			if( !m_texturePrimary->IsGood() )
			{
				// Don't draw if we're still loading (or failed)
				return false;
			}
			m_texturePrimary->Apply( renderer, 0 );
		}
		else
		{
			// Don't draw if no texture set
			return false;
		}

		if( m_spriteEffect >= TR2_SFX_TWO_TEXTURES )
		{
			if( m_textureSecondary )
			{
				if( !m_textureSecondary->IsGood() )
				{
					// Don't draw if we're still loading (or failed)
					return false;
				}
				m_textureSecondary->Apply( renderer, 1 );
			}
			else
			{
				// Don't draw if no texture set
				return false;
			}
		}
	}

	return true;
}

void Tr2TexturedSpriteObject::SetValidatedTextures( Tr2Sprite2dScene* renderer )
{
	// Set textures - note that we don't need the full check here - we would have
	// bailed earlier while preparing the vertices. If anything changed with the textures
	// we would have been flagged dirty so the check is always done there.
	if( m_spriteEffect >= TR2_SFX_ONE_TEXTURE )
	{
		m_texturePrimary->Apply( renderer, 0 );

		if( m_spriteEffect >= TR2_SFX_TWO_TEXTURES )
		{
			m_textureSecondary->Apply( renderer, 1 );
		}
	}
}
