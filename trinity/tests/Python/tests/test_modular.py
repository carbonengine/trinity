# Copyright © 2026 CCP ehf.

import unittest
import blue
import trinity


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

    faction = trinity.EveSOFDataFaction()
    faction.name = 'testfaction'
    data.faction.append(faction)
    race = trinity.EveSOFDataRace()
    race.name = 'testrace'
    data.race.append(race)
    sof = trinity.EveSOF()
    sof.dataMgr.SetData(data)
    return sof

_time = 1

def _UpdateTransfroms(ship):
    global _time
    scene = trinity.EveSpaceScene()
    scene.objects.append(ship)
    scene.UpdateScene(_time)
    _time += 1


class TestModular(unittest.TestCase):
    def test_createEmptyShip(self):
        sof = _CreateSof()
        ship, modifier = trinity.CreateModularObject(sof, "testfaction", "restrace")
        del modifier
        self.assertTrue(ship)
        self.assertTrue(len(ship.effectChildren) == 1)
        self.assertTrue(isinstance(ship.effectChildren[0], trinity.EveChildPartData))

    def test_addStaticHull(self):
        sof = _CreateSof()
        ship, modifier = trinity.CreateModularObject(sof, "testfaction", "restrace")
        modifier.AddHull('static_hull', 'testfaction', 'testrace', (0, 0, 0), (0, 0, 0, 1), (1,1,1))
        del modifier

        # Static meshes are added as instanced
        instancedMeshes = blue.FindInterface(ship, 'EveChildInstancedMeshes')[0]
        self.assertTrue(instancedMeshes.GetMeshCount() == 1)

    def test_addAnimatedHull(self):
        sof = _CreateSof()
        ship, modifier = trinity.CreateModularObject(sof, "testfaction", "restrace")
        modifier.AddHull('anim_hull', 'testfaction', 'testrace', (0, 0, 0), (0, 0, 0, 1), (1,1,1))
        del modifier

        self.assertTrue(len(blue.FindInterface(ship, 'EveChildInstancedMeshes')) == 0)
        self.assertTrue(len(blue.FindInterface(ship, 'EveChildMesh')) == 1)

    def test_addTwoStaticHulls(self):
        sof = _CreateSof()
        ship, modifier = trinity.CreateModularObject(sof, "testfaction", "restrace")
        modifier.AddHull('static_hull', 'testfaction', 'testrace', (0, 0, 0), (0, 0, 0, 1), (1,1,1))
        modifier.AddHull('static_hull', 'testfaction', 'testrace', (1, 2, 3), (0, 0, 0, 1), (1,1,1))
        del modifier

        # Static meshes are added as instanced
        instancedMeshes = blue.FindInterface(ship, 'EveChildInstancedMeshes')[0]
        self.assertEqual(instancedMeshes.GetMeshCount(), 1)
        info = instancedMeshes.GetMeshInfo(0)
        instances = info[6]
        self.assertEqual(instances, 2)

    def test_addTwoDifferentStaticHulls(self):
        sof = _CreateSof()
        ship, modifier = trinity.CreateModularObject(sof, "testfaction", "restrace")
        modifier.AddHull('static_hull', 'testfaction', 'testrace', (0, 0, 0), (0, 0, 0, 1), (1,1,1))
        modifier.AddHull('static_hull2', 'testfaction', 'testrace', (1, 2, 3), (0, 0, 0, 1), (1,1,1))
        del modifier

        # Static meshes are added as instanced
        instancedMeshes = blue.FindInterface(ship, 'EveChildInstancedMeshes')[0]
        self.assertEqual(instancedMeshes.GetMeshCount(), 2)
        self.assertEqual(instancedMeshes.GetMeshInfo(0)[6], 1)
        self.assertEqual(instancedMeshes.GetMeshInfo(1)[6], 1)

    def test_removeAnimatedHull(self):
        sof = _CreateSof()
        ship, modifier = trinity.CreateModularObject(sof, "testfaction", "restrace")
        part = modifier.AddHull('anim_hull', 'testfaction', 'testrace', (0, 0, 0), (0, 0, 0, 1), (1,1,1))
        del modifier

        self.assertTrue(len(blue.FindInterface(ship, 'EveChildInstancedMeshes')) == 0)
        self.assertTrue(len(blue.FindInterface(ship, 'EveChildMesh')) == 1)

        modifier = trinity.ModifyModularObject(ship, sof)
        modifier.Remove(part)
        del modifier

    def test_removeStaticHull(self):
        sof = _CreateSof()
        ship, modifier = trinity.CreateModularObject(sof, "testfaction", "restrace")
        part = modifier.AddHull('static_hull', 'testfaction', 'testrace', (0, 0, 0), (0, 0, 0, 1), (1,1,1))
        del modifier

        # Static meshes are added as instanced
        instancedMeshes = blue.FindInterface(ship, 'EveChildInstancedMeshes')[0]
        self.assertTrue(instancedMeshes.GetMeshCount() == 1)

        modifier = trinity.ModifyModularObject(ship, sof)
        modifier.Remove(part)
        del modifier

        instancedMeshes = blue.FindInterface(ship, 'EveChildInstancedMeshes')[0]
        self.assertTrue(instancedMeshes.GetMeshCount() == 0)

    def test_removeAnimatedHull(self):
        sof = _CreateSof()
        ship, modifier = trinity.CreateModularObject(sof, "testfaction", "testrace")
        part = modifier.AddHull('anim_hull', 'testfaction', 'testrace', (0, 0, 0), (0, 0, 0, 1), (1,1,1))
        del modifier

        self.assertEqual(len(blue.FindInterface(ship, 'EveChildMesh')), 1)

        modifier = trinity.ModifyModularObject(ship, sof)
        modifier.Remove(part)
        del modifier

        self.assertEqual(len(blue.FindInterface(ship, 'EveChildMesh')), 0)

    def test_setBoundingSphere(self):
        sof = _CreateSof()
        ship, modifier = trinity.CreateModularObject(sof, "testfaction", "restrace")
        modifier.AddHull('static_hull', 'testfaction', 'testrace', (0, 0, 0), (0, 0, 0, 1), (1,1,1))
        del modifier

        self.assertEqual(ship.boundingSphereRadius, 50)

    def test_boundingSphereAccountsForTransforms(self):
        sof = _CreateSof()
        ship, modifier = trinity.CreateModularObject(sof, "testfaction", "restrace")
        modifier.AddHull('static_hull', 'testfaction', 'testrace', (30, 0, 0), (0, 0, 0, 1), (1, 1, 1))
        del modifier

        self.assertEqual(ship.boundingSphereRadius, 50.0)
        self.assertEqual(ship.boundingSphereCenter, (30, 0, 0))

    def test_boundingSphereEncapsulatesAllChilren(self):
        sof = _CreateSof()
        ship, modifier = trinity.CreateModularObject(sof, "testfaction", "restrace")
        modifier.AddHull('static_hull', 'testfaction', 'testrace', (30, 0, 0), (0, 0, 0, 1), (1, 1, 1))
        modifier.AddHull('static_hull', 'testfaction', 'testrace', (-30, 0, 0), (0, 0, 0, 1), (1, 1, 1))
        del modifier

        self.assertEqual(ship.boundingSphereRadius, 30.0 + 50.0)
        self.assertEqual(ship.boundingSphereCenter, (0, 0, 0))

    def test_boundingSphereUpdatesOnRemoval(self):
        sof = _CreateSof()
        ship, modifier = trinity.CreateModularObject(sof, "testfaction", "restrace")
        part = modifier.AddHull('static_hull', 'testfaction', 'testrace', (30, 0, 0), (0, 0, 0, 1), (1, 1, 1))
        modifier.AddHull('static_hull', 'testfaction', 'testrace', (-30, 0, 0), (0, 0, 0, 1), (1, 1, 1))
        del modifier

        modifier = trinity.ModifyModularObject(ship, sof)
        modifier.Remove(part)
        del modifier

        self.assertEqual(ship.boundingSphereRadius, 50.0)
        self.assertEqual(ship.boundingSphereCenter, (-30, 0, 0))

    def test_addsLocators(self):
        sof = _CreateSof()
        ship, modifier = trinity.CreateModularObject(sof, "testfaction", "restrace")
        modifier.AddHull('static_hull', 'testfaction', 'testrace', (30, 0, 0), (0, 0, 0, 1), (1, 1, 1))
        del modifier

        self.assertEqual(len(ship.locatorSets), 1)
        self.assertEqual(ship.locatorSets[0].name, 'damage')
        self.assertEqual(len(ship.locatorSets[0].locators), 1)

    def test_locatorsInheritTransform(self):
        sof = _CreateSof()
        ship, modifier = trinity.CreateModularObject(sof, "testfaction", "restrace")
        modifier.AddHull('static_hull', 'testfaction', 'testrace', (30, 0, 0), (0, 0, 0, 1), (1, 1, 1))
        del modifier

        self.assertEqual(ship.locatorSets[0].locators[0][0], (30, 0, 0))

    def test_addsLocatorsMerged(self):
        sof = _CreateSof()
        ship, modifier = trinity.CreateModularObject(sof, "testfaction", "restrace")
        modifier.AddHull('static_hull', 'testfaction', 'testrace', (30, 0, 0), (0, 0, 0, 1), (1, 1, 1))
        modifier.AddHull('static_hull', 'testfaction', 'testrace', (-30, 0, 0), (0, 0, 0, 1), (1, 1, 1))
        del modifier

        self.assertEqual(len(ship.locatorSets), 1)
        self.assertEqual(ship.locatorSets[0].name, 'damage')
        self.assertEqual(len(ship.locatorSets[0].locators), 2)

    def test_addsLocatorsRemoved(self):
        sof = _CreateSof()
        ship, modifier = trinity.CreateModularObject(sof, "testfaction", "restrace")
        part = modifier.AddHull('static_hull', 'testfaction', 'testrace', (30, 0, 0), (0, 0, 0, 1), (1, 1, 1))
        modifier.AddHull('static_hull', 'testfaction', 'testrace', (-30, 0, 0), (0, 0, 0, 1), (1, 1, 1))
        del modifier

        modifier = trinity.ModifyModularObject(ship, sof)
        modifier.Remove(part)
        del modifier

        self.assertEqual(len(ship.locatorSets), 1)
        self.assertEqual(ship.locatorSets[0].name, 'damage')
        self.assertEqual(len(ship.locatorSets[0].locators), 1)

    def test_moveAnimatedHull(self):
        sof = _CreateSof()
        ship, modifier = trinity.CreateModularObject(sof, "testfaction", "testrace")
        part = modifier.AddHull('anim_hull', 'testfaction', 'testrace', (10, 20, 0), (0, 0, 0, 1), (1,1,1))
        del modifier

        _UpdateTransfroms(ship)
        mesh = blue.FindInterface(ship, 'EveChildMesh')[0]
        self.assertEqual(mesh.worldTransform, ((1, 0, 0, 0), (0, 1, 0, 0), (0, 0, 1, 0), (10, 20, 0, 1)))

        modifier = trinity.ModifyModularObject(ship, sof)
        modifier.SetTransform(part, (1, 2, 3), (0, 0, 0, 1), (1, 1, 1))
        del modifier

        _UpdateTransfroms(ship)

        self.assertEqual(mesh.worldTransform, ((1, 0, 0, 0), (0, 1, 0, 0), (0, 0, 1, 0), (1, 2, 3, 1)))

    def test_moveLocators(self):
        sof = _CreateSof()
        ship, modifier = trinity.CreateModularObject(sof, "testfaction", "testrace")
        part = modifier.AddHull('static_hull', 'testfaction', 'testrace', (10, 20, 0), (0, 0, 0, 1), (1,1,1))
        del modifier

        self.assertEqual(ship.locatorSets[0].locators[0][0], (10, 20, 0))

        modifier = trinity.ModifyModularObject(ship, sof)
        modifier.SetTransform(part, (1, 2, 3), (0, 0, 0, 1), (1, 1, 1))
        del modifier

        self.assertEqual(ship.locatorSets[0].locators[0][0], (1, 2, 3))


# TODO: audio emitters