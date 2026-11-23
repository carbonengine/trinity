import contextlib
import unittest

import blue
import trinity

from ._util import TempRes


IDENTITY = (0, 0, 0, 1)
UNIT = (1, 1, 1)
FACTION = 'testfaction'
RACE = 'testrace'


def _CreateSof():
    data = trinity.EveSOFData()
    data.generic = trinity.EveSOFDataGeneric()
    shader = trinity.EveSOFDataGenericShader()
    shader.shader = 'my_shader.fx'
    data.generic.areaShaders.append(shader)

    hull = trinity.EveSOFDataHull()
    hull.name = 'static_hull'
    hull.geometryResFilePath = 'res:/mygeo.cmf'
    hull.boundingSphere = (0.0, 0.0, 0.0, 50.0)
    area = trinity.EveSOFDataHullArea()
    area.name = 'area'
    area.shader = shader.shader
    area.index = 0
    area.count = 1
    hull.opaqueAreas.append(area)
    locatorSet = trinity.EveSOFDataHullLocatorSet()
    locatorSet.name = "damage"
    locator = trinity.EveSOFDataTransform()
    locator.position = 0, 0, 0
    locatorSet.locators.append(locator)
    hull.locatorSets.append(locatorSet)
    data.hull.append(hull)

    hull = trinity.EveSOFDataHull()
    hull.name = 'static_hull2'
    hull.geometryResFilePath = 'res:/mygeo2.cmf'
    hull.boundingSphere = (0.0, 0.0, 0.0, 60.0)
    area = trinity.EveSOFDataHullArea()
    area.name = 'area'
    area.shader = shader.shader
    area.index = 0
    area.count = 1
    hull.opaqueAreas.append(area)
    data.hull.append(hull)

    hull = trinity.EveSOFDataHull()
    hull.name = 'anim_hull'
    hull.isSkinned = True
    hull.geometryResFilePath = 'res:/mygeo.cmf'
    hull.boundingSphere = (0.0, 0.0, 0.0, 70.0)
    area = trinity.EveSOFDataHullArea()
    area.name = 'area'
    area.shader = shader.shader
    area.index = 0
    area.count = 1
    hull.opaqueAreas.append(area)
    data.hull.append(hull)

    hull = trinity.EveSOFDataHull()
    hull.name = 'audio_hull'
    hull.geometryResFilePath = 'res:/mygeo.cmf'
    hull.boundingSphere = (0.0, 0.0, 0.0, 50.0)
    area = trinity.EveSOFDataHullArea()
    area.name = 'area'
    area.shader = shader.shader
    area.index = 0
    area.count = 1
    hull.opaqueAreas.append(area)
    emitter = trinity.EveSOFDataHullSoundEmitter()
    emitter.name = 'engine_sound'
    emitter.position = (1, 2, 3)
    hull.soundEmitters.append(emitter)
    data.hull.append(hull)

    faction = trinity.EveSOFDataFaction()
    faction.name = 'testfaction'
    data.faction.append(faction)
    race = trinity.EveSOFDataRace()
    race.name = 'testrace'
    data.race.append(race)
    sof = trinity.EveSOF()
    sof.dataMgr.SetData(data)
    return sof


