#pragma once
#ifndef Tr2InteriorCell_H
#define Tr2InteriorCell_H

#include "include/ITr2Interior.h"
#include "Tr2InteriorVisualization.h"

// forwards
BLUE_DECLARE( Tr2InteriorCell );
BLUE_DECLARE( TriLineSet );
BLUE_DECLARE( Tr2SHProbeRes );
BLUE_DECLARE( TriTextureRes );
BLUE_DECLARE( Tr2VariableStore );
BLUE_DECLARE( Tr2InteriorStatic );
BLUE_DECLARE_VECTOR( Tr2InteriorStatic );

class Tr2InteriorCell :
	public IInitialize,
	public INotify,
	public IListNotify
{
public:
	Tr2InteriorCell( IRoot* lockobj = 0 );
	~Tr2InteriorCell();

	EXPOSE_TO_BLUE();

    using IInitialize::Lock;
    using IInitialize::Unlock;

	/////////////////////////////////////////////////////////////////////////////////////
	// IInitialize
	bool Initialize();

	/////////////////////////////////////////////////////////////////////////////////////
	// INotify
	virtual bool OnModified( Be::Var* value );

	/////////////////////////////////////////////////////////////////////////////////////
	// IListNotify
	void OnListModified( long event, ssize_t key, ssize_t key2, IRoot* value, const IList* theList );

	// Is the cell dirty (needs intersection with lights, dynamics, etc. update)
	bool IsDirty( void );
	// Resets the cell's dirty flag
	void ResetDirtyFlag( void );

	void UpdateBoundingBox( void );
	bool IsBoundingBoxReady( void ) const;
	void RebuildBoundingBox( void );
	bool IsUnbounded( void ) const;
	const std::string& GetName( void ) const { return m_name; }


	void Update( Be::Time time );

	bool HasSHProbes() const;
	void GetSHProbe( const Vector3& position, Matrix& red, Matrix& green, Matrix& blue );

	// Reflection map
	TriTextureRes* GetReflectionMap();

	// changes
	void RebuildInternalData();

	void UpdateBoundingBox( const Vector3& minBounds, const Vector3& maxBounds );
	
	// handle statics
	void AddStatic( Tr2InteriorStatic* interiorStatic );
	void RemoveStatic( Tr2InteriorStatic* interiorStatic );
	void ClearStatics();
	// Handle dynamics
	bool AddDynamic( ITr2InteriorDynamic* dynamic );
	bool RemoveDynamic( ITr2InteriorDynamic* dynamic );
	// Handle lights
	bool AddLight( ITr2InteriorLight* light );
	void RemoveLight( ITr2InteriorLight* light );

	// Debug
	void RenderDebugInfo( TriLineSetPtr lines );
	void SetVisualizeMethod( VisualizeMethod method );

	void SetSHProbeResource();
	void SetReflectionMapPath();

	bool IsVolumeReady() const;

	// Sets debug probe spheres brightness factor
	void SetSHScale( float scale );

	void MarkShadowsDirtyForDynamic( ITr2InteriorDynamic* dynamic );
	void MarkShadowsDirtyForSkinnedObjects();

	//	Utility Functions to determine cell Membership
	bool ContainsPoint( const Vector3& testPoint, float epsilon=0.0f );
	bool IntersectsAABB( const Vector3& boxMin, const Vector3& boxMax );
	bool IntersectsOBB( const Vector3& boxCenter, const Vector3& boxExtents, const Quaternion& boxOrientation );
	bool IntersectsSphere( const Vector3& center, float radius );

		//	Deprecated
	bool GetBoundingBox( Vector3& minBounds, Vector3& maxBounds );

	// Returns local variable store for this cell
	Tr2VariableStore& GetVariableStore() { return *m_variableStore; }

private:
	// name
	std::string m_name;

	// Reflection map
	std::string m_reflectionMapPath;
	TriTextureResPtr m_reflectionMapRes;

	bool m_isDirty;

	// cell has a size
	Vector3 m_minBounds, m_maxBounds;
	bool m_boundingBoxReady;
	bool m_isUnbounded;

	// cell content
public:
	PTr2InteriorStaticVector m_statics;

	std::string m_irradianceTexturePath;
	std::string m_directionalIrradianceTexturePath;
	TriTextureResPtr m_irradianceTexture;
	TriTextureResPtr m_directionalIrradianceTexture;
private:
	PITr2InteriorLightVector m_lights;
	PITr2InteriorDynamicVector m_dynamics;
	PITr2InteriorDynamicVector m_skinnedObjects;

	Tr2SHProbeResPtr m_shProbeResource;
	std::string m_shProbeResPath;

	// debug
	bool m_drawBoundingBox;

	// Sphere probe brightness factor (initialized by Tr2InteriorScene)
	float m_shScale;

	// Local variable store for this cell
	Tr2VariableStorePtr m_variableStore;
};

TYPEDEF_BLUECLASS( Tr2InteriorCell );
BLUE_DECLARE_VECTOR( Tr2InteriorCell );


#endif // Tr2InteriorCell_H
