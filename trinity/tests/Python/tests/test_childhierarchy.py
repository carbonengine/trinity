# Copyright © 2026 CCP ehf.

import shutil
import tempfile

import contextlib
import blue
import unittest

import trinity


@contextlib.contextmanager
def TempRes():
    tempdir = tempfile.mkdtemp()
    blue.paths.SetSearchPath('res', '%s;%s' % (tempdir, blue.paths.GetSearchPath('res')))
    try:
        yield
    finally:
        shutil.rmtree(tempdir)


class TestChildHierarchy(unittest.TestCase):
    def test_initValues(self):
        obj = trinity.EveChildMesh()
        self.assertIsNone(obj.GetOwner())
        self.assertIsNone(obj.GetParent())
        self.assertEqual(obj.partTag, 0)

    def test_ownerAssignment(self):
        ship = trinity.EveShip2()
        obj = trinity.EveChildMesh()
        ship.effectChildren.append(obj)
        self.assertEqual(obj.GetOwner(), ship)

    def test_transitiveOwnerAssignment(self):
        ship = trinity.EveShip2()
        container = trinity.EveChildContainer()
        mesh = trinity.EveChildMesh()
        ship.effectChildren.append(container)
        self.assertEqual(container.GetOwner(), ship)
        container.objects.append(mesh)
        self.assertEqual(mesh.GetOwner(), ship)

    def test_ownerPropagation(self):
        ship = trinity.EveShip2()
        container = trinity.EveChildContainer()
        mesh = trinity.EveChildMesh()
        container.objects.append(mesh)
        ship.effectChildren.append(container)
        self.assertEqual(mesh.GetOwner(), ship)

    def test_parentAssignment(self):
        container = trinity.EveChildContainer()
        mesh = trinity.EveChildMesh()
        container.objects.append(mesh)
        self.assertEqual(mesh.GetParent(), container)

    def test_ownerRemoval(self):
        ship = trinity.EveShip2()
        container = trinity.EveChildContainer()
        mesh = trinity.EveChildMesh()
        ship.effectChildren.append(container)
        container.objects.append(mesh)

        ship.effectChildren.remove(container)
        self.assertIsNone(container.GetOwner())
        self.assertIsNone(mesh.GetOwner())

    def test_grandChildOwnerRemoval(self):
        ship = trinity.EveShip2()
        container = trinity.EveChildContainer()
        mesh = trinity.EveChildMesh()
        ship.effectChildren.append(container)
        container.objects.append(mesh)

        container.objects.remove(mesh)
        self.assertIsNone(mesh.GetOwner())

    def test_parentRemoval(self):
        container = trinity.EveChildContainer()
        mesh = trinity.EveChildMesh()
        container.objects.append(mesh)
        container.objects.remove(mesh)
        self.assertIsNone(mesh.GetParent())

    def test_parentClear(self):
        container = trinity.EveChildContainer()
        mesh = trinity.EveChildMesh()
        container.objects.append(mesh)
        container.objects.removeAt(-1)
        self.assertIsNone(mesh.GetParent())

    def test_loadedHierarchy(self):
        ship = trinity.EveShip2()
        container = trinity.EveChildContainer()
        mesh = trinity.EveChildMesh()
        container.objects.append(mesh)
        ship.effectChildren.append(container)

        with TempRes():
            blue.resMan.SaveObject(ship, 'res:/test.red')

            ship2 = blue.resMan.LoadObject('res:/test.red')
            container2 = ship2.effectChildren[0]
            mesh2 = container2.objects[0]
            self.assertEqual(mesh2.GetOwner(), ship2)
            self.assertEqual(mesh2.GetParent(), container2)
            self.assertEqual(container2.GetOwner(), ship2)

