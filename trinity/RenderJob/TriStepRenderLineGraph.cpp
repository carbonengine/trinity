#include "StdAfx.h"
#include "Tr2LineGraph.h"
#include "TriStepRenderLineGraph.h"
#include "Tr2Renderer.h"

TriStepRenderLineGraph::TriStepRenderLineGraph( IRoot* lockobj ) :
PARENTLOCK( m_lineGraphs ),
m_autoScale( true ),
m_showLegend( true ),
m_maxLegend( 1e12f ),
m_scale( 1.0f ),
m_legendScale( 1.0f )
{
}

TriStepRenderLineGraph::~TriStepRenderLineGraph()
{
}

// --------------------------------------------------------------------------------------
// Description:
//   Blue-exposed initializer. 
// --------------------------------------------------------------------------------------
void TriStepRenderLineGraph::py__init__( 
	const std::vector<Tr2LineGraph*>& graphs, 
	Be::Optional<float> legendScale,
	Be::Optional<float> scale,
	Be::Optional<bool> autoScale )
{
	for( auto it = graphs.begin(); it != graphs.end(); ++it )
	{
		m_lineGraphs.Insert( -1, *it );
	}
	if( legendScale.IsAssigned() )
	{
		m_legendScale = legendScale;
	}
	if( scale.IsAssigned() )
	{
		m_scale = scale;
	}
	if( autoScale.IsAssigned() )
	{
		m_autoScale = autoScale;
	}
}

TriStepResult TriStepRenderLineGraph::Execute( Be::Time realTime, Be::Time simTime, Tr2RenderContext& renderContext )
{
	if( m_autoScale )
	{
		float maxValue = 0.0f;
		for( Tr2LineGraphVector::iterator it = m_lineGraphs.begin(); it != m_lineGraphs.end(); ++it )
		{
			Tr2LineGraph* lg = *it;
			float x = lg->GetMaxValue();
			if( x > maxValue )
			{
				maxValue = x;
			}
		}

		if( maxValue == 0.0f )
		{
			maxValue = 1.0f;
		}

		maxValue *= 1.1f;

		if( maxValue > 1.0f )
		{
			maxValue /= 10.0f;
			maxValue = ceil( maxValue );
			maxValue *= 10.0f;
		}
		else
		{
			maxValue = 1.0f / maxValue;
			maxValue /= 10.0f;
			if( maxValue > 1.0f )
			{
				maxValue = floor( maxValue );
			}
			maxValue *= 10.0f;
			maxValue = 1.0f / maxValue;
		}

		if( maxValue * m_legendScale > m_maxLegend )
		{
			maxValue = m_maxLegend / m_legendScale;
		}

		float newScale = 1.0f / maxValue;
		if( newScale != m_scale )
		{
			m_scale = newScale;

			if( m_scaleChangeCallback )
			{
				m_scaleChangeCallback.CallVoid();
			}
		}
	}

	int y = 0;
	for( Tr2LineGraphVector::iterator it = m_lineGraphs.begin(); it != m_lineGraphs.end(); ++it )
	{
		Tr2LineGraph* lg = *it;
		lg->Render( m_scale, renderContext );

		y += 10;
	}

	if( m_showLegend )
	{
		int x = 2;
		y = 0;
		for( Tr2LineGraphVector::iterator it = m_lineGraphs.begin(); it != m_lineGraphs.end(); ++it )
		{
			Tr2LineGraph* lg = *it;
			Tr2Renderer::PrintfImmediate( renderContext, x, y, lg->GetColor() | 0xff000000, TRI_DFS_LEFT, lg->GetName().c_str() );

			y += 10;
		}

		Tr2Viewport viewport;
		renderContext.GetViewport( viewport );
		
		const int kNumLabels = 5;
		x = (unsigned)viewport.m_width - 20;
		int step = ((unsigned)viewport.m_height - 10) / (kNumLabels - 1);
		y = (unsigned)viewport.m_height - 10;
		float label = 0.0f;
		float labelStep = 1.0f / (float)(kNumLabels - 1);
		for( int i = 0; i < kNumLabels; ++i )
		{
			float labelValue = label / m_scale * m_legendScale + 0.5f;
			int intLabelValue = (int)labelValue;
			Tr2Renderer::PrintfImmediate( renderContext, x, y, 0xffffffff, TRI_DFS_RIGHT, "%d", intLabelValue );
			label += labelStep;
			y -= step;
		}	
	}

	return RS_OK;
}

