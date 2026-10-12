# Copyright © 2026 CCP ehf.

cmake_minimum_required(VERSION 3.16)

project(TrinityShaders VERSION 1.0.0)

set(_SHADER_SOURCES
    include/Carbon/InteriorScene/PerObjectData.fxh
    include/Carbon/InteriorScene/PerSceneData.fxh
    include/Carbon/SpaceScene/Raytracing/GeometryData.fxh
    include/Carbon/SpaceScene/Raytracing/ShadowData.fxh
    include/Carbon/SpaceScene/BehaviorGroupBooster.fxh
    include/Carbon/SpaceScene/DepthMap.fxh
    include/Carbon/SpaceScene/DynamicLighting.fxh
    include/Carbon/SpaceScene/EnvironmentMap.fxh
    include/Carbon/SpaceScene/EveBannerSet.fxh
    include/Carbon/SpaceScene/EveBoosterSet.fxh
    include/Carbon/SpaceScene/EveChildBoosterSet.fxh
    include/Carbon/SpaceScene/EveChildBulletStorm.fxh
    include/Carbon/SpaceScene/EveChildCloud.fxh
    include/Carbon/SpaceScene/EveChildCloud2.fxh
    include/Carbon/SpaceScene/EveHazeSet.fxh
    include/Carbon/SpaceScene/EveLensflare.fxh
    include/Carbon/SpaceScene/EveOccluder.fxh
    include/Carbon/SpaceScene/EvePlaneSet.fxh
    include/Carbon/SpaceScene/EveChildQuad.fxh
    include/Carbon/SpaceScene/EveSpaceObjectDecal.fxh
    include/Carbon/SpaceScene/EveSpherePin.fxh
    include/Carbon/SpaceScene/EveSpotlightSet.fxh
    include/Carbon/SpaceScene/EveSpriteSet.fxh
    include/Carbon/SpaceScene/EveStarfield.fxh
    include/Carbon/SpaceScene/EveTransform.fxh
    include/Carbon/SpaceScene/EveStretch2.fxh
    include/Carbon/SpaceScene/EveTacticalOverlay.fxh
    include/Carbon/SpaceScene/EveTurretSet.fxh
    include/Carbon/SpaceScene/FroxelFog.fxh
    include/Carbon/SpaceScene/GpuParticles.fxh
    include/Carbon/SpaceScene/MeshMorphing.fxh
    include/Carbon/SpaceScene/PerObjectData.fxh
    include/Carbon/SpaceScene/PerSceneData.fxh
    include/Carbon/SpaceScene/Volumetrics.fxh
    include/Carbon/UI/UI.fxh
    include/Carbon/Bindless.fxh
    include/Carbon/BoneTransforms.fxh
    include/Carbon/Intersections.fxh
    include/Carbon/Math.fxh
    include/Carbon/PackedTangents.fxh
    include/Carbon/ParticleAnnotations.fxh
    include/Carbon/RandomXOrShift.fxh
    include/Carbon/System.fxh
    include/Carbon/Tr2CurveLineSet.fxh
    include/Carbon/VertexBufferUtil.fxh
    Managed/Space/SpecialFX/Lensflares/OccluderManagement.fx 
    Managed/Space/SpecialFX/Particles/Gpu/Clear.fx
    Managed/Space/SpecialFX/Particles/Gpu/Emit.fx
    Managed/Space/SpecialFX/Particles/Gpu/SetDrawParameters.fx
    Managed/Space/SpecialFX/Particles/Gpu/SetSortArgs.fx
    Managed/Space/SpecialFX/Particles/Gpu/Sort.fx
    Managed/Space/SpecialFX/Particles/Gpu/SortInner.fx
    Managed/Space/SpecialFX/Particles/Gpu/SortStep.fx
    Managed/Space/SpecialFX/Particles/Gpu/Update.fx
    Managed/Space/SpecialFX/Volumetric/Fog/ApplyFroxels.fx
    Managed/Space/SpecialFX/Volumetric/Fog/FilterFroxels.fx
    Managed/Space/SpecialFX/Volumetric/Fog/RaymarchFroxels.fx
    Managed/Space/SpecialFX/Volumetric/Fog/UpdateMieEnvironmentMap.fx
    Managed/Space/SpecialFX/Volumetric/BlurVolumetric.fx
    Managed/Space/SpecialFX/Volumetric/DownsampleDepth.fx
    Managed/Space/System/Raytracing/SkinVertices.fx
    Managed/Space/System/ComputeLightLists.fx
    Managed/Space/System/Denoise1D.fx
    Managed/Space/System/DenoiseEstimate.fx
    Managed/Space/System/EstimateNoise.fx
    Managed/Utility/BillboardNoZ.fx
    Managed/Utility/BillboardPicking.fx
    Managed/Utility/Debug.fx
    Managed/Utility/DebugLine.fx
    Managed/Utility/DebugPrimitive.fx
    Managed/Utility/DebugPrimitivePicking.fx
    Managed/Utility/Lines.fxh
    Managed/Utility/LinesNoZ.fx
    Managed/Utility/LinesRotationTool.fx
    Managed/Utility/LinesRotationToolLH.fx
    Managed/Utility/LinesWithZ.fx
    Managed/Utility/PerFrameVSData.fxh
    Managed/Utility/PrimitivePicking.fx
    Managed/Utility/SolidNoLightNoCullWithZ.fx
    Managed/Utility/Solids.fxh
    Managed/Utility/SolidsNoZ.fx
    Managed/Utility/SolidsWithZ.fx
    Utility/CubeTextureViewer.fx
    Utility/GeometryViewer.fx
    Utility/LineGraph.fx
    Utility/TextureViewer.fx
    )

add_custom_target(Shaders SOURCES ${_SHADER_SOURCES})
source_group(
    TREE "${CMAKE_CURRENT_SOURCE_DIR}"
    FILES ${_SHADER_SOURCES}
)

install(DIRECTORY . DESTINATION shaders FILES_MATCHING PATTERN "*.fx" PATTERN "*.fxh")
