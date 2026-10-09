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
    src/Managed/Space/SpecialFX/Lensflares/OccluderManagement.fx 
    src/Managed/Space/SpecialFX/Particles/Gpu/Clear.fx
    src/Managed/Space/SpecialFX/Particles/Gpu/Emit.fx
    src/Managed/Space/SpecialFX/Particles/Gpu/SetDrawParameters.fx
    src/Managed/Space/SpecialFX/Particles/Gpu/SetSortArgs.fx
    src/Managed/Space/SpecialFX/Particles/Gpu/Sort.fx
    src/Managed/Space/SpecialFX/Particles/Gpu/SortInner.fx
    src/Managed/Space/SpecialFX/Particles/Gpu/SortStep.fx
    src/Managed/Space/SpecialFX/Particles/Gpu/Update.fx
    src/Managed/Space/SpecialFX/Volumetric/Fog/ApplyFroxels.fx
    src/Managed/Space/SpecialFX/Volumetric/Fog/FilterFroxels.fx
    src/Managed/Space/SpecialFX/Volumetric/Fog/RaymarchFroxels.fx
    src/Managed/Space/SpecialFX/Volumetric/Fog/UpdateMieEnvironmentMap.fx
    src/Managed/Space/SpecialFX/Volumetric/BlurVolumetric.fx
    src/Managed/Space/SpecialFX/Volumetric/DownsampleDepth.fx
    src/Managed/Space/System/Raytracing/SkinVertices.fx
    src/Managed/Space/System/ComputeLightLists.fx
    src/Managed/Space/System/Denoise1D.fx
    src/Managed/Space/System/DenoiseEstimate.fx
    src/Managed/Space/System/EstimateNoise.fx
    src/Managed/Utility/BillboardNoZ.fx
    src/Managed/Utility/BillboardPicking.fx
    src/Managed/Utility/Debug.fx
    src/Managed/Utility/DebugLine.fx
    src/Managed/Utility/DebugPrimitive.fx
    src/Managed/Utility/DebugPrimitivePicking.fx
    src/Managed/Utility/Lines.fxh
    src/Managed/Utility/LinesNoZ.fx
    src/Managed/Utility/LinesRotationTool.fx
    src/Managed/Utility/LinesRotationToolLH.fx
    src/Managed/Utility/LinesWithZ.fx
    src/Managed/Utility/PerFrameVSData.fxh
    src/Managed/Utility/PrimitivePicking.fx
    src/Managed/Utility/SolidNoLightNoCullWithZ.fx
    src/Managed/Utility/Solids.fxh
    src/Managed/Utility/SolidsNoZ.fx
    src/Managed/Utility/SolidsWithZ.fx
    src/Utility/CubeTextureViewer.fx
    src/Utility/GeometryViewer.fx
    src/Utility/LineGraph.fx
    src/Utility/TextureViewer.fx
    )

add_custom_target(Shaders SOURCES ${_SHADER_SOURCES})
source_group(
    TREE "${CMAKE_CURRENT_SOURCE_DIR}"
    FILES ${_SHADER_SOURCES}
)

install(DIRECTORY shaders/include DESTINATION shaders)
install(DIRECTORY shaders/src DESTINATION shaders)