class TestModular(unittest.TestCase):
    def setUp(self):
        self.sof = _CreateSof()
        self.ship, modifier = trinity.CreateModularObject(self.sof, FACTION, RACE)
        del modifier
        self._m = None
        self._sceneTime = 0

    @contextlib.contextmanager
    def modify(self):
        # The modifier commits deferred state (bounding sphere / shape ellipsoid)
        # in its destructor. Holding it only on self and clearing it here drops the
        # last reference at block exit, which is the commit point the tests rely on.
        self._m = trinity.ModifyModularObject(self.ship, self.sof)
        try:
            yield self._m
        finally:
            self._m = None

    def addHull(self, name='static_hull', pos=(0, 0, 0), rot=IDENTITY, scale=UNIT,
                faction=FACTION, race=RACE):
        return self._m.AddHull(name, faction, race, pos, rot, scale)

    def instancedMeshes(self):
        return blue.FindInterface(self.ship, 'EveChildInstancedMeshes')[0]

    def instanceCount(self, meshIndex=0):
        return self.instancedMeshes().GetMeshInfo(meshIndex)[6]

    def updateTransforms(self):
        self._sceneTime += 1
        scene = trinity.EveSpaceScene()
        scene.objects.append(self.ship)
        scene.UpdateScene(self._sceneTime)

    def assertBounds(self, radius, center):
        self.assertEqual(self.ship.boundingSphereRadius, radius)
        self.assertEqual(self.ship.boundingSphereCenter, center)

    def test_createEmptyShip(self):
        self.assertIsNotNone(self.ship)
        self.assertEqual(len(self.ship.effectChildren), 1)
        self.assertIsInstance(self.ship.effectChildren[0], trinity.EveChildPartData)

    def test_addStaticHull(self):
        with self.modify():
            self.addHull()

        # Static meshes are added as instanced
        self.assertEqual(self.instancedMeshes().GetMeshCount(), 1)

    def test_addAnimatedHull(self):
        with self.modify():
            self.addHull('anim_hull')

        self.assertEqual(len(blue.FindInterface(self.ship, 'EveChildInstancedMeshes')), 0)
        self.assertEqual(len(blue.FindInterface(self.ship, 'EveChildMesh')), 1)

    def test_addTwoStaticHulls(self):
        with self.modify():
            self.addHull()
            self.addHull(pos=(1, 2, 3))

        # Static meshes are added as instanced
        self.assertEqual(self.instancedMeshes().GetMeshCount(), 1)
        self.assertEqual(self.instanceCount(), 2)

    def test_removeOneOfTwoInstances(self):
        with self.modify():
            self.addHull()
            part_tag = self.addHull(pos=(1, 2, 3))

        # Static meshes are added as instanced
        self.assertEqual(self.instancedMeshes().GetMeshCount(), 1)
        self.assertEqual(self.instanceCount(), 2)

        with self.modify():
            self._m.Remove(part_tag)

        self.assertEqual(self.instancedMeshes().GetMeshCount(), 1)
        self.assertEqual(self.instanceCount(), 1)

    def test_addTwoDifferentStaticHulls(self):
        with self.modify():
            self.addHull()
            self.addHull('static_hull2', pos=(1, 2, 3))

        # Static meshes are added as instanced
        self.assertEqual(self.instancedMeshes().GetMeshCount(), 2)
        self.assertEqual(self.instanceCount(0), 1)
        self.assertEqual(self.instanceCount(1), 1)

    def test_addHullUnknownHullReturnsInvalidTag(self):
        with self.modify():
            tag = self.addHull('invalid_hull_name')

        self.assertEqual(tag, trinity.GetInvalidPartTag())
        self.assertEqual(len(blue.FindInterface(self.ship, 'EveChildInstancedMeshes')), 0)

    def test_removeAnimatedHull(self):
        with self.modify():
            part = self.addHull('anim_hull')

        self.assertEqual(len(blue.FindInterface(self.ship, 'EveChildInstancedMeshes')), 0)
        self.assertEqual(len(blue.FindInterface(self.ship, 'EveChildMesh')), 1)

        with self.modify():
            self._m.Remove(part)

        self.assertEqual(len(blue.FindInterface(self.ship, 'EveChildMesh')), 0)

    def test_removeStaticHull(self):
        with self.modify():
            part = self.addHull()

        # Static meshes are added as instanced
        self.assertEqual(self.instancedMeshes().GetMeshCount(), 1)

        with self.modify():
            self._m.Remove(part)

        self.assertEqual(self.instancedMeshes().GetMeshCount(), 0)

    def test_readdStaticHullAfterRemove(self):
        with self.modify():
            part = self.addHull()

        with self.modify():
            self._m.Remove(part)

        with self.modify():
            part = self.addHull(pos=(1, 2, 3))

        self.assertNotEqual(part, trinity.GetInvalidPartTag())
        self.assertEqual(self.instancedMeshes().GetMeshCount(), 1)
        self.assertEqual(self.instanceCount(), 1)

    def test_setBoundingSphere(self):
        with self.modify():
            self.addHull()

        self.assertEqual(self.ship.boundingSphereRadius, 50)

    def test_boundingSphereAccountsForTransforms(self):
        with self.modify():
            self.addHull(pos=(30, 0, 0))

        self.assertBounds(50.0, (30, 0, 0))

    def test_boundingSphereEncapsulatesAllChildren(self):
        with self.modify():
            self.addHull(pos=(30, 0, 0))
            self.addHull(pos=(-30, 0, 0))

        self.assertBounds(30.0 + 50.0, (0, 0, 0))

    def test_boundingSphereUpdatesOnRemoval(self):
        with self.modify():
            part_tag = self.addHull(pos=(30, 0, 0))
            self.addHull(pos=(-30, 0, 0))

        self.assertBounds(30.0 + 50.0, (0, 0, 0))

        with self.modify():
            self._m.Remove(part_tag)

        self.assertBounds(50, (-30, 0, 0))

    def test_boundingSphereUpdatesOnSetTransform(self):
        with self.modify():
            part_tag = self.addHull(pos=(30, 0, 0))

        self.assertBounds(50, (30, 0, 0))

        with self.modify():
            self._m.SetTransform(part_tag, (-30, 0, 0), IDENTITY, UNIT)

        self.assertBounds(50, (-30, 0, 0))

    def test_shapeEllipsoidEncapsulatesAllParts(self):
        with self.modify():
            self.addHull(pos=(30, 0, 0))
        self.assertEqual(self.ship.shapeEllipsoidCenter, (30, 0, 0))

        with self.modify():
            self.addHull(pos=(-30, 0, 0))
        self.assertEqual(self.ship.shapeEllipsoidCenter, (0, 0, 0))

    def test_addsLocators(self):
        with self.modify():
            self.addHull(pos=(30, 0, 0))

        # part locators are owned by the part's children and only exist in the
        # merged view; nothing bakes into the ship's own locator sets
        self.assertEqual(len(self.ship.locatorSets), 0)
        self.assertEqual(self.ship.GetDamageLocatorCount(), 1)

    def test_locatorsInheritTransform(self):
        with self.modify():
            self.addHull(pos=(30, 0, 0))

        self.assertEqual(self.ship.GetDamageLocator(0), (30, 0, 0))

    def test_addsLocatorsMerged(self):
        with self.modify():
            self.addHull(pos=(30, 0, 0))
            self.addHull(pos=(-30, 0, 0))

        self.assertEqual(self.ship.GetDamageLocatorCount(), 2)

    def test_addsLocatorsRemoved(self):
        with self.modify():
            part = self.addHull(pos=(30, 0, 0))
            self.addHull(pos=(-30, 0, 0))

        with self.modify():
            self._m.Remove(part)

        self.assertEqual(self.ship.GetDamageLocatorCount(), 1)
        self.assertEqual(self.ship.GetDamageLocator(0), (-30, 0, 0))

    def test_moveAnimatedHull(self):
        with self.modify():
            part = self.addHull('anim_hull', pos=(10, 20, 0))

        self.updateTransforms()
        mesh = blue.FindInterface(self.ship, 'EveChildMesh')[0]
        self.assertEqual(mesh.worldTransform, ((1, 0, 0, 0), (0, 1, 0, 0), (0, 0, 1, 0), (10, 20, 0, 1)))

        with self.modify():
            self._m.SetTransform(part, (1, 2, 3), IDENTITY, UNIT)

        self.updateTransforms()

        self.assertEqual(mesh.worldTransform, ((1, 0, 0, 0), (0, 1, 0, 0), (0, 0, 1, 0), (1, 2, 3, 1)))

    def test_transformStaticHull(self):
        # PLAT-11963: static (instanced) parts don't move on SetTransform.
        rot180z = (0, 0, 1, 0)

        def transforms():
            return sorted(self.instancedMeshes().GetInstancesTransforms(0))

        with self.modify():
            self.addHull()

        with self.modify():
            core = self.addHull()
            self.addHull(pos=(450, 0, 0))

        self.assertEqual(transforms(), [
            ((0, 0, 0), IDENTITY, UNIT),
            ((0, 0, 0), IDENTITY, UNIT),
            ((450, 0, 0), IDENTITY, UNIT),
        ])

        with self.modify():
            self._m.SetTransform(core, (0, 1000, 0), rot180z, (2, 4, 8))

        self.assertEqual(transforms(), [
            ((0, 0, 0), IDENTITY, UNIT),
            ((0, 1000, 0), rot180z, (2, 4, 8)),
            ((450, 0, 0), IDENTITY, UNIT),
        ])

    def test_moveLocators(self):
        with self.modify():
            part = self.addHull(pos=(10, 20, 0))

        self.assertEqual(self.ship.GetDamageLocator(0), (10, 20, 0))

        with self.modify():
            self._m.SetTransform(part, (1, 2, 3), IDENTITY, UNIT)

        self.assertEqual(self.ship.GetDamageLocator(0), (1, 2, 3))

    def test_partIdsAreUnique(self):
        with self.modify():
            part_tag_1 = self.addHull(pos=(10, 20, 0))
            part_tag_2 = self.addHull('anim_hull', pos=(10, 20, 0))

        self.assertNotEqual(part_tag_1, part_tag_2)
        self.assertNotEqual(part_tag_1, trinity.GetInvalidPartTag())
        self.assertNotEqual(part_tag_2, trinity.GetInvalidPartTag())

    def test_partIdRecycledAfterRemove(self):
        pos1 = (20, 10, 0)
        pos2 = (10, 20, 0)
        with self.modify():
            part_tag_1 = self.addHull(pos=pos1)
            self._m.Remove(part_tag_1)
            part_tag_2 = self.addHull(pos=pos2)
            self.assertEqual(part_tag_1, part_tag_2)
            self.assertEqual(pos2, self._m.GetPosition(part_tag_2))

        self.assertEqual(self.ship.GetDamageLocatorCount(), 1)
        self.assertEqual(self.ship.GetDamageLocator(0), pos2)

    def test_removeWrongID(self):
        with self.modify():
            self.addHull('anim_hull', pos=(10, 20, 0))

            with self.assertRaises(KeyError):
                self._m.Remove(5000)

    def test_setTransformRemovedPartRaises(self):
        with self.modify():
            part_tag = self.addHull(pos=(30, 0, 0))
            self._m.Remove(part_tag)

        with self.modify():
            with self.assertRaises(KeyError):
                self._m.SetTransform(part_tag, (-30, 0, 0), IDENTITY, UNIT)

    def test_addChild(self):
        with self.modify():
            with TempRes():
                child = trinity.EveChildMesh()
                blue.resMan.SaveObject(child, 'res:/child.red')
                part_tag = self._m.AddChild('res:/child.red', (10, 20, 30), IDENTITY, UNIT)

            self.assertNotEqual(part_tag, trinity.GetInvalidPartTag())
            self.assertEqual(len(blue.FindInterface(self.ship, 'EveChildMesh')), 1)
            self.assertEqual(self._m.GetPosition(part_tag), (10, 20, 30))

    def test_addChildInvalidPathReturnsInvalidTag(self):
        with self.modify():
            part_tag = self._m.AddChild('res:/does_not_exist.red', (0, 0, 0), IDENTITY, UNIT)

        self.assertEqual(part_tag, trinity.GetInvalidPartTag())
        self.assertEqual(len(blue.FindInterface(self.ship, 'EveChildMesh')), 0)

    def test_getRotationAndScale(self):
        with self.modify():
            part_tag = self.addHull(rot=(0.5, 0.5, 0.5, 0.5), scale=(2, 4, 8))

            self.assertEqual(self._m.GetRotation(part_tag), (0.5, 0.5, 0.5, 0.5))
            self.assertEqual(self._m.GetScale(part_tag), (2, 4, 8))

    def test_getTransformUnknownPartRaises(self):
        with self.modify():
            with self.assertRaises(KeyError):
                self._m.GetPosition(5000)
            with self.assertRaises(KeyError):
                self._m.GetRotation(5000)
            with self.assertRaises(KeyError):
                self._m.GetScale(5000)

    def test_addHullEmptyFactionRaceUsesDefaults(self):
        with self.modify():
            part_tag = self.addHull(faction='', race='')

        self.assertNotEqual(part_tag, trinity.GetInvalidPartTag())
        self.assertEqual(self.instancedMeshes().GetMeshCount(), 1)

    def test_boundingSphereScalesWithSetTransform(self):
        with self.modify():
            part_tag = self.addHull()

        self.assertEqual(self.ship.boundingSphereRadius, 50)

        with self.modify():
            self._m.SetTransform(part_tag, (0, 0, 0), IDENTITY, (2, 2, 2))

        self.assertBounds(100, (0, 0, 0))

    def test_shapeEllipsoidEmptyAfterRemovingAllParts(self):
        with self.modify():
            part_tag = self.addHull(pos=(30, 0, 0))

        self.assertEqual(self.ship.shapeEllipsoidCenter, (30, 0, 0))

        with self.modify():
            self._m.Remove(part_tag)

        self.assertEqual(self.ship.shapeEllipsoidCenter, (0, 0, 0))

    def test_getTransformReflectsSetTransform(self):
        with self.modify():
            part_tag = self.addHull()
            self._m.SetTransform(part_tag, (1, 2, 3), (0.5, 0.5, 0.5, 0.5), (2, 4, 8))

            self.assertEqual(self._m.GetPosition(part_tag), (1, 2, 3))
            self.assertEqual(self._m.GetRotation(part_tag), (0.5, 0.5, 0.5, 0.5))
            self.assertEqual(self._m.GetScale(part_tag), (2, 4, 8))

    def test_partDataSurvivesModifierReopen(self):
        with self.modify():
            part_tag = self.addHull(pos=(10, 20, 30))

        with self.modify():
            self.assertEqual(self._m.GetPosition(part_tag), (10, 20, 30))

    def test_doubleRemoveRaises(self):
        with self.modify():
            part_tag = self.addHull()
            self._m.Remove(part_tag)

            with self.assertRaises(KeyError):
                self._m.Remove(part_tag)

    def test_removeInvalidPartTagRaises(self):
        with self.modify():
            self.addHull()

            with self.assertRaises(KeyError):
                self._m.Remove(trinity.GetInvalidPartTag())

    def test_boundingSphereConcentricParts(self):
        with self.modify():
            self.addHull()
            self.addHull('static_hull2')

        self.assertBounds(60, (0, 0, 0))

    def test_removeMiddleInstanceKeepsOthers(self):
        with self.modify():
            self.addHull()
            middle = self.addHull(pos=(1, 0, 0))
            self.addHull(pos=(2, 0, 0))

        self.assertEqual(self.instanceCount(), 3)

        with self.modify():
            self._m.Remove(middle)

        self.assertEqual(self.instanceCount(), 2)

    def test_removeChildAddedViaAddChild(self):
        with self.modify():
            with TempRes():
                child = trinity.EveChildMesh()
                blue.resMan.SaveObject(child, 'res:/child.red')
                part_tag = self._m.AddChild('res:/child.red', (10, 20, 30), IDENTITY, UNIT)

            self.assertEqual(len(blue.FindInterface(self.ship, 'EveChildMesh')), 1)

            self._m.Remove(part_tag)
            self.assertEqual(len(blue.FindInterface(self.ship, 'EveChildMesh')), 0)

    def test_audioHullCreatesPlacementContainer(self):
        with self.modify():
            self.addHull('audio_hull')

        containers = blue.FindInterface(self.ship, 'EveChildContainer')
        self.assertEqual(len(containers), 1)
        self.assertEqual(containers[0].name, 'audio_hull')

    def test_staticHullWithoutEmittersHasNoContainer(self):
        with self.modify():
            self.addHull()

        self.assertEqual(len(blue.FindInterface(self.ship, 'EveChildContainer')), 0)

    def test_audioHullContainerCarriesPartTag(self):
        with self.modify():
            part_tag = self.addHull('audio_hull')

        containers = blue.FindInterface(self.ship, 'EveChildContainer')
        self.assertEqual(containers[0].partTag, part_tag)

    def test_removeAudioHullRemovesContainer(self):
        with self.modify():
            part_tag = self.addHull('audio_hull')

        self.assertEqual(len(blue.FindInterface(self.ship, 'EveChildContainer')), 1)

        with self.modify():
            self._m.Remove(part_tag)

        self.assertEqual(len(blue.FindInterface(self.ship, 'EveChildContainer')), 0)

    def test_removeFirstOfTwoDifferentHullsKeepsSecond(self):
        with self.modify():
            first = self.addHull()
            self.addHull('static_hull2', pos=(1, 0, 0))

        self.assertEqual(self.instancedMeshes().GetMeshCount(), 2)

        with self.modify():
            self._m.Remove(first)

        meshes = self.instancedMeshes()
        self.assertEqual(meshes.GetMeshCount(), 1)
        self.assertEqual(meshes.GetMeshInfo(0)[0], 'res:/mygeo2.cmf')
        self.assertEqual(self.instanceCount(), 1)

    def test_removeAndReaddIntoSharedMesh(self):
        with self.modify():
            first = self.addHull()
            self.addHull(pos=(1, 0, 0))
            self.addHull(pos=(2, 0, 0))

        with self.modify():
            self._m.Remove(first)

        with self.modify():
            readded = self.addHull(pos=(3, 0, 0))

        self.assertNotEqual(readded, trinity.GetInvalidPartTag())
        self.assertEqual(self.instancedMeshes().GetMeshCount(), 1)
        self.assertEqual(self.instanceCount(), 3)


