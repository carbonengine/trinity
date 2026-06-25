// Copyright © 2023 CCP ehf.

#include "StdAfx.h"
#include "TriGrannyRes.h"
#include "TriGeometryRes.h"



BLUE_DEFINE( Tr2GrannyIntersectionResult );

const Be::ClassInfo* Tr2GrannyIntersectionResult::ExposeToBlue(){
	EXPOSURE_BEGIN( Tr2GrannyIntersectionResult, "" )
		MAP_ATTRIBUTE( "position", m_result.position, "", Be::READWRITE )
			MAP_ATTRIBUTE( "hasPosition", m_result.hasPosition, "", Be::READWRITE )
				MAP_ATTRIBUTE( "normal", m_result.normal, "", Be::READWRITE )
					MAP_ATTRIBUTE( "hasNormal", m_result.hasNormal, "", Be::READWRITE )
						MAP_ATTRIBUTE( "uv", m_result.uv, "", Be::READWRITE )
							MAP_ATTRIBUTE( "hasUv", m_result.hasUv, "", Be::READWRITE )
								MAP_ATTRIBUTE( "boneIndex", m_result.boneIndex, "", Be::READWRITE )
									MAP_ATTRIBUTE( "hasBoneIndex", m_result.hasBoneIndex, "", Be::READWRITE )
										MAP_ATTRIBUTE( "meshIndex", m_result.meshIndex, "", Be::READWRITE )
											MAP_ATTRIBUTE( "areaIndex", m_result.areaIndex, "", Be::READWRITE )
												EXPOSURE_END()
}

BLUE_DEFINE( TriGrannyRes );


