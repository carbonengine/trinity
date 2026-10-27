import unittest

import blue
import trinity

from ._util import TempRes


class TestVoxelData(unittest.TestCase):
    def assertAlmostEqual(self, first, second, places = None, msg = None, delta = None):
        if isinstance(first, tuple) and isinstance(second, tuple):
            for f, s in zip(first, second):
                super().assertAlmostEqual(f, s, places, msg, delta)
            return
        return super().assertAlmostEqual(first, second, places, msg, delta)

    def test_cannotCreateVoxelDataWithNoPayload(self):
        with self.assertRaises(ValueError):
            data = trinity.Tr2VoxelData((2, 2, 2), [])

    def test_canCreateVoxelDataWithPayload(self):
        data = trinity.Tr2VoxelData((2, 2, 2), [{"type": trinity.Tr2VoxelDataType.UInt8, "elementCount": 1, "normalized": False}, {"type": trinity.Tr2VoxelDataType.Float16, "elementCount": 2, "normalized": False}])
        self.assertEqual(data.payloadChannelCount, 2)

    def test_cannotCreateWithInvalidTypes(self):
        with self.assertRaises(ValueError):
            trinity.Tr2VoxelData((2, 2, 2), [{"type": trinity.Tr2VoxelDataType.UInt8, "elementCount": 5, "normalized": False}])
        with self.assertRaises(ValueError):
            trinity.Tr2VoxelData((2, 2, 2), [{"type": trinity.Tr2VoxelDataType.Boolean, "elementCount": 1, "normalized": True}])

    def test_unpopulatedDataIsZeroInitialized(self):
        data = trinity.Tr2VoxelData((2, 2, 2), [{"type": trinity.Tr2VoxelDataType.UInt8, "elementCount": 1, "normalized": False}, {"type": trinity.Tr2VoxelDataType.Float16, "elementCount": 2, "normalized": False}])
        self.assertEqual(data.GetPayloadData(0, (0, 0, 0)), 0)
        self.assertEqual(data.GetPayloadData(1, (0, 0, 0)), (0.0, 0.0))

    def test_canPopulateData(self):
        data = trinity.Tr2VoxelData((2, 2, 2), [{"type": trinity.Tr2VoxelDataType.UInt8, "elementCount": 1, "normalized": False}, {"type": trinity.Tr2VoxelDataType.Float16, "elementCount": 2, "normalized": False}])
        data.SetVoxels([((0, 0, 0), 7, (0.5, 0.2)), ((1, 0, 1), 8, (0.3, 0.7))])
        self.assertEqual(data.GetPayloadData(0, (0, 0, 0)), 7)
        
        self.assertAlmostEqual(data.GetPayloadData(1, (0, 0, 0)), (0.5, 0.2), places=2)
        self.assertEqual(data.GetPayloadData(0, (1, 0, 1)), 8)
        self.assertAlmostEqual(data.GetPayloadData(1, (1, 0, 1)), (0.3, 0.7), places=2)
        self.assertEqual(data.GetPayloadData(0, (1, 0, 0)), 0)
        self.assertEqual(data.GetPayloadData(1, (1, 0, 0)), (0.0, 0.0))

    def test_canUseBooleans(self):
        data = trinity.Tr2VoxelData((2, 2, 2), [{"type": trinity.Tr2VoxelDataType.Boolean, "elementCount": 1, "normalized": False}])
        data.SetVoxels([((0, 0, 0), False), ((1, 0, 1), True)])
        self.assertEqual(data.GetPayloadData(0, (0, 0, 0)), False)
        self.assertEqual(data.GetPayloadData(0, (1, 0, 1)), True)
        self.assertEqual(data.GetPayloadData(0, (1, 0, 0)), False)

    def test_canUseBooleans2(self):
        data = trinity.Tr2VoxelData((2, 2, 2), [{"type": trinity.Tr2VoxelDataType.Boolean, "elementCount": 2, "normalized": False}])
        data.SetVoxels([((0, 0, 0), (False, True)), ((1, 0, 1), (True, False))])
        self.assertEqual(data.GetPayloadData(0, (0, 0, 0)), (False, True))
        self.assertEqual(data.GetPayloadData(0, (1, 0, 1)), (True, False))
        self.assertEqual(data.GetPayloadData(0, (1, 0, 0)), (False, False))

    def test_canUseNormalizedInts(self):
        data = trinity.Tr2VoxelData((2, 2, 2), [{"type": trinity.Tr2VoxelDataType.UInt8, "elementCount": 1, "normalized": True}])
        data.SetVoxels([((0, 0, 0), 0.5), ((1, 0, 1), 1.0)])
        self.assertAlmostEqual(data.GetPayloadData(0, (0, 0, 0)), 0.5, places=2)
        self.assertAlmostEqual(data.GetPayloadData(0, (1, 0, 1)), 1.0, places=2)
        self.assertAlmostEqual(data.GetPayloadData(0, (1, 0, 0)), 0.0, places=2)

    def test_populateOutsideBoundsFails(self):
        data = trinity.Tr2VoxelData((2, 2, 2), [{"type": trinity.Tr2VoxelDataType.UInt8, "elementCount": 1, "normalized": False}])
        with self.assertRaises(ValueError):
            data.SetVoxels([((0, 0, 0), 1), ((3, 0, 1), 2)])

    def test_canAddVoxel(self):
        data = trinity.Tr2VoxelData((2, 2, 2), [{"type": trinity.Tr2VoxelDataType.UInt8, "elementCount": 1, "normalized": False}, {"type": trinity.Tr2VoxelDataType.Float16, "elementCount": 2, "normalized": False}])
        data.SetVoxels([((1, 1, 1), 7, (0.5, 0.2))])
        data.SetVoxels([((1, 0, 0), 3, (0.3, 0.7))])
        self.assertEqual(data.GetPayloadData(0, (1, 1, 1)), 7)
        self.assertEqual(data.GetPayloadData(0, (1, 0, 0)), 3)
        self.assertEqual(data.GetPayloadData(0, (0, 0, 0)), 0)

    def test_canChangeVoxel(self):
        data = trinity.Tr2VoxelData((2, 2, 2), [{"type": trinity.Tr2VoxelDataType.UInt8, "elementCount": 1, "normalized": False}, {"type": trinity.Tr2VoxelDataType.Float16, "elementCount": 2, "normalized": False}])
        data.SetVoxels([((1, 1, 1), 7, (0.5, 0.2))])
        data.SetVoxels([((1, 0, 0), 3, (0.3, 0.7)), ((1, 1, 1), 8, (0.6, 0.4))])
        self.assertEqual(data.GetPayloadData(0, (1, 1, 1)), 8)

    def test_canPersistData(self):
        with TempRes():
            data = trinity.Tr2VoxelData((2, 2, 2), [{"type": trinity.Tr2VoxelDataType.UInt8, "elementCount": 1, "normalized": False}, {"type": trinity.Tr2VoxelDataType.Float16, "elementCount": 2, "normalized": False}])
            data.SetVoxels([((1, 1, 1), 7, (0.5, 0.2))])

            self.assertTrue(blue.resMan.SaveObject(data, "res:/test_voxeldata.red"))

            data1 = blue.resMan.LoadObject("res:/test_voxeldata.red")
            self.assertEqual(data.payloadChannelCount, data1.payloadChannelCount)
            self.assertEqual(data.width, data1.width)
            self.assertEqual(data.height, data1.height)
            self.assertEqual(data.depth, data1.depth)
            self.assertEqual(data.GetPayloadData(0, (1, 1, 1)), 7)
            self.assertAlmostEqual(data.GetPayloadData(1, (1, 1, 1)), (0.5, 0.2), places=2)

    def test_asyncAccess(self):
        data = trinity.Tr2VoxelData((2, 2, 2), [{"type": trinity.Tr2VoxelDataType.UInt8, "elementCount": 1, "normalized": False}])
        data.SetVoxels([((1, 1, 1), 7)])
        accessor = trinity.Tr2VoxelDataTestAccessor(data, 0)
        # The accessor should have a snapshot of the data at the time of creation, so changing the data afterwards should not affect the accessor.
        self.assertEqual(accessor.GetData((1, 1, 1)), 7)
        data.SetVoxels([((1, 1, 1), 8)])
        self.assertEqual(accessor.GetData((1, 1, 1)), 7)
        accessor = trinity.Tr2VoxelDataTestAccessor(data, 0)
        self.assertEqual(accessor.GetData((1, 1, 1)), 8)
