#ifndef _TRILINESET_H_
#define _TRILINESET_H_

#include "IRenderCallback.h"
#include "TriDebugResourceHelper.h"

BLUE_CLASS( TriLineSet ) :
	public IRoot,
	public IRenderCallback
{
public:
	EXPOSE_TO_BLUE();

	TriLineSet( IRoot* lockobj = 0 );
	~TriLineSet(){}

	void AddDefaultColor( const Vector3& from, const Vector3& to );
	void Add( const Vector3& from, uint32_t fromColor, const Vector3& to, uint32_t toColor );
	void AddTransformed( const Matrix& transform, const Vector3& from, uint32_t fromColor, const Vector3& to, uint32_t toColor );

	void AddLines( std::vector<std::pair<Vector3, Vector3>> lines );

	void AddSphere( const Vector3& center, float radius, int segments, uint32_t color = 0xffffffff );
	void AddCylinder( const Vector3& start, const Vector3& end, float radius, int segments, uint32_t color = 0xffffffff );
	void AddCone( const Vector3& start, const Vector3& end, float radius, int segments, uint32_t color = 0xffffffff );
	void AddCapsule( const Vector3& start, const Vector3& end, float radius, int segments, uint32_t color = 0xffffffff );

	void AddBox( const Vector3& min, const Vector3& max, uint32_t color = 0xffffffff );
	void AddOrientedBox( const Matrix& transform, const Vector3& min, const Vector3& max, uint32_t = 0xffffffff );
	void AddOrientedBox( const Matrix& boxMatrix, uint32_t color = 0xffffffff );

	void Clear();

	void Render( Tr2RenderContext& renderContext );
	void RenderFromScript();

	void SubmitGeometry( Tr2RenderContext& renderContext );
	void SetDefaultColor( uint32_t val ) { m_defaultColor = val; }
	void SetCurrentColor( uint32_t val );

	Matrix GetWorldTransform( void ) const { return m_transform; }

private:
	Matrix m_transform;
	uint32_t m_defaultColor;
	bool	m_zEnable;
	std::vector<TriDebugResourceHelper::VertexPosColor> m_vertices;
#if BLUE_WITH_PYTHON
	PyObject* PyRender( PyObject* args );
#endif
};
TYPEDEF_BLUECLASS(TriLineSet);
BLUE_DECLARE_VECTOR(TriLineSet);
#endif

