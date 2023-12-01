// There is already an outdated interface for ITr2DebugRenderer that needs to be 
// there atm for legacy reasons. Thus why this is nr.2 (okt 2019)
#pragma once

#ifndef ITr2DebugRenderer2_H
#define ITr2DebugRenderer2_H

BLUE_DECLARE( Tr2Effect );
BLUE_DECLARE( Tr2DebugRenderer );

typedef std::string Tr2DebugRendererOption;
typedef std::set<Tr2DebugRendererOption> Tr2DebugRendererOptions;

class Tr2RenderContext;


enum TriDebugFont
{
	TRI_DBG_FONT_SMALL,
	TRI_DBG_FONT_MEDIUM,
	TRI_DBG_FONT_LARGE,
};

enum TriDebugFontStyle
{
	TRI_DFS_LEFT = 0x00000000,
	TRI_DFS_CENTER = 0x00000001,
	TRI_DFS_RIGHT = 0x00000002,
	TRI_DFS_TOP = 0x00000000,
	TRI_DFS_BOTTOM = 0x00000008,
	TRI_DFS_VCENTER = 0x00000004,
};


struct Tr2DebugColor
{
	// front color
	uint32_t m_color;
	// color for things behind other geometry
	uint32_t m_zFailColor;

	// front color when object is selected
	uint32_t m_colorSelected;
	// color for things behind other geometry when object is selected
	uint32_t m_zFailColorSelected;

	uint32_t GetAutoSelectedColor( uint32_t color )
	{
		auto c = Color( color );
		c.r = std::min( 1.f, c.r + 0.7f );
		c.g = std::min( 1.f, c.g + 0.7f );
		c.b = std::min( 1.f, c.b + 0.7f );
		return c;
	}

	Tr2DebugColor( uint32_t color )
		:m_color( color ),
		m_zFailColor( 0u ),
		m_colorSelected( GetAutoSelectedColor( color ) ),
		m_zFailColorSelected( 0u )
	{
	}

	Tr2DebugColor( const Color& color )
		:m_color( color ),
		m_zFailColor( 0u ),
		m_colorSelected( GetAutoSelectedColor( color ) ),
		m_zFailColorSelected( 0u )
	{
	}

	Tr2DebugColor( uint32_t color, uint32_t zFailColor )
		:m_color( color ),
		m_zFailColor( zFailColor ),
		m_colorSelected( GetAutoSelectedColor( color ) ),
		m_zFailColorSelected( GetAutoSelectedColor( zFailColor ) )
	{
	}

	Tr2DebugColor( const Color& color, const Color& zFailColor )
		:m_color( color ),
		m_zFailColor( zFailColor ),
		m_colorSelected( GetAutoSelectedColor( color ) ),
		m_zFailColorSelected( GetAutoSelectedColor( zFailColor ) )
	{
	}

	Tr2DebugColor( const Color& color, const Color& zFailColor, const Color& selectedColor, const Color& zFailSelectedColor )
		:m_color( color ),
		m_zFailColor( zFailColor ),
		m_colorSelected( selectedColor ),
		m_zFailColorSelected( zFailSelectedColor )
	{
	}
};

struct Tr2DebugObjectReference
{
	IRootPtr m_object;
	uint32_t m_area;

	template <typename T>
	Tr2DebugObjectReference( T* object )
		:m_object( object->GetRawRoot() ),
		m_area( 0 )
	{
	}

	template <typename T>
	Tr2DebugObjectReference( T* object, uint32_t area )
		: m_object( object->GetRawRoot() ),
		m_area( area )
	{
	}

	Tr2DebugObjectReference( IRoot* object );
	Tr2DebugObjectReference( IRoot* object, uint32_t area );

	operator bool() const;
	bool operator<( const Tr2DebugObjectReference& other ) const;
	bool operator==( const Tr2DebugObjectReference& other ) const;
	bool operator!=( const Tr2DebugObjectReference& other ) const;
};