class TestModifyExistingObject(unittest.TestCase):
    def setUp(self):
        self.sof = _CreateSof()

    def test_modifyObjectWithoutPartDataCreatesIt(self):
        ship = trinity.EveStation2()
        modifier = trinity.ModifyModularObject(ship, self.sof)
        part_tag = modifier.AddHull('static_hull', FACTION, RACE, (0, 0, 0), IDENTITY, UNIT)
        modifier = None

        self.assertNotEqual(part_tag, trinity.GetInvalidPartTag())
        self.assertEqual(len(blue.FindInterface(ship, 'EveChildPartData')), 1)
        self.assertEqual(len(blue.FindInterface(ship, 'EveChildInstancedMeshes')), 1)

    def test_modifyObjectWithoutPartDataSurvivesReopen(self):
        ship = trinity.EveStation2()
        modifier = trinity.ModifyModularObject(ship, self.sof)
        part_tag = modifier.AddHull('static_hull', FACTION, RACE, (1, 2, 3), IDENTITY, UNIT)
        modifier = None

        modifier = trinity.ModifyModularObject(ship, self.sof)
        self.assertEqual(modifier.GetPosition(part_tag), (1, 2, 3))
        modifier = None

        self.assertEqual(len(blue.FindInterface(ship, 'EveChildPartData')), 1)
