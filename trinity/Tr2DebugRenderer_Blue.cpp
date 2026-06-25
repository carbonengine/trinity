// Copyright © 2016 CCP ehf.

#include "StdAfx.h"
#include "Tr2DebugRenderer.h"

namespace
{

std::vector<std::string> GetDebugRenderOptions( IRoot* object )
{
	std::vector<std::string> result;
	if( auto renderable = dynamic_cast<ITr2DebugRenderable*>( object ) )
	{
		Tr2DebugRendererOptions options;
		renderable->GetDebugOptions( options );
		result.insert( result.end(), options.begin(), options.end() );
	}
	return result;
}

}

BLUE_DEFINE( Tr2DebugRenderer );


const Be::ClassInfo* Tr2DebugRenderer::ExposeToBlue(){
	EXPOSURE_BEGIN( Tr2DebugRenderer, "" )
		MAP_INTERFACE( Tr2DebugRenderer )

			MAP_METHOD_AND_WRAP(
				"SetSelectedObjects",
				SetSelectedObjects,
				"Assign a set of selected objects. Object may render differently in selected mode\n"
				":param objects: selected objects, as a list of tuples (object, sub-object id)\n" )
				MAP_METHOD_AND_WRAP(
					"GetOptions",
					GetOptions,
					"Returns a list of currently enabled options for an object\n"
					":param obj: object\n" )
					MAP_METHOD_AND_WRAP(
						"SetOptions",
						SetOptions,
						"Assign a set of visualization options for a single object\n"
						":param obj: object\n"
						":param options: set of visualization options\n" )
						MAP_METHOD_AND_WRAP(
							"GetDefaultOptions",
							GetDefaultOptions,
							"Returns a list of fallback visualization options\n" )
							MAP_METHOD_AND_WRAP(
								"SetDefaultOptions",
								SetDefaultOptions,
								"Assign a list of fallback visualization options\n"
								":param options: set of visualization options\n" )
								MAP_METHOD(
									"GetColorForOption",
									PyGetColorForOption,
									"Get color assigned to visulization option globally\n"
									":param option: the visualization option\n"
									":type: option: str\n"
									":returns: RGBA tuple each entry in range 0-1\n"
									":rtype: tuple[float, float, float, float]" )
									MAP_METHOD_AND_WRAP(
										"SetColorForOption",
										SetColorForOption,
										"Set color assigned to visulization option globally\n"
										":param option: the visualization option\n"
										":type: option: str\n"
										":param color: the color as a RGBA tuple each entry in range 0-1\n"
										":type: color: tuple[float, float, float, float]\n" )
										EXPOSURE_END()
}

MAP_FUNCTION_AND_WRAP(
	"GetDebugRenderOptions",
	GetDebugRenderOptions,
	"Returns possible debug rendering options for an object\n"
	":param obj: any blue-exposed object\n"
	":returns: list of option names that can be set with Tr2DebugRenderer" );