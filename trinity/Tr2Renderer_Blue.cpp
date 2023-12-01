#include "StdAfx.h"

#include "Tr2Renderer.h"

#include "TriPythonContext.h"

static void SetViewTransform( const Matrix& m )
{	
	Tr2Renderer::SetViewTransform( m );
}
MAP_FUNCTION_AND_WRAP( "SetViewTransform", SetViewTransform, 
				"Sets the view matrix for rendering.\n\n"
				":param matrix:  4-tuple of 4-tuples representing a D3D view matrix" );


static Matrix GetViewTransform()
{	
	return Tr2Renderer::GetViewTransform();
}
MAP_FUNCTION_AND_WRAP( "GetViewTransform", GetViewTransform, 
				"Returns the view matrix used for rendering.\n"
				"The matrix is a 4-tuple of 4-tuples representing a D3D view matrix" );


static void SetPerspectiveProjection( float fov, float front, float back, float asp )
{
	Tr2Renderer::SetPerspectiveProjection( fov, front, back, asp );
}
MAP_FUNCTION_AND_WRAP( "SetPerspectiveProjection", SetPerspectiveProjection,
				"Sets the perspective projection for rendering.\n\n"
				":param fov:   field of view on the y-axis, in radians,\n"
				":param front: z-value of the near view plane,\n"
				":param back:  z-value of the far view plane,\n"
				":param asp:   aspect ratio, defined as view space width divided by height." );

static Vector3 GetViewPosition()
{
	return Tr2Renderer::GetViewPosition();
}
MAP_FUNCTION_AND_WRAP( "GetViewPosition", GetViewPosition,
				"Returns the view (camera) position as a tuple of 3 floats." );

static Matrix GetProjectionTransform()
{
	return Tr2Renderer::GetProjectionTransform();
}
MAP_FUNCTION_AND_WRAP( "GetProjectionTransform", GetProjectionTransform, 
			"Returns the projection matrix used for rendering.\n"
			"The matrix is a 4x4 tuple of floats." );

static float GetFrontClip()
{
	return Tr2Renderer::GetFrontClip();
}
MAP_FUNCTION_AND_WRAP( "GetFrontClip", GetFrontClip, "Returns the near-plane z value." );

static float GetBackClip()
{
	return Tr2Renderer::GetBackClip();
}
MAP_FUNCTION_AND_WRAP( "GetBackClip", GetBackClip, "Returns the far-plane z value." );

static float GetFieldOfView()
{
	return Tr2Renderer::GetFieldOfView();
}
MAP_FUNCTION_AND_WRAP( "GetFieldOfView", GetFieldOfView, "Returns the field of view on the y-axis, in radians." );


static float GetAspectRatio()
{
	return Tr2Renderer::GetAspectRatio();
}
MAP_FUNCTION_AND_WRAP( "GetAspectRatio", GetAspectRatio, "Returns the aspect ratio, width divided by height." );

static void SetPerspectiveProjectionOffCenter( float left, float right, float bottom, float top, float front, float back )
{
	Tr2Renderer::SetPerspectiveProjection( left, right, bottom, top, front, back );
}
MAP_FUNCTION_AND_WRAP( "SetPerspectiveProjectionOffCenter", SetPerspectiveProjectionOffCenter,
			"This method allows an off-center projection to be set for the renderer.\n\n"
			":param left:   minimum x-value of the view volume,\n"
			":param right:  maximum x-value of the view volume,\n"
			":param bottom: minimum y-value of the view volume,\n"
			":param top:    maximum y-value of the view volume,\n"
			":param front:  minimum z-value of the view volume,\n"
			":param back:   maximum z-value of the view volume." );

