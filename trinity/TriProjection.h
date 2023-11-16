#pragma once
#ifndef _TRIPROJECTION_H_
#define _TRIPROJECTION_H_


const char TRIPROJECTION_FOV = 1;
const char TRIPROJECTION_OFF_CENTER = 2;
const char TRIPROJECTION_ORTHO = 3;
const char TRIPROJECTION_CUSTOM = 4;

BLUE_CLASS( TriProjection ) : public IRoot
{
public:
	EXPOSE_TO_BLUE();
	
	TriProjection( IRoot* lockobj = NULL );
	~TriProjection(void);

	void PerspectiveFov( float fov, float aspect, float zn, float zf);
	void PerspectiveOffCenter( float left, float right, float bottom, float top, float zn, float zf);
	void PerspectiveOrthographic( float witdth, float height, float front, float back);
	void CustomProjection( const Matrix& mat );

	int GetProjectionType();
	void SetProjection();
	void SetProjection( Tr2RenderContext& renderContext );
	void GetMatrixWithoutViewAdjustment( Matrix& proj ) const;

	Matrix GetTransform() const;

private:
	char m_projectionType;
	float m_fov;
	float m_aspect;
	float m_left;
	float m_right;
	float m_bottom;
	float m_top;
	float m_zn;
	float m_zf;
	Matrix m_customTransform;
};

TYPEDEF_BLUECLASS( TriProjection );

#endif