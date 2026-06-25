// Copyright © 2016 CCP ehf.

#pragma once

#include "TriDebugTextRenderer.h"
#include "Tr2PickBuffer.h"
#ifdef BLUE_USE_LOCAL_ITr2DebugRenderer2
// This is only needed for py2 as the file now belongs in blue.
// Unfortunatly the blue py2 branch cannot be updated at present due to security vulnerability work.
// The file version in the older blue versions had diverged from this one is incompatible.
#include "Include/ITr2DebugRenderer2.h"
#else
#include <ITr2DebugRenderer2.h>
#endif
#include "TriDebugTextRenderer.h"
#include "Eve/EvePicking.h"

BLUE_CLASS( Tr2DebugRenderer ) :
	public ITr2DebugRenderer2
{
public:
	Tr2DebugRenderer( IRoot* lockobj = nullptr );

	EXPOSE_TO_BLUE();

	template <typename T>
	bool HasOption( T * owner, const char* option ) const
	{
		return this->HasOption( owner->GetRawRoot(), option );
	}

	bool HasOption( IRoot * owner, const char* option ) const;
	bool IsSelected( IRoot * owner ) const;
	bool IsSelected( Tr2DebugObjectReference owner ) const;

	void DrawLine( Tr2DebugObjectReference owner, const Vector3& from, const Vector3& to, Tr2DebugColor color );
	void DrawTriangle(
		Tr2DebugObjectReference owner,
		const Vector3& vertex1,
		const Vector3& normal1,
		const Vector3& vertex2,
		const Vector3& normal2,
		const Vector3& vertex3,
		const Vector3& normal3,
		Tr2DebugColor color );
	void DrawTriangle(
		Tr2DebugObjectReference owner,
		const Vector3& vertex1,
		const Vector3& vertex2,
		const Vector3& vertex3,
		const Vector3& normal,
		Tr2DebugColor color );

	void DrawBox( Tr2DebugObjectReference owner, const Vector3& min, const Vector3& max, Effect effect, Tr2DebugColor color );
	void DrawBox( Tr2DebugObjectReference owner, const Matrix& transform, const Vector3& min, const Vector3& max, Effect effect, Tr2DebugColor color );

	void DrawSphere( Tr2DebugObjectReference owner, const Vector4& sphere, uint32_t segments, Effect effect, Tr2DebugColor color );
	void DrawSphere( Tr2DebugObjectReference owner, const Vector3& center, float radius, uint32_t segments, Effect effect, Tr2DebugColor color );
	void DrawSphere( Tr2DebugObjectReference owner, const Matrix& transform, uint32_t segments, Effect effect, Tr2DebugColor color );
	void DrawSphere( Tr2DebugObjectReference owner, const Matrix& transform, float radius, uint32_t segments, Effect effect, Tr2DebugColor color );
	void DrawSphere( Tr2DebugObjectReference owner, const Matrix& transform, const Vector3& center, float radius, uint32_t segments, Effect effect, Tr2DebugColor color );

	void DrawCylinder( Tr2DebugObjectReference owner, const Matrix& transform, float radius, float height, uint32_t segments, Effect effect, Tr2DebugColor color );
	void DrawCylinder( Tr2DebugObjectReference owner, const Vector3& cap0, const Vector3& cap1, float radius, uint32_t segments, Effect effect, Tr2DebugColor color );
	void DrawCylinder( Tr2DebugObjectReference owner, const Matrix& transform, const Vector3& cap0, const Vector3& cap1, float radius, uint32_t segments, Effect effect, Tr2DebugColor color );

	void DrawCone( Tr2DebugObjectReference owner, const Matrix& transform, float radius, float height, uint32_t segments, Effect effect, Tr2DebugColor color );
	void DrawCone( Tr2DebugObjectReference owner, const Vector3& base, const Vector3& focal, float radius, uint32_t segments, Effect effect, Tr2DebugColor color );

	void DrawCone( Tr2DebugObjectReference owner, const Matrix& transform, float height, float angle, uint32_t segments, uint32_t coneSegments, Effect effect, Tr2DebugColor color );

	void DrawCapsule( Tr2DebugObjectReference owner, const Matrix& transform, float radius, float height, uint32_t segments, Effect effect, Tr2DebugColor color );
	void DrawCapsule( Tr2DebugObjectReference owner, const Vector3& cap0, const Vector3& cap1, float radius, uint32_t segments, Effect effect, Tr2DebugColor color );

	void DrawArrow( Tr2DebugObjectReference owner, const Vector3& start, const Vector3& end, float radius, float pointerLength, uint32_t segments, Effect effect, Tr2DebugColor color );
	void DrawDoubleArrow( Tr2DebugObjectReference owner, const Vector3& start, const Vector3& end, float radius, float pointerLength, uint32_t segments, Effect effect, Tr2DebugColor color );

	void DrawSphereArrow( Tr2DebugObjectReference owner, const Vector3& center, const Vector3& direction, float radius, uint32_t segments, Effect effect, Tr2DebugColor color );

	void DrawAxis( Tr2DebugObjectReference owner, const Matrix& transform, Effect effect );

	void DrawExtrusionShape( Tr2DebugObjectReference owner, const Matrix& transform, const Vector2* vertices, const Vector2* normals, uint32_t vertexCount, uint32_t segments, Effect effect, Tr2DebugColor color );

	// Draw a speaker like cone with sound waves emanating from it
	void DrawAudioSpeaker( Tr2DebugObjectReference owner, const Matrix& transform, float size, uint32_t segments, Effect effect, Tr2DebugColor color );

	void DrawText( TriDebugFont font, const Vector3& pos, const Color& color, const char* msg, ... );

	// needs to be called every frame before any Draw method
	void BeginRender();
	// needs to be called every frame after all Draw methods
	void EndRender( Tr2RenderContext & renderContext );

	void Pick( EvePendingPickingReadback & readback, bool synchronize, Tr2RenderContext& renderContext );


	void SetSelectedObjects( const std::vector<std::pair<IRoot*, uint32_t>>& objects );
	void SetOptions( IRoot * owner, std::vector<Tr2DebugRendererOption> & options );
	std::vector<Tr2DebugRendererOption> GetOptions( IRoot * owner ) const;
	void SetDefaultOptions( const std::vector<Tr2DebugRendererOption>& options );
	std::vector<Tr2DebugRendererOption> GetDefaultOptions() const;

	bool GetColorForOption( Color & color, const Tr2DebugRendererOption& option ) const;
	void SetColorForOption( const Tr2DebugRendererOption& option, const Color& color );

#if BLUE_WITH_PYTHON
	static PyObject* PyGetColorForOption( PyObject * self, PyObject * args );
#endif

private:
	struct Vertex
	{
		Vector3 m_position;
		Vector3 m_normal;
		float m_object;
		float m_line;
		uint32_t m_color;
		uint32_t m_zFailColor;

		Vertex();
		Vertex( const Vector3& position, Tr2DebugColor color, bool selected, size_t objectIndex );
		Vertex( const Vector3& position, const Vector3& normal, Tr2DebugColor color, bool selected, size_t objectIndex );
	};

	Tr2EffectPtr m_effect;
	Tr2EffectPtr m_pickingEffect;

	std::vector<Vertex> m_lines;
	std::vector<Vertex> m_triangles;
	std::vector<std::pair<Tr2DebugObjectReference, size_t>> m_objectLineOffsets;
	std::vector<std::pair<Tr2DebugObjectReference, size_t>> m_objectTriangleOffsets;

	Tr2DebugRendererOptions m_defaultOptions;
	std::map<IRootPtr, Tr2DebugRendererOptions> m_options;
	std::set<Tr2DebugObjectReference> m_selectedObjects;
	std::map<Tr2DebugRendererOption, Color> m_optionColors;
};

TYPEDEF_BLUECLASS( Tr2DebugRenderer );
