#include "StdAfx.h"
#include "Tr2MeshArea.h"

Tr2MeshArea::Tr2MeshArea( IRoot* lockobj ):
	m_display( true ),
	m_index( 0 ),
    m_count( 1 ),
	m_reversed( false ),
	m_useSHLighting( false ),
	m_generateDepthArea( false ),
	m_castShadows( true ),
	m_jointCount( 0 ),
	m_jointMappingAnimRig( NULL ),
	m_minLod( TR2_LOD_UNSPECIFIED )
{
}

Tr2MeshArea::~Tr2MeshArea()
{
	SetJointMappingAnimRig( NULL );
}

void Tr2MeshArea::SetJointMappingAnimRig( unsigned int* val )
{
	m_jointMappingAnimRig = val;
}

Tr2MeshArea& Tr2MeshArea::operator=( const Tr2MeshArea& other )
{
	m_name = other.m_name;
	m_index = other.m_index;
	m_count = other.m_count;
	m_reversed = other.m_reversed;
	m_material = other.m_material;
	m_jointCount = 0;
	m_jointMappingAnimRig = NULL;
	m_display = other.m_display;
	m_useSHLighting = other.m_useSHLighting;
	m_generateDepthArea = other.m_generateDepthArea;

	return *this;
}

bool Tr2MeshArea::IsReversed() const
{
	return m_reversed;
}

const std::string& Tr2MeshArea::GetName() const
{
	return m_name;
}

int Tr2MeshArea::GetIndex() const
{
	return m_index;
}

void Tr2MeshArea::SetIndex( int ix )
{
	m_index = ix;
}

int Tr2MeshArea::GetCount() const
{
	return m_count;
}

void Tr2MeshArea::SetCount( int n )
{
	m_count = n;
}

// --------------------------------------------------------------------------------
bool Tr2MeshArea::GetDisplay() const
{
	return m_display;
}

// --------------------------------------------------------------------------------
void Tr2MeshArea::SetDisplay( bool display )
{
	m_display = display;
}

// --------------------------------------------------------------------------------
bool Tr2MeshArea::GetGenerateDepthArea() const
{
	return m_generateDepthArea;
}

// --------------------------------------------------------------------------------
void Tr2MeshArea::SetGenerateDepthArea( bool generate )
{
	m_generateDepthArea = generate;
}

// --------------------------------------------------------------------------------
bool Tr2MeshArea::IsCastingShadows() const
{
	return m_castShadows;
}

// --------------------------------------------------------------------------------
void Tr2MeshArea::SetCastsShadows( bool castShadows )
{
	m_castShadows = castShadows;
}


// -------------------------------------------------------------
// Description:
//   Returns a flag indicating that the area requires rendering
//   triangles in reversed order and reversed culling order.
// Return value:
//   true If the area requests reversed order of rendering
//   false If the area needs a normal order of rendering
// -------------------------------------------------------------
bool Tr2MeshArea::GetReversed() const
{
	return m_reversed;
}

// -------------------------------------------------------------
// Description:
//   Assigns a flag indicating that the area requires rendering
//   triangles in reversed order and reversed culling order.
// Arguments:
//   reversed - true If the area requests reversed order of rendering
//              false If the area needs a normal order of rendering
// -------------------------------------------------------------
void Tr2MeshArea::SetReversed( bool reversed )
{
	m_reversed = reversed;
}

// -------------------------------------------------------------
// Description:
//   Returns a flag indicating that the area requires SH lighting
//   insread of "normal" direct lightin. This flag is only checked
//   in interior scenes for transparent areas.
// Return value:
//   true If the area SH lighting
//   false If the area needs a normal lighting
// -------------------------------------------------------------
bool Tr2MeshArea::GetUseSHLighting() const
{
	return m_useSHLighting;
}

Tr2Material* Tr2MeshArea::GetMaterialInterface() const
{
	return m_material;
}

void Tr2MeshArea::SetMaterial( Tr2Material* mat )
{
	m_material = mat;
}

void Tr2MeshArea::SetName( const std::string& name )
{
	m_name = name;
}

unsigned int Tr2MeshArea::GetJointCount() const
{
	return m_jointCount;
}

void Tr2MeshArea::SetJointCount( unsigned int val )
{
	m_jointCount = val;
}

unsigned int* Tr2MeshArea::GetJointMappingAnimRig() const
{
	return m_jointMappingAnimRig;
}

Tr2Lod Tr2MeshArea::GetMinLod() const
{
	return m_minLod;
}

void Tr2MeshArea::SetMinLod( Tr2Lod lod )
{
	m_minLod = lod;
}