#if BLUE_WITH_PYTHON
static PyObject* PySetShaderModel( PyObject* self, PyObject* args )
{
	TriPythonContext ctx;

	static bool isShaderModelChanging = false;
	if( !isShaderModelChanging )
	{
		isShaderModelChanging = true;
	}
	else
	{
		PyErr_SetString( PyExc_Exception, "Shader model change already in progress." );
		return NULL;
	}

	const char* name = 0;

	if( !PyArg_ParseTuple( args, "s", &name ))
	{
        isShaderModelChanging = false;
		return NULL;
	}

	TR2SHADERMODEL sm = TR2SM_COUNT;
	for( unsigned int i = 0; i < TR2SM_COUNT; ++i )
	{
		const char* candidate = Tr2Renderer::GetShaderModelString( (TR2SHADERMODEL)i );
		if( strcmp( name, candidate ) == 0 )
		{
			sm = (TR2SHADERMODEL)i;
			break;
		}
	}

	if( sm < TR2SM_COUNT )
	{
		Tr2Renderer::SetShaderModel( sm );
	}
	else
	{
		PyErr_SetString( PyExc_ValueError, "Shader model not recognized." );
        isShaderModelChanging = false;
		return NULL;
	}

	isShaderModelChanging = false;
	Py_RETURN_NONE;
}
MAP_FUNCTION( "SetShaderModel", PySetShaderModel,
			"Sets the shader model to use for rendering in Trinity.\n"
			"This is a combination of the D3D shader model to use, and a high/low fidelity version\n"
			"within that model. This value affects what .fx files are loaded by Tr2Effect objects.\n"
			"Currently supported values are (as strings):\n"
			"  SM_1_1\n"
			"  SM_2_0_LO\n"
			"  SM_2_0_HI\n"
			"  SM_3_0_LO\n"
			"  SM_3_0_HI\n"
			"  SM_3_0_DEPTH\n"
			"  SM_AUTHORING\n"
			"May raise a trinity.D3DERR_INVALIDCALL if a device reset is already in progress"
			"\n\n"
			":param sm: shader model name\n"
			":type sm: str\n"
			":rtype: None"
			);
#endif

static const char* GetShaderModel()
{
	return Tr2Renderer::GetShaderModelString( Tr2Renderer::GetShaderModel() );
}
MAP_FUNCTION_AND_WRAP( "GetShaderModel", GetShaderModel, "Gets the currently selected shader model (see SetShaderModel).\n" );

static uint64_t GetCurrentFrameCounter()
{
	return Tr2Renderer::GetCurrentFrameCounter();
}
MAP_FUNCTION_AND_WRAP( "GetCurrentFrameCounter", GetCurrentFrameCounter, "Gets the current frame counter.\n" );

static void AddMipLevelSkipExclusionDirectory( const char* name )
{
	Tr2Renderer::AddMipLevelSkipExclusionDirectory(name);
}

MAP_FUNCTION_AND_WRAP( "AddMipLevelSkipExclusionDirectory", AddMipLevelSkipExclusionDirectory,
			"Adds a resource directory to internal list. All textures in that directory are\n"
			"not downsized due to graphics quality.\n"
			"example: AddMipLevelSkipExclusionDirectory(\"res:/ui/\")\n"
			"To clear the internal directory-list, use ::ClearMipLevelSkipExclusionDirectories()\n"
			":param name: directory res path"
			);

static void EnableFallbackTextureDebugging()
{
	Tr2Renderer::EnableFallbackTextureDebugging();
}

MAP_FUNCTION_AND_WRAP(
	"EnableFallbackTextureDebugging",
	EnableFallbackTextureDebugging,
	"Enable debugging of fallback textures used by effect texture parameters when the original\n"
	"texture is missing. When debugging is enabled the texture is set to an ugly blinking mess and a\n"
	"warning is generated whenever the fallback texture is used." );

static void DisableFallbackTextureDebugging()
{
	Tr2Renderer::DisableFallbackTextureDebugging();
}

MAP_FUNCTION_AND_WRAP(
	"DisableFallbackTextureDebugging",
	DisableFallbackTextureDebugging,
	"Disables debugging of a fallback texture previously enabled by trinity.EnableFallbackTextureDebugging." );
