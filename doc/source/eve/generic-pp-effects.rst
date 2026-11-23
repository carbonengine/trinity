Custom Post-Processing Effects
===============================

Trinity supports having custom post-processing effects that can be applied to the rendered scene. These effects are specified in post-process volume 
objects (see ``Tr2PostProcessAttributes`` and ``EveChildPostProcessVolume``). The Tr2PostProcessAttributes maintains a list ``genericEffects`` that contains
custom post-processing effects (of type ``Tr2PPGenericEffect``). Each of these represents a custom post-processing pass.

``Tr2PPGenericEffect`` objects have members:

- ``effect``: The shader effect used for this post-processing pass.
- ``quality``: Minimal post-processing quality setting when this effect is visible.
- ``executionSlot``: Place in the post-processing pipeline where this effect should be executed.
- ``order``: The order in which this effect should be applied relative to other effects in the same execution slot.

Effect Files (Shaders)
----------------------

Effect files (.fx) for custom post-processing effects are used to define the shader programs that implement the visual effects. These effects need
to have a `"Main"` technique defined, which serves as the entry point for the post-processing effect. Trinity will render a quad using this technique 
to apply the effect to the scene.

Trinity will pass pre-transform position (in clip space) and texture coordinates (0 to 1) to the vertex shader of the `"Main"` technique: ::

    struct PostProcessVertex
    {
        float4 pos : POSITION;
        float2 texCoord : TEXCOORD0;
    };


The effect may use the output of the previous post-processing step as the input texture. For that, the shader needs to define a ``Texture2D`` object
with the name "Blit". It is possible to have a shader that does not use the previous post-processing output, in which case the Trinity will render 
the effect into the output of the previous pass directly. This can be used for overlay/additive effects.

The effect may use other scene-wide textures, like `DepthMap` or `VelocityMap`. Note that currently, there is no guarantee that these textures will always be 
available, especially at the execution slots later in the pipeline. Normally, `DepthMap` is safe to use at most stages, but `VelocityMap` may not be available 
after tonemapping, or with certain rendering settings.

The effect may expose parameters that can be adjusted by post-processing based on the volume intensity/relative weight. Such parameters need to have
floating-point type (e.g., ``float``, ``float2``, ``float3``, ``float4``) and have ``bool IsBlendable = true;`` annotation. When rendering the post-processing effect, 
Trinity will blend these parameters based on the volume intensity/relative weight. For each effect instanced, it will interpolate such parameters between their
default values, specified in HLSL code and the values specified in the effect's parameter object.


Blending and Grouping
---------------------

Generally, Trinity treats each custom post-processing effect as an individual pass. This means that different effects are applied separately, 
without any automatic blending or overriding each other.

For identical effects, Trinity tries to group them together to optimize rendering. Effects that share the same shader (along with all the non-blendable
material parameters), execution slot, and quality threshold are merged together and blended as a single post-processing effect. When custom effects are 
blended together, all of their blendable parameters are blended using the normal rules similar to any other post-processing attribute.


Execution Slots
---------------

Execution slots determine the place in the post-processing pipeline where the effect should be executed:

- ``BEFORE_UPSCALING``: The effect is applied before the upscaling step or TAA in the rendering pipeline, after legacy fog and godray passes. Note that the
  input Src texture and the required output are in linear HDR color space. 
- ``AFTER_TONEMAP``: The effect is applied after the tonemapping step in the rendering pipeline. The input Src texture and the required output are in the 
  final color space, typically sRGB. Note that the scene-wide textures like `DepthMap` and `VelocityMap` may not be available at this stage, and if they
  are, they may be of a different resolution than the target one, and may have camera jitter applied.