BLUE_INTERFACE( ITr2DebugRenderer2 ) : public IRoot
{
	enum Effect
	{
		Wireframe,
		Solid,
		Lit,
	};

	template <typename T>
	bool HasOption( T* owner, const char* option ) const
	{
		return this->HasOption( owner->GetRawRoot(), option );
	}

	virtual bool HasOption( IRoot* owner, const char* option ) const = 0;
	virtual bool IsSelected( IRoot* owner ) const = 0;
	virtual bool IsSelected( Tr2DebugObjectReference owner ) const = 0;

	virtual void DrawLine( Tr2DebugObjectReference owner, const Vector3& from, const Vector3& to, Tr2DebugColor color ) = 0;
	virtual void DrawTriangle( Tr2DebugObjectReference owner, const Vector3& vertex1, const Vector3& normal1, const Vector3& vertex2, const Vector3& normal2, const Vector3& vertex3, const Vector3& normal3, Tr2DebugColor color ) = 0;
	virtual void DrawTriangle( Tr2DebugObjectReference owner, const Vector3& vertex1, const Vector3& vertex2, const Vector3& vertex3, const Vector3& normal, Tr2DebugColor color ) = 0;

	virtual void DrawBox( Tr2DebugObjectReference owner, const Vector3& min, const Vector3& max, Effect effect, Tr2DebugColor color ) = 0;
	virtual void DrawBox( Tr2DebugObjectReference owner, const Matrix& transform, const Vector3& min, const Vector3& max, Effect effect, Tr2DebugColor color ) = 0;

	virtual void DrawSphere( Tr2DebugObjectReference owner, const Vector4& sphere, uint32_t segments, Effect effect, Tr2DebugColor color ) = 0;
	virtual void DrawSphere( Tr2DebugObjectReference owner, const Vector3& center, float radius, uint32_t segments, Effect effect, Tr2DebugColor color ) = 0;
	virtual void DrawSphere( Tr2DebugObjectReference owner, const Matrix& transform, uint32_t segments, Effect effect, Tr2DebugColor color ) = 0;
	virtual void DrawSphere( Tr2DebugObjectReference owner, const Matrix& transform, float radius, uint32_t segments, Effect effect, Tr2DebugColor color ) = 0;
	virtual void DrawSphere( Tr2DebugObjectReference owner, const Matrix& transform, const Vector3& center, float radius, uint32_t segments, Effect effect, Tr2DebugColor color ) = 0;

	virtual void DrawCylinder( Tr2DebugObjectReference owner, const Matrix& transform, float radius, float height, uint32_t segments, Effect effect, Tr2DebugColor color ) = 0;
	virtual void DrawCylinder( Tr2DebugObjectReference owner, const Vector3& cap0, const Vector3& cap1, float radius, uint32_t segments, Effect effect, Tr2DebugColor color ) = 0;
	virtual void DrawCylinder( Tr2DebugObjectReference owner, const Matrix& transform, const Vector3& cap0, const Vector3& cap1, float radius, uint32_t segments, Effect effect, Tr2DebugColor color ) = 0;

	virtual void DrawCone( Tr2DebugObjectReference owner, const Matrix& transform, float radius, float height, uint32_t segments, Effect effect, Tr2DebugColor color ) = 0;
	virtual void DrawCone( Tr2DebugObjectReference owner, const Vector3& base, const Vector3& focal, float radius, uint32_t segments, Effect effect, Tr2DebugColor color ) = 0;
	
	virtual void DrawCone( Tr2DebugObjectReference owner, const Matrix& transform, float height, float angle, uint32_t segments, uint32_t coneSegments, Effect effect, Tr2DebugColor color ) = 0;
	
	virtual void DrawCapsule( Tr2DebugObjectReference owner, const Matrix& transform, float radius, float height, uint32_t segments, Effect effect, Tr2DebugColor color ) = 0;
	virtual void DrawCapsule( Tr2DebugObjectReference owner, const Vector3& cap0, const Vector3& cap1, float radius, uint32_t segments, Effect effect, Tr2DebugColor color ) = 0;

	virtual void DrawArrow( Tr2DebugObjectReference owner, const Vector3& start, const Vector3& end, float radius, float pointerLength, uint32_t segments, Effect effect, Tr2DebugColor color ) = 0;
	virtual void DrawDoubleArrow( Tr2DebugObjectReference owner, const Vector3& start, const Vector3& end, float radius, float pointerLength, uint32_t segments, Effect effect, Tr2DebugColor color ) = 0;

	virtual void DrawSphereArrow( Tr2DebugObjectReference owner, const Vector3& center, const Vector3& direction, float radius, uint32_t segments, Effect effect, Tr2DebugColor color ) = 0;

	virtual void DrawAxis( Tr2DebugObjectReference owner, const Matrix& transform, Effect effect ) = 0;

	virtual void DrawExtrusionShape( Tr2DebugObjectReference owner, const Matrix& transform, const Vector2* vertices, const Vector2* normals, uint32_t vertexCount, uint32_t segments, Effect effect, Tr2DebugColor color ) = 0;

	virtual void DrawText( TriDebugFont font, const Vector3& pos, const Color& color, const char* msg, ... ) = 0;

	virtual bool GetColorForOption( Color & color, const Tr2DebugRendererOption& option ) const = 0;
	virtual void SetColorForOption( const Tr2DebugRendererOption& option, const Color& color ) = 0;

};

BLUE_INTERFACE( ITr2DebugRenderable ) : public IRoot
{
	virtual void GetDebugOptions( Tr2DebugRendererOptions & options ) = 0;

	virtual void RenderDebugInfo( ITr2DebugRenderer2& renderer ) = 0;
};

#endif
