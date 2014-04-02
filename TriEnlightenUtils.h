#pragma once
#ifndef TriEnlightenUtils_h
#define TriEnlightenUtils_h



class TriEnlightenProgressBar:
	public Geo::IGeoProgressProxy
{
public:
	virtual ~TriEnlightenProgressBar() {}
	virtual void SetProgressString( const Geo::c8* desc );
	virtual void SetProportionDone(float amount);
	virtual void Finished( bool success ) {}

	virtual bool ShouldCancel() { return false; };
};


//-------------------------------------------------------------------------------------------------
namespace GeoEngine
{

enum 
{
	VOL_INTERP_NN	= 1,	// nearest neighbour volume interpolation
	VOL_INTERP_TRI	= 8,	// trilinear volume interpolation
};

//--------------------------------------------------------------------------------------------------
/// Non axis aligned bounding box
//--------------------------------------------------------------------------------------------------
class NonAABoundingBox
{
public:
	//--------------------------------------------------------------------------------------------------
	/// Initialise a non axis aligned bounding box given an origin 'pos', an orthonormal basis
	/// 'basis' (3 unit vectors) describing orientation and the extent 'size' of the the basis
	/// vectors. NOTE: The basis should be orthonormal, thus forming a valid box. Returns TRUE
	/// on success.
	/// \param[in]	basis	Three vectors describing the orientation of the box.
	/// \param[in]	pos		The world space origin of the box.
	/// \param[in] size		The world space size of the box.
	//--------------------------------------------------------------------------------------------------
	bool		Initialise(Geo::v128* basis, Geo::v128 pos, Geo::v128 size);

	/// Map a WC point to the local unit coordinate system of the volume box.
	Geo::v128	MapToLocalUnitCube(const Geo::v128& p) const;

	/// Map a point in the local unit coordinate system of the volume box to WC.
	Geo::v128	MapFromLocalUnitCube(const Geo::v128& p) const;

	/// Returns true if the supplied point is inside the box.
	bool		ContainsPoint(const Geo::v128& p) const;

	/// Returns the shortest distance from the supplied point to the box. NOTE: Returns a negative distance for points inside the box.
	float		SquaredDistanceToPoint(const Geo::v128& p) const;
	float		DistanceToPoint(const Geo::v128& p) const;

	/// Comparison operator.
	bool		operator==(const NonAABoundingBox& other) const;

	/// Comparison operator.
	bool		operator!=(const NonAABoundingBox& other) const;

	/// Accessors
	const Geo::v128& GetPosition( ) const { return m_Pos; };
	const Geo::v128& GetSize( ) const { return m_Size; };
	bool GetBasis( Geo::s32 row, Geo::v128& val ) const { if (row<0||row>2) return false; val=m_Basis[row]; return true; };

protected:
	/// Origo of the axis aligned box.
	Geo::v128	m_Pos;
	/// Size of the bounding volume (in the basis).
	Geo::v128	m_Size;
	/// An orthonormal basis (of unit vectors) describing the orientation of the box.
	Geo::v128	m_Basis[3];
	/// Matrix mapping from local to world coordinates.
	Geo::Matrix	m_LU2WC;
	/// Matrix mapping from world to local coordinates.
	Geo::Matrix	m_WC2LU;
	/// (Re)Initialise internal matrices.
	bool	SetMatrices();
};

//--------------------------------------------------------------------------------------------------
/// Non axis aligned volume, essentially a box with the notion of voxel resolution 
//--------------------------------------------------------------------------------------------------
class NonAAVolume: public NonAABoundingBox
{
public:
	NonAAVolume();

	/// Initialises a volume from a bounding box, sets up a voxel grid with resolution (resX,resY,resZ) 
	/// with 'num' samples per voxel. 'num' must be a multiple of 8. Sensible values for 'num' are 64, 128 or 256,
	/// default is 128. Group id (gId) is currently unused.
	bool				Initialise(const NonAABoundingBox* bb,Geo::u32 resX,Geo::u32 resY,Geo::u32 resZ);

	/// Returns the voxel X resolution
	Geo::u32			GetXRes() const {return m_XRes;};

	/// Returns the voxel Y resolution
	Geo::u32			GetYRes() const {return m_YRes;};

	/// Returns the voxel Z resolution
	Geo::u32			GetZRes() const {return m_ZRes;};

	/// Returns an index into a linear array computed from a 3D voxel index (x,y,z)
	bool				LineariseVoxelID(Geo::u32& i,const Geo::u32 x,const Geo::u32 y,const Geo::u32 z) const;

	/// Returns a 3D voxel index (x,y,z) computed from an index into a linear array
	bool				UnLineariseVoxelID(Geo::u32& x, Geo::u32& y, Geo::u32& z, Geo::u32 i) const;

	/// Central position of voxel
	bool				GetVoxelCentre(Geo::v128& point, Geo::u32 i) const;

	//--------------------------------------------------------------------------------------------------
	/// Computes the nearest voxel(s) to the sample point with appropriate interpolation weights.
	/// Returns TRUE on success. 
	/// \param[out]	voxIDs		The ids of the closest voxels (depending on INTERP can be 1 or 8).
	/// \param[out]	weights		Interpolation weights for the voxels (these will sum to 1).
	/// \param[in] point		The world space position to query.
	/// \param[in] INTERP		Number of voxels to interpolate. Can be 1 for nearest neighbour, or 8
	///							for tri-linear smooth interpolation.
	//--------------------------------------------------------------------------------------------------
	virtual bool		GetNearestSamples(Geo::s32 voxIDs[], float weights[], Geo::v128 point, Geo::s32 INTERP) const;

	/// Returns the number of interpolants
	virtual Geo::s32	GetNumProbes( void ) const {return m_XRes*m_YRes*m_ZRes;}

	/// Returns the maximum number of interpolants
	virtual Geo::s32	GetMaxNumInterpolants( void ) const {return 8;}

	// Shortest Distance from volume to a point
	virtual float		GetDistanceToPoint(const Geo::v128& point) const {return NonAABoundingBox::DistanceToPoint(point);};

	bool				operator==(const NonAAVolume& other) const;
	bool				operator!=(const NonAAVolume& other) const;

private:
	Geo::u32		m_XRes;
	Geo::u32		m_YRes;
	Geo::u32		m_ZRes;
};

}

#endif