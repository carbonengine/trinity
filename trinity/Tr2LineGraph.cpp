#include "StdAfx.h"
#include "Tr2LineGraph.h"
#include "Tr2Renderer.h"
#include "Shader/Tr2Effect.h"
#include "Shader/Tr2Shader.h"

using namespace Tr2RenderContextEnum;

struct LineGraphVertex
{
	Vector2 pos;
};

static Tr2EffectPtr s_lineGraphEffect;
static TriVariable* s_lineGraphScale = NULL;
static TriVariable* s_lineGraphColor = NULL;
const char* LINEGRAPH_EFFECT_PATH = "res:/Graphics/Effect/Utility/LineGraph.fx";


Tr2LineGraph::Tr2LineGraph( IRoot* lockobj ) :
	m_data( "Tr2LineGraph/m_data" ),
	m_markers( "Tr2LineGraph/m_markers" ),
	m_primitiveCount( 0 ),
	m_currentIx( 0 ),
	m_color( 1.0f, 1.0f, 1.0f, 1.0f ),
	m_isPrepared( false ),
	m_maxValue( 0.0f ),
	m_vertexDeclaration( Tr2EffectStateManager::UNINITIALIZED_DECLARATION )
{
	m_data.resize( 200 );
}

Tr2LineGraph::~Tr2LineGraph()
{
}

/////////////////////////////////////////////////////////////////////////////
// ITriDeviceResource
bool Tr2LineGraph::OnPrepareResources()
{
	m_isPrepared = false;

	if( m_data.empty() )
	{
		m_primitiveCount = 0;
		return true;
	}

	if( m_vertexDeclaration == Tr2EffectStateManager::UNINITIALIZED_DECLARATION )
	{
		Tr2VertexDefinition vd;
		vd.Add( vd.FLOAT32_2, vd.POSITION );

		m_vertexDeclaration = Tr2EffectStateManager::GetVertexDeclarationHandle( vd );
		if( m_vertexDeclaration == Tr2EffectStateManager::UNINITIALIZED_DECLARATION )
		{
			return false;
		}
	}

	if( !m_vertexBuffer.IsValid() )
	{
		m_primitiveCount = (unsigned int)m_data.size() - 1;

		// Data sets are rendered as line lists, rather than strips, to allow one draw call
		// for all data sets.
		int vbSize = m_primitiveCount * 2 * sizeof( LineGraphVertex );
		if( vbSize )
		{
			USE_MAIN_THREAD_RENDER_CONTEXT();
			CR_RETURN_VAL( m_vertexBuffer.Create( 
				1,
				vbSize, 
				Tr2GpuUsage::VERTEX_BUFFER,
				Tr2CpuUsage::WRITE_OFTEN, 
				nullptr, 
				renderContext ), true );
		}
	}

	if( !s_lineGraphEffect )
	{
		s_lineGraphScale = GlobalStore().RegisterVariable( "lineGraphScale", 1.0f );
		s_lineGraphColor = GlobalStore().RegisterVariable( "lineGraphColor", Color( 1.0f, 1.0f, 1.0f, 1.0f ) );

		s_lineGraphEffect.CreateInstance();
		s_lineGraphEffect->SetEffectPathName( LINEGRAPH_EFFECT_PATH );
	}
	
	if( !m_vertexBuffer.IsValid() || m_vertexDeclaration == Tr2EffectStateManager::UNINITIALIZED_DECLARATION || !s_lineGraphEffect )
	{
		return true;
	}

	auto effectRes = s_lineGraphEffect->GetShaderStateInterface();
	if( !effectRes )
	{
		return true;
	}

	m_isPrepared = true;
	return true;
}

void Tr2LineGraph::ReleaseResources( TriStorage s )
{
	m_vertexBuffer = Tr2BufferAL();
	m_vertexDeclaration = Tr2EffectStateManager::UNINITIALIZED_DECLARATION;

	m_isPrepared = false;
}

void Tr2LineGraph::SetSize( unsigned int size )
{
	m_data.resize( size );
	m_currentIx = 0;

	// Vertex buffer must reflect new data set size
	m_vertexBuffer = Tr2BufferAL();
	m_isPrepared = false;
}

void Tr2LineGraph::Add( float value )
{
	if( !m_markers.empty() )
	{
		for( MarkerList_t::iterator it = m_markers.begin(); it != m_markers.end();  )
		{
			--it->m_ticksLeft;
			if( it->m_ticksLeft == 0 )
			{
				it = m_markers.erase( it );
			}
			else
			{
				++it;
			}
		}
	}

	m_data[m_currentIx] = value;
	++m_currentIx;
	m_currentIx %= m_data.size();
}

void Tr2LineGraph::Add( double value )
{
	Add( (float)value );
}

void Tr2LineGraph::AddMarker( const std::string& name )
{
	if( !m_markers.empty() )
	{
		Marker& lastMarkerAdded = m_markers.back();
		if( lastMarkerAdded.m_index == m_currentIx )
		{
			lastMarkerAdded.m_values.push_back( name );
			return;
		}
	}

	// No earlier marker added this frame - add this one.
	Marker m;
	m.m_index = m_currentIx;
	m.m_ticksLeft = (unsigned int)m_data.size();
	m.m_values.push_back( name );

	m_markers.push_back( m );
}

void Tr2LineGraph::Render( float scale, Tr2RenderContext& renderContext )
{
	if( m_isPrepared && ( !s_lineGraphEffect || !s_lineGraphEffect->GetShaderStateInterface() ) )
	{
		m_isPrepared = false;
	}

	if( !m_isPrepared )
	{
		PrepareResources();

		if( !m_isPrepared )
		{
			return;
		}
	}

	LineGraphVertex* pVerts;
	CR_RETURN( m_vertexBuffer.MapForWriting( pVerts, renderContext ) );

	LineGraphVertex* pVertsEnd = pVerts + m_primitiveCount * 2;

	unsigned int count = (unsigned int)m_data.size();
	
	float step = 2.0f / ((float)count - 1.0f);

	float x = -1.0f;

	m_maxValue = 0.0f;

	// Data is stored in a circular buffer - start at the oldest one,
	// where the current index is and the next item would go.
	float* p = &m_data[m_currentIx];
	for( unsigned int i = m_currentIx; i < count - 1; ++i )
	{
		if( *p > m_maxValue )
		{
			m_maxValue = *p;
		}

		pVerts->pos.x = x;
		pVerts->pos.y = *p;
		++pVerts;

		pVerts->pos.x = x + step;
		pVerts->pos.y = *(p + 1);
		++pVerts;

		++p;
		x += step;
	}

	// Then take the ones from the start of the buffer up the current index.
	if( m_currentIx > 0 )
	{
		// curIx is unsigned, so the condition in the for loop below is not
		// enough to prevent iterating when curIx is 0.

		pVerts->pos.x = x;
		pVerts->pos.y = *p;
		++pVerts;

		p = &m_data[0];
		pVerts->pos.x = x + step;
		pVerts->pos.y = *p;
		++pVerts;

		// Don't increment p - it was set to what would have been p+1 in the loop.
		x += step;

		for( unsigned int i = 0; i < m_currentIx - 1; ++i )
		{
			if( *p > m_maxValue )
			{
				m_maxValue = *p;
			}

			pVerts->pos.x = x;
			pVerts->pos.y = *p;
			++pVerts;

			pVerts->pos.x = x + step;
			pVerts->pos.y = *(p + 1);
			++pVerts;

			++p;
			x += step;
		}
	}

	CCP_ASSERT( pVerts == pVertsEnd );
	m_vertexBuffer.UnmapForWriting( renderContext );

	if( !m_markers.empty() )
	{
		Tr2Viewport viewport;
		renderContext.GetViewport( viewport );
		
		int yiTop = (int)( 0.1f*(float)viewport.m_height );
		int yiBottom = (int)( 0.95f*(float)viewport.m_height );
		int yi = yiTop;

		for( MarkerList_t::iterator it = m_markers.begin(); it != m_markers.end(); ++it )
		{
			float x = -1.0f + (float)(it->m_ticksLeft)*step;

			int xi = (int)( (x + 1.0f)*0.5f*(float)viewport.m_width );
			for( std::list<std::string>::iterator valueIt = it->m_values.begin(); valueIt != it->m_values.end(); ++valueIt )
			{
				Tr2Renderer::PrintfImmediate( renderContext, xi, yi, m_color, TRI_DFS_LEFT, valueIt->c_str() );
				yi += 10;

				if( yi > yiBottom )
				{
					yi = yiTop;
				}
			}
		}
	}

	auto effectRes = s_lineGraphEffect->GetShaderStateInterface();
	s_lineGraphScale->SetValue( scale );
	s_lineGraphColor->SetValue( m_color );

	effectRes->ApplyAllStateForPass( 0, 0, renderContext );
	s_lineGraphEffect->ApplyMaterialDataForPass( 0, 0, renderContext );

	renderContext.m_esm.ApplyVertexDeclaration( m_vertexDeclaration );
	renderContext.m_esm.ApplyStreamSource( 0, m_vertexBuffer, 0, sizeof( LineGraphVertex ) );
	renderContext.SetTopology( Tr2RenderContextEnum::TOP_LINES );
	renderContext.DrawPrimitive( 0, m_primitiveCount );
}

unsigned int Tr2LineGraph::GetSize()
{
	return (unsigned int)m_data.size();
}

const Color& Tr2LineGraph::GetColor() const
{
	return m_color;
}

void Tr2LineGraph::SetColor( const Color& val )
{
	m_color = val;
}

const std::string& Tr2LineGraph::GetName() const
{
	return m_name;
}

float Tr2LineGraph::GetMaxValue() const
{
	return m_maxValue;
}

std::vector<float> Tr2LineGraph::GetStatsHistory() const
{
	const unsigned int currentIndex = m_currentIx;
	const Tr2LineGraph::FloatVector_t& data = m_data;
	const unsigned int count = (unsigned int)data.size();

	std::vector<float> result;
	result.resize( count );

	unsigned int id = 0;
	// Go from the current index to the end
	typedef Tr2LineGraph::FloatVector_t::const_iterator it_type;
	for( it_type i = data.begin() + currentIndex; i != data.end(); ++i, ++id )
	{
		result[id] = *i;
	}

	// Then take the ones from the start of the buffer up the current index.
	for( it_type i = data.begin(); i != data.begin() + currentIndex; ++i, ++id )
	{
		result[id] = *i;
	}

	return result;
}
