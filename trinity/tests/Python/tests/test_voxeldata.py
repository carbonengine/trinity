import unittest

import blue
import trinity


class TestVoxelData(unittest.TestCase):
    def test_canCreateVoxelDataWithNoPayload(self):
        data = trinity.Tr2VoxelData(2, 2, 2, [])
        self.assertEqual(data.payloadChannelCount, 0)

    def test_canCreateVoxelDataWithPayload(self):
        data = trinity.Tr2VoxelData(2, 2, 2, [trinity.PIXEL_FORMAT.R8_UINT, trinity.PIXEL_FORMAT.R16G16_FLOAT])
        self.assertEqual(data.payloadChannelCount, 2)

    def test_cannotCreateWithUnreadablePixelFormat(self):
        with self.assertRaises(ValueError):
            trinity.Tr2VoxelData(2, 2, 2, [trinity.PIXEL_FORMAT.BC1_UNORM])
        with self.assertRaises(ValueError):
            trinity.Tr2VoxelData(2, 2, 2, [trinity.PIXEL_FORMAT.D32_FLOAT])

    def test_canPopulateDataNoPayload(self):
        data = trinity.Tr2VoxelData(2, 2, 2, [])
        data.PopulateSparse([(0, 0, 0), (1, 0, 1)])
        self.assertTrue(data.IsOccupied(0, 0, 0))
        self.assertFalse(data.IsOccupied(1, 0, 0))
        self.assertTrue(data.IsOccupied(1, 0, 1))

    def test_canPopulateDataWithPayload(self):
        data = trinity.Tr2VoxelData(2, 2, 2, [trinity.PIXEL_FORMAT.R8_UINT, trinity.PIXEL_FORMAT.R16G16_FLOAT])
        data.PopulateSparse([(0, 0, 0, 7, (0.5, 0.2)), (1, 0, 1, 8, (0.3, 0.7))])
        self.assertTrue(data.IsOccupied(0, 0, 0))
        self.assertFalse(data.IsOccupied(1, 0, 0))
        self.assertTrue(data.IsOccupied(1, 0, 1))

    def test_canPopulateOutsideBoundsFails(self):
        data = trinity.Tr2VoxelData(2, 2, 2, [])
        with self.assertRaises(ValueError):
            data.PopulateSparse([(0, 0, 0), (3, 0, 1)])

    def test_unoccupiedOutsideBounds(self):
        data = trinity.Tr2VoxelData(2, 2, 2, [])
        self.assertFalse(data.IsOccupied(-1, 0, 0))
        self.assertFalse(data.IsOccupied(0, -1, 0))
        self.assertFalse(data.IsOccupied(0, 0, -1))
        self.assertFalse(data.IsOccupied(2, 0, 0))
        self.assertFalse(data.IsOccupied(0, 2, 0))
        self.assertFalse(data.IsOccupied(0, 0, 2))

    def test_canAddVoxel(self):
        data = trinity.Tr2VoxelData(2, 2, 2, [])
        data.PopulateSparse([(1, 1, 1)])
        data.SetVoxels([(1, 0, 0)], [])
        self.assertTrue(data.IsOccupied(1, 1, 1))
        self.assertTrue(data.IsOccupied(1, 0, 0))
        self.assertFalse(data.IsOccupied(0, 1, 1))

    def test_canAddVoxelWithPayload(self):
        data = trinity.Tr2VoxelData(2, 2, 2, [trinity.PIXEL_FORMAT.R8_UINT, trinity.PIXEL_FORMAT.R16G16_FLOAT])
        data.PopulateSparse([(1, 1, 1, 7, (0.5, 0.2))])
        data.SetVoxels([(1, 0, 0, 3, (0.3, 0.7))], [])
        self.assertTrue(data.IsOccupied(1, 1, 1))
        self.assertTrue(data.IsOccupied(1, 0, 0))
        self.assertFalse(data.IsOccupied(0, 1, 1))

    def test_canChangeVoxel(self):
        data = trinity.Tr2VoxelData(2, 2, 2, [trinity.PIXEL_FORMAT.R8_UINT, trinity.PIXEL_FORMAT.R16G16_FLOAT])
        data.PopulateSparse([(1, 1, 1, 7, (0.5, 0.2))])
        data.SetVoxels([(1, 0, 0, 3, (0.3, 0.7)), (1, 1, 1, 8, (0.6, 0.4))], [])
        self.assertTrue(data.IsOccupied(1, 1, 1))
        self.assertTrue(data.IsOccupied(1, 0, 0))
        self.assertFalse(data.IsOccupied(0, 1, 1))

    def test_canRemoveVoxel(self):
        data = trinity.Tr2VoxelData(2, 2, 2, [])
        data.PopulateSparse([(1, 1, 1)])
        self.assertTrue(data.IsOccupied(1, 1, 1))
        data.SetVoxels([], [(1, 1, 1)])
        self.assertFalse(data.IsOccupied(1, 1, 1))

    def test_canAddAndRemoveVoxel(self):
        data = trinity.Tr2VoxelData(2, 2, 2, [])
        data.PopulateSparse([(1, 1, 1)])
        data.SetVoxels([(1, 0, 0)], [(1, 0, 0)])
        self.assertFalse(data.IsOccupied(1, 0, 0))
