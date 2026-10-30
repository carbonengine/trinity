import json
import math
import unittest

import blue
import trinity

from ._util import TempRes


class TestVoxelGeometry(unittest.TestCase):
    def test_noDataRaisesError(self):
        geometry = trinity.Tr2VoxelGeometry()
        with TempRes():
            with self.assertRaises(RuntimeError):
                geometry.TestBuildFile("res:/voxels.cmf")

    def test_canCreateSimpleGeometry(self):
        data = trinity.Tr2VoxelData((2, 2, 2), [{"type": trinity.Tr2VoxelDataType.Float, "elementCount": 1, "normalized": False}])
        data.SetVoxels([((0, 0, 0), 0.0), ((1, 0, 0), 1.0), ((1, 1, 1), math.sqrt(3.0))])
        geometry = trinity.Tr2VoxelGeometry()
        geometry.voxelData = data
        with TempRes():
            geometry.TestBuildFile( "res:/voxels.cmf")
            granny = trinity.TriGrannyRes()
            self.assertTrue(granny.LoadForTools("res:/voxels.cmf"))
            self.assertEqual(granny.GetMeshCount(), 1)
            self.assertEqual(granny.GetMeshAreaCount(0), 1)

    def test_canPersistGeometry(self):
        data = trinity.Tr2VoxelData((2, 2, 2), [{"type": trinity.Tr2VoxelDataType.Float, "elementCount": 1, "normalized": False}])
        data.SetVoxels([((0, 0, 0), 0.0), ((1, 0, 0), 1.0), ((1, 1, 1), math.sqrt(3.0))])
        geometry = trinity.Tr2VoxelGeometry(data)
        with TempRes():
            geometry.TestBuildFile( "res:/voxels1.cmf")
            self.assertTrue(blue.resMan.SaveObject(geometry, "res:/voxels.red"))
            geometry1 = blue.resMan.LoadObject("res:/voxels.red")
            geometry1.TestBuildFile( "res:/voxels2.cmf")
            # assert files are the same
            f1 = blue.paths.GetFileContentsWithYield("res:/voxels1.cmf").read()
            f2 = blue.paths.GetFileContentsWithYield("res:/voxels2.cmf").read()
            self.assertEqual(f1, f2)

    def test_canCreateSurfaceNetsGeometry(self):
        data = trinity.Tr2VoxelData((2, 2, 2), [{"type": trinity.Tr2VoxelDataType.Float, "elementCount": 1, "normalized": False}])
        data.SetVoxels([((0, 0, 0), 0.0), ((1, 0, 0), 1.0), ((1, 1, 1), math.sqrt(3.0))])
        geometry = trinity.Tr2VoxelGeometry()
        geometry.voxelData = data
        geometry.method = trinity.Tr2VoxelGeometryMethod.SurfaceNets
        geometry.subdivision = 1
        geometry.relaxIterations = 0
        with TempRes():
            geometry.TestBuildFile( "res:/voxels.cmf")
            granny = trinity.TriGrannyRes()
            self.assertTrue(granny.LoadForTools("res:/voxels.cmf"))
            self.assertEqual(granny.GetMeshCount(), 1)
            self.assertEqual(granny.GetMeshAreaCount(0), 1)
