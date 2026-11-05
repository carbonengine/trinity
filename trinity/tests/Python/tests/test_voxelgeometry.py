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
        geometry = trinity.Tr2VoxelGeometry(data, 0)
        geometry.shapeChannelIndex = 0
        with TempRes():
            geometry.TestBuildFile( "res:/voxels.cmf")
            granny = trinity.TriGrannyRes()
            self.assertTrue(granny.LoadForTools("res:/voxels.cmf"))
            self.assertEqual(granny.GetMeshCount(), 1)
            self.assertEqual(granny.GetMeshAreaCount(0), 1)

    def test_canPersistGeometry(self):
        data = trinity.Tr2VoxelData((2, 2, 2), [{"type": trinity.Tr2VoxelDataType.Float, "elementCount": 1, "normalized": False}])
        data.SetVoxels([((0, 0, 0), 0.0), ((1, 0, 0), 1.0), ((1, 1, 1), math.sqrt(3.0))])
        geometry = trinity.Tr2VoxelGeometry(data, 0)
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
        geometry = trinity.Tr2VoxelGeometry(data, 0)
        geometry.method = trinity.Tr2VoxelGeometryMethod.SurfaceNets
        geometry.subdivision = 1
        geometry.relaxIterations = 0
        with TempRes():
            geometry.TestBuildFile( "res:/voxels.cmf")
            granny = trinity.TriGrannyRes()
            self.assertTrue(granny.LoadForTools("res:/voxels.cmf"))
            self.assertEqual(granny.GetMeshCount(), 1)
            self.assertEqual(granny.GetMeshAreaCount(0), 1)

    def test_canCreateSurfaceNetsGeometryWithDataTags(self):
        data = trinity.Tr2VoxelData((2, 2, 2), [{"type": trinity.Tr2VoxelDataType.Float, "elementCount": 1, "normalized": False}, {"type": trinity.Tr2VoxelDataType.Int8, "elementCount": 1, "normalized": True}])
        data.SetVoxels([((0, 0, 0), 0.0, 0.4), ((1, 0, 0), 1.0, 5.0), ((1, 1, 1), math.sqrt(3.0), 0.7)])
        geometry = trinity.Tr2VoxelGeometry(data, 0)
        geometry.dataTagChannelIndex = 1
        geometry.method = trinity.Tr2VoxelGeometryMethod.SurfaceNets
        geometry.components = trinity.Tr2VoxelGeometryComponents.SmoothNormals | trinity.Tr2VoxelGeometryComponents.DataTags
        with TempRes():
            geometry.TestBuildFile( "res:/voxels.cmf")
            granny = trinity.TriGrannyRes()
            self.assertTrue(granny.LoadForTools("res:/voxels.cmf"))
            self.assertEqual(granny.GetMeshCount(), 1)
