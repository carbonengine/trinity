:orphan:

Modular ship assembly
=====================

A modular space object is a single ``EveSpaceObject2`` assembled at runtime from multiple SOF
hulls ("parts"), created via ``CreateModularObject`` and edited through the transient
``EveModularObjectModifier`` session object. Persistent per-part state lives in
``EveChildPartData``, an effect child on the object itself, so a saved or handed-off object
carries everything needed to reopen an edit session with ``ModifyModularObject``.

This page only describes the cross-cutting flow that no single file shows. API contracts live
with the API: the headers (``EveModularObjectModifier.h``, ``EveChildPartData.h``, ``EveSOF.h``) and the python docstrings on
``trinity.CreateModularObject`` and the modifier methods. Beyond the usage example below,
values and signatures are deliberately not repeated here.

Python example
--------------

Condensed from ``packages/trinity/tests/test_modular.py``, which exercises the full API and is
the authoritative reference for behavior. The modifier edits the object immediately, but
culling bounds are only committed by ``ApplyBounds``, or by dropping the last reference to the
modifier, which the example relies on.

.. code-block:: python

    import trinity

    IDENTITY_ROT = (0, 0, 0, 1)
    UNIT_SCALE = (1, 1, 1)

    sof = trinity.EveSOF()
    sof.dataMgr.LoadData('res:/dx9/model/spaceobjectfactory/data.red')

    # Create an empty modular object. The faction/race arguments seed the
    # defaults used when AddHull is passed empty strings.
    ship, modifier = trinity.CreateModularObject(sof, 'somefaction', 'somerace')

    core = modifier.AddHull('some_hull', '', '', (0, 0, 0), IDENTITY_ROT, UNIT_SCALE)
    wing = modifier.AddHull('other_hull', 'somefaction', 'somerace',
                            (30, 0, 0), IDENTITY_ROT, UNIT_SCALE)
    if wing == trinity.GetInvalidPartTag():
        raise RuntimeError('hull failed to build')

    # Non-SOF parts come from a space object child resource.
    beacon = modifier.AddChild('res:/model/somechild.red', (0, 50, 0), IDENTITY_ROT, UNIT_SCALE)

    modifier.SetTransform(wing, (-30, 0, 0), IDENTITY_ROT, UNIT_SCALE)
    modifier.Remove(beacon)  # KeyError on an unknown or already-removed tag

    del modifier  # last reference dropped: bounding sphere and shape ellipsoid commit here

    # Part tags stay valid across sessions: EveChildPartData persists them on the
    # object, so a saved/reloaded object reopens the same way.
    modifier = trinity.ModifyModularObject(ship, sof)
    assert modifier.GetPosition(wing) == (-30, 0, 0)
    del modifier

Part-tag propagation
--------------------

A part tag (``EveSpaceObjectChild::PartTag``, sentinels documented in ``EveSpaceObjectChild.h``
and ``EveModularObjectModifier.h``) identifies everything belonging to one part. It flows:

1. **Allocation**: ``EveModularObjectModifier::AllocatePartId`` (``EveModularObjectModifier.cpp``) takes
   the max over ``EveChildPartData::GetUnusedPartID`` and the tags of existing effect children.
2. **SOF build**: ``EveSOF::BuildChild`` (``EveSOF.cpp``) stamps the tag on every container and
   child it creates; nested layout placements flow it through ``EveSOF::SetupLayout`` /
   ``EveSOF::CreatePlacement``.
3. **Locators**: ``EveSOF::SetupLocatorSets`` stamps ``partTag`` on each generated locator
   (``EveSOFDataMgr::LocatorDirectionData`` converts to ``Locator`` preserving it), then merges
   into the object via ``EveSpaceObject2::MergeToLocatorSet``. The merged view built by
   ``EveSpaceObject2::EnsureChildLocatorMerged`` preserves per-locator tags.
4. **Mesh instances**: instanced meshes are shared across parts, so the tag is per *instance*,
   not per child. Each ``EveChildInstancedMeshes::Mesh`` carries a ``partTags`` vector parallel
   to the instance data (written in ``AddMesh``, consumed by ``RemoveInstancesByPartTag``). The
   child's own ``m_partTag`` is meaningless for instanced meshes.
5. **Effect children**: ``EveSpaceObjectChild::SetPartTag`` propagates through container
   overrides (``EveChildContainer::SetPartTag`` etc.), and ``EveSpaceObjectChild::RegisterChild``
   copies the parent's tag onto newly attached children.

Locator lifecycle during editing
--------------------------------

- **AddHull**: locators from every hull merge into the object's sets *by set name*, with no
  renaming or prefixing (``EveSpaceObject2::MergeToLocatorSet`` appends to an existing same-named
  set). Parts are distinguishable within a set only by ``partTag``.
- **Remove**: locators are stripped from every set by exact ``partTag`` match, mesh instances via
  ``RemoveInstancesByPartTag``, effect children by tag; accumulated impact damage is cleared.
- **SetTransform**: locators of the part are re-derived in place (position through
  inverse-old-transform then new-transform, direction and scale by delta), and the part's stored
  bounding sphere is re-transformed the same way. See
  ``EveModularObjectModifier::SetTransform`` (``EveModularObjectModifier.cpp``).
- **Damage locators / impact overlay**: the impact overlay allocates per-damage-locator slots, so
  its count must track the merged ``DAMAGE_LOCATOR_SET_NAME`` locator set. ``UpdateImpactOverlayLocatorCount``
  re-syncs it after AddHull/Remove; a stale count would index locators that no longer exist.
- Any structural edit calls ``EveSpaceObject2::InvalidateMergedLocators`` so the merged view is
  rebuilt lazily.

Gotchas
-------

- A modular object with zero parts (or before ``ApplyBounds``/modifier destruction ever ran) has
  a zero-radius bounding sphere: ``EveSpaceObject2::UpdateVisibility`` skips the mesh-visibility
  test and ``EveSpaceObject2::IsVisible`` culls it at any distance, so it never renders.
- Culling volumes are only pushed to the object by ``EveModularObjectModifier::ApplyBounds`` (the
  destructor calls it too). Editing without applying leaves the object rendering with stale
  bounds.