const Be::ClassInfo* TriGrannyRes::ExposeToBlue(){
	EXPOSURE_BEGIN( TriGrannyRes, "" )
		MAP_INTERFACE( TriGrannyRes )
			MAP_INTERFACE( IBlueResource )
				MAP_INTERFACE( ICacheable )
					MAP_ICACHEABLE_METHODS()

						MAP_METHOD_AND_WRAP(
							"CreateGeometryRes",
							CreateGeometryRes,
							"y = x.CreateGeometryRes()\n"
							"Create a TriGeometryRes from this resource. Useful for baking blendshapes, for example." )

							MAP_METHOD_AND_WRAP(
								"BakeBlendshape",
								BakeBlendshapeFromScript,
								"x.BakeBlendshape( mesh, weights, geom )\n"
								"Bake a blendshape with the given weights, using the resulting vertex buffer to replace the vertex buffer of the given TriGeometryRes.\n"
								":param mesh: index of the mesh.\n"
								":param weights: a list with weights as floating point numbers.\n"
								":param geom: a TriGeometryRes object that receives the resulting vertex buffer. Note that it should have been created with a call to x.CreateGeometryRes().\n" )
								MAP_PROPERTY_READONLY(
									"modelCount",
									GetModelCount,
									"Gets the count of models in the Granny file" )
									MAP_METHOD_AND_WRAP(
										"GetModelName",
										GetModelName,
										"s = x.GetModelName( ix )\n"
										"Gets the name of the model with index 'ix'\n"
										":param ix: index of the model\n" )
										MAP_PROPERTY_READONLY(
											"meshCount",
											GetMeshCount,
											"Gets the count of meshes in this Granny file." )
											MAP_METHOD_AND_WRAP(
												"GetMeshCount",
												GetMeshCount,
												"Gets the count of meshes in this Granny file." )
												MAP_METHOD_AND_WRAP(
													"GetMeshAreaCount",
													GetMeshAreaCount,
													"Gets the count of areas within a specified meshID in this Granny file.\n"
													":param meshIdx: mesh index" )
													MAP_METHOD_AND_WRAP(
														"GetMeshName",
														GetMeshName,
														"Gets the name of the mesh with the given index.\n"
														":param meshIdx: mesh index" )
														MAP_METHOD_AND_WRAP(
															"GetMeshBoneBindings",
															GetMeshBoneBindings,
															"Gets the bone bindings of the mesh with the given index.\n"
															":param meshIdx: mesh index" )
															MAP_METHOD_AND_WRAP(
																"GetMeshVertexElements",
																GetMeshVertexElements,
																"Gets the vertex declaration elements (list of (usage, usageIndex) tuples) of the mesh with the given index.\n"
																":param meshIdx: mesh index" )
																MAP_METHOD_AND_WRAP(
																	"GetMeshTriangleCount",
																	GetMeshTriangleCount,
																	"Gets the triangle count of the mesh with the given index.\n"
																	":param meshIdx: mesh index" )
																	MAP_METHOD_AND_WRAP(
																		"GetMeshMorphCount",
																		GetMeshMorphCount,
																		"Gets the count of morph targets available for the given mesh.\n"
																		":param meshIdx: mesh index" )
																		MAP_METHOD_AND_WRAP(
																			"GetMeshMorphName",
																			GetMeshMorphName,
																			"Gets the name of given morph target index for the given mesh.\n"
																			":param meshIdx: mesh index\n"
																			":param morphIdx: morph index" )
																			MAP_METHOD_AND_WRAP(
																				"GetAllMeshMorphNamesNoDigits",
																				GetAllMeshMorphNamesNoDigits,
																				"Returns a list containing the names of all the morph targets,\n"
																				"all converted to lowercase and stripped of any digits.  Specialized\n"
																				"method to get answers in a format that python wants in a single call.\n"
																				":param meshIdx: mesh index\n" )
																				MAP_PROPERTY_READONLY(
																					"animationCount",
																					GetAnimationCount,
																					"Gets the count of animation in the Granny file" )
																					MAP_METHOD_AND_WRAP(
																						"GetAnimationName",
																						GetAnimationName,
																						"Gets the name of the animation with index 'ix'\n"
																						":param ix: index of the animation\n" )
																						MAP_METHOD_AND_WRAP(
																							"GetAnimationDuration",
																							GetAnimationDuration,
																							"Gets the duration of the animation with index 'ix'\n"
																							":param ix: index of the animation\n" )

#if WITH_GRANNY
																							MAP_METHOD_AND_WRAP( "GetTrackGroupCount", GetTrackGroupCount, "Get the number of track groups" )
																								MAP_METHOD_AND_WRAP(
																									"GetTrackGroupName",
																									GetTrackGroupName,
																									"Get the name of the track group at index\n"
																									":param groupIdx: track group index" )
																									MAP_METHOD_AND_WRAP(
																										"GetVectorTrackCount",
																										GetVectorTrackCount,
																										"Get the number of transform tracks under a track group\n"
																										":param groupIdx: track group index\n" )
																										MAP_METHOD_AND_WRAP(
																											"GetVectorTrackName",
																											GetVectorTrackName,
																											"Get the name of the transform track\n"
																											":param groupIdx: track group index\n"
																											":param trackIdx: track index\n" )
																											MAP_METHOD_AND_WRAP(
																												"GetEventTrackCount",
																												GetEventTrackCount,
																												"Get the number of event (text) tracks under a track group\n"
																												":param groupIdx: track group index\n" )
																												MAP_METHOD_AND_WRAP(
																													"GetEventTrackName",
																													GetEventTrackName,
																													"Get the name of the event (text) track\n"
																													":param groupIdx: track group index\n"
																													":param trackIdx: track index\n" )
#endif

																													MAP_METHOD_AND_WRAP(
																														"RayIntersection",
																														RayIntersection,
																														"Returns ray intersection"
																														":param position: ray position\n"
																														":param direction: ray direction\n"
																														":param meshIndex: mesh index, intersect with all meshes if <0\n"
																														":param areaIndex: area index, intersect with all areas if <0\n" )

																														MAP_METHOD_AND_WRAP(
																															"LoadForTools",
																															Load,
																															"Loads geometry resource immediately blocking the thread/tasklet. Shoud only be used by tools, never in the client.\n\n"
																															":param path: path to the resource to load\n"
																															":return: true if loading succeeded, false otherwise\n" )

																															EXPOSURE_CHAINTO( BlueAsyncRes )
}

#if WITH_GRANNY
MAP_FUNCTION_AND_WRAP(
	"GrannyRayIntersection",
	GrannyRayIntersectionWrap,
	"Returns ray intersection"
	":param granny: granny info pointer\n"
	":param position: ray position\n"
	":param direction: ray direction\n"
	":param meshIndex: mesh index, intersect with all meshes if <0\n"
	":param areaIndex: area index, intersect with all areas if <0\n" );
#endif
