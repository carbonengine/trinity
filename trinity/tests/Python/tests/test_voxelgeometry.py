import contextlib
import json
import math
import unittest

import blue
import trinity

from ._util import TempRes


FLOAT = {"type": trinity.Tr2VoxelDataType.Float, "elementCount": 1, "normalized": False}
FLOAT2 = {"type": trinity.Tr2VoxelDataType.Float, "elementCount": 2, "normalized": False}
BOOL = {"type": trinity.Tr2VoxelDataType.Boolean, "elementCount": 1, "normalized": False}
INT8 = {"type": trinity.Tr2VoxelDataType.Int8, "elementCount": 1, "normalized": False}
INT8_NORM = {"type": trinity.Tr2VoxelDataType.Int8, "elementCount": 1, "normalized": True}
UINT8 = {"type": trinity.Tr2VoxelDataType.UInt8, "elementCount": 1, "normalized": False}

ALL_METHODS = (
    trinity.Tr2VoxelGeometryMethod.Simple,
    trinity.Tr2VoxelGeometryMethod.SurfaceNets,
    trinity.Tr2VoxelGeometryMethod.DualContouring,
)
DUAL_METHODS = (
    trinity.Tr2VoxelGeometryMethod.SurfaceNets,
    trinity.Tr2VoxelGeometryMethod.DualContouring,
)

# (usage, usageIndex) pairs as reported by TriGrannyRes.GetMeshVertexElements
POSITION = (0, 0)
NORMAL = (2, 0)
SHADING_NORMAL = (2, 1)
PACKED_TANGENT = (3, 0)
PACKED_SHADING_TANGENT = (3, 1)
EXTRA_DATA = (5, 0)

SPHERE_SIZE = 8
SPHERE_RADIUS = 2.5


def _sphereDistance(coord):
    center = (SPHERE_SIZE - 1) / 2.0
    return math.sqrt(sum((c - center) ** 2 for c in coord)) - SPHERE_RADIUS


def _sphereCoords():
    return [(x, y, z) for x in range(SPHERE_SIZE) for y in range(SPHERE_SIZE) for z in range(SPHERE_SIZE)]


def _sphereData(channels=(FLOAT,), valuesFn=lambda coord: (_sphereDistance(coord),)):
    """Creates voxel data with a sphere in the middle of the grid; valuesFn maps a coordinate to the per-channel values."""
    data = trinity.Tr2VoxelData((SPHERE_SIZE, SPHERE_SIZE, SPHERE_SIZE), list(channels))
    data.SetVoxels([(coord,) + tuple(valuesFn(coord)) for coord in _sphereCoords()])
    return data


@contextlib.contextmanager
def _case(**params):
    """Labels failures with the case parameters; used instead of subTest, whose failures the test runner does not report."""
    try:
        yield
    except Exception as e:
        raise AssertionError("%r: %s: %s" % (params, type(e).__name__, e)) from e


def _build(geometry, path="res:/voxels.cmf"):
    """Builds the geometry to a CMF file and returns (TriGrannyRes, file contents). Must be called within TempRes."""
    geometry.TestBuildFile(path)
    granny = trinity.TriGrannyRes()
    if not granny.LoadForTools(path):
        raise AssertionError("Failed to load %s" % path)
    return granny, blue.paths.GetFileContentsWithYield(path).read()


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

    def test_constructorSetsChannels(self):
        data = _sphereData((FLOAT, BOOL), lambda coord: (_sphereDistance(coord), _sphereDistance(coord) < 0))
        geometry = trinity.Tr2VoxelGeometry(data, 0, 1)
        self.assertEqual(geometry.voxelData, data)
        self.assertEqual(geometry.shapeChannelIndex, 0)
        self.assertEqual(geometry.occupancyChannelIndex, 1)

    def test_invalidShapeChannelIndexRaisesError(self):
        data = _sphereData()
        with self.assertRaises(ValueError):
            trinity.Tr2VoxelGeometry(data, 1)

    def test_unsupportedShapeChannelTypeRaisesError(self):
        for channel in (BOOL, INT8, UINT8, FLOAT2):
            with _case(channel=channel):
                data = trinity.Tr2VoxelData((2, 2, 2), [channel])
                with self.assertRaises(ValueError):
                    trinity.Tr2VoxelGeometry(data, 0)

    def test_invalidOccupancyChannelIndexRaisesError(self):
        data = _sphereData()
        with self.assertRaises(ValueError):
            trinity.Tr2VoxelGeometry(data, 0, 1)

    def test_unsupportedOccupancyChannelTypeRaisesError(self):
        for channel in (FLOAT, INT8_NORM, UINT8):
            with _case(channel=channel):
                data = trinity.Tr2VoxelData((2, 2, 2), [channel])
                with self.assertRaises(ValueError):
                    trinity.Tr2VoxelGeometry(data, -1, 0)

    def test_unsupportedShapeChannelSetAsPropertyBuildsNothing(self):
        data = _sphereData((FLOAT, UINT8), lambda coord: (_sphereDistance(coord), 1))
        geometry = trinity.Tr2VoxelGeometry(data, 0)
        geometry.shapeChannelIndex = 1
        with TempRes():
            with self.assertRaises(RuntimeError):
                geometry.TestBuildFile("res:/voxels.cmf")

    def test_noShapeOrOccupancyChannelBuildsNothing(self):
        data = _sphereData()
        geometry = trinity.Tr2VoxelGeometry(data, -1, -1)
        with TempRes():
            with self.assertRaises(RuntimeError):
                geometry.TestBuildFile("res:/voxels.cmf")

    def test_emptyVolumeBuildsNothing(self):
        data = _sphereData(valuesFn=lambda coord: (1.0,))
        for method in ALL_METHODS:
            with _case(method=method):
                geometry = trinity.Tr2VoxelGeometry(data, 0)
                geometry.method = method
                with TempRes():
                    with self.assertRaises(RuntimeError):
                        geometry.TestBuildFile("res:/voxels.cmf")

    def test_canCreateGeometryWithAllMethods(self):
        data = _sphereData()
        for method in ALL_METHODS:
            with _case(method=method):
                geometry = trinity.Tr2VoxelGeometry(data, 0)
                geometry.method = method
                with TempRes():
                    granny, _ = _build(geometry)
                    self.assertEqual(granny.GetMeshCount(), 1)
                    self.assertEqual(granny.GetMeshAreaCount(0), 1)
                    self.assertGreater(granny.GetMeshTriangleCount(0), 0)

    def test_canCreateGeometryFromNormalizedInt8Shape(self):
        data = _sphereData((INT8_NORM,), lambda coord: (max(-1.0, min(1.0, _sphereDistance(coord) / 4.0)),))
        for method in ALL_METHODS:
            with _case(method=method):
                geometry = trinity.Tr2VoxelGeometry(data, 0)
                geometry.method = method
                with TempRes():
                    granny, _ = _build(geometry)
                    self.assertGreater(granny.GetMeshTriangleCount(0), 0)

    def test_canCreateGeometryFromOccupancy(self):
        data = _sphereData((BOOL,), lambda coord: (_sphereDistance(coord) < 0,))
        for method in ALL_METHODS:
            with _case(method=method):
                geometry = trinity.Tr2VoxelGeometry(data, -1, 0)
                geometry.method = method
                with TempRes():
                    granny, _ = _build(geometry)
                    self.assertGreater(granny.GetMeshTriangleCount(0), 0)

    def test_vertexElementsFollowComponents(self):
        components = trinity.Tr2VoxelGeometryComponents
        cases = [
            (0, [POSITION]),
            (components.SmoothNormals, [POSITION, NORMAL]),
            (components.SplitNormals, [POSITION, SHADING_NORMAL]),
            (components.SmoothNormals | components.SplitNormals, [POSITION, NORMAL, SHADING_NORMAL]),
            (components.Curvature, [POSITION, EXTRA_DATA]),
            (components.SmoothedCurvature, [POSITION, EXTRA_DATA]),
            (components.DataTags, [POSITION, EXTRA_DATA]),
            (components.SmoothNormals | components.Curvature | components.DataTags, [POSITION, NORMAL, EXTRA_DATA]),
        ]
        data = _sphereData()
        for method in ALL_METHODS:
            for flags, expected in cases:
                with _case(method=method, components=flags):
                    geometry = trinity.Tr2VoxelGeometry(data, 0)
                    geometry.method = method
                    geometry.components = flags
                    with TempRes():
                        granny, _ = _build(geometry)
                        self.assertEqual(granny.GetMeshVertexElements(0), expected)

    def test_packedTangentsReplaceNormals(self):
        components = trinity.Tr2VoxelGeometryComponents
        data = _sphereData()
        for method in ALL_METHODS:
            with _case(method=method):
                geometry = trinity.Tr2VoxelGeometry(data, 0)
                geometry.method = method
                geometry.components = components.SmoothNormals | components.SplitNormals
                geometry.packedTangents = True
                with TempRes():
                    granny, _ = _build(geometry)
                    self.assertEqual(granny.GetMeshVertexElements(0), [POSITION, PACKED_TANGENT, PACKED_SHADING_TANGENT])

    def test_packedTangentsWithoutNormalsHasNoEffect(self):
        geometry = trinity.Tr2VoxelGeometry(_sphereData(), 0)
        with TempRes():
            _, unpacked = _build(geometry, "res:/voxels1.cmf")
            geometry.packedTangents = True
            _, packed = _build(geometry, "res:/voxels2.cmf")
        self.assertEqual(unpacked, packed)

    def test_subdivisionIncreasesTriangleCount(self):
        data = _sphereData()
        for method in DUAL_METHODS:
            with _case(method=method):
                geometry = trinity.Tr2VoxelGeometry(data, 0)
                geometry.method = method
                with TempRes():
                    coarse, _ = _build(geometry, "res:/voxels1.cmf")
                    geometry.subdivision = 2
                    fine, _ = _build(geometry, "res:/voxels2.cmf")
                    self.assertGreater(fine.GetMeshTriangleCount(0), coarse.GetMeshTriangleCount(0))

    def test_relaxIterationsMoveVertices(self):
        data = _sphereData()
        for method in DUAL_METHODS:
            with _case(method=method):
                geometry = trinity.Tr2VoxelGeometry(data, 0)
                geometry.method = method
                with TempRes():
                    original, originalContents = _build(geometry, "res:/voxels1.cmf")
                    geometry.relaxIterations = 3
                    relaxed, relaxedContents = _build(geometry, "res:/voxels2.cmf")
                    self.assertEqual(relaxed.GetMeshTriangleCount(0), original.GetMeshTriangleCount(0))
                    self.assertNotEqual(relaxedContents, originalContents)

    def test_shapeTransformOffsetChangesSurface(self):
        data = _sphereData()
        for method in ALL_METHODS:
            with _case(method=method):
                geometry = trinity.Tr2VoxelGeometry(data, 0)
                geometry.method = method
                with TempRes():
                    original, _ = _build(geometry, "res:/voxels1.cmf")
                    # A positive offset moves the zero crossing inwards, shrinking the sphere
                    geometry.shapeTransformOffset = 0.5
                    shrunk, _ = _build(geometry, "res:/voxels2.cmf")
                    self.assertLess(shrunk.GetMeshTriangleCount(0), original.GetMeshTriangleCount(0))

    def test_cutsChangeGeometry(self):
        data = _sphereData()
        for method in DUAL_METHODS:
            for name in ("facetDepth", "cleaveDepth"):
                with _case(method=method, cut=name):
                    geometry = trinity.Tr2VoxelGeometry(data, 0)
                    geometry.method = method
                    with TempRes():
                        _, uncut = _build(geometry, "res:/voxels1.cmf")
                        setattr(geometry, name, 1.0)
                        _, cut = _build(geometry, "res:/voxels2.cmf")
                        self.assertNotEqual(cut, uncut)

    def test_cutsAreDeterministic(self):
        data = _sphereData()
        for method in DUAL_METHODS:
            with _case(method=method):
                with TempRes():
                    contents = []
                    for i in range(2):
                        geometry = trinity.Tr2VoxelGeometry(data, 0)
                        geometry.method = method
                        geometry.facetDepth = 1.0
                        geometry.cleaveDepth = 1.0
                        geometry.cutSeed = 7
                        contents.append(_build(geometry, "res:/voxels%d.cmf" % i)[1])
                    self.assertEqual(contents[0], contents[1])

    def test_cutSeedChangesCuts(self):
        data = _sphereData()
        for method in DUAL_METHODS:
            with _case(method=method):
                geometry = trinity.Tr2VoxelGeometry(data, 0)
                geometry.method = method
                geometry.facetDepth = 1.0
                geometry.cleaveDepth = 1.0
                with TempRes():
                    _, seed0 = _build(geometry, "res:/voxels1.cmf")
                    geometry.cutSeed = 7
                    _, seed7 = _build(geometry, "res:/voxels2.cmf")
                    self.assertNotEqual(seed0, seed7)

    def test_dataTagsChangeOutput(self):
        components = trinity.Tr2VoxelGeometryComponents
        for tagChannel, tagValue in ((INT8_NORM, 0.5), (INT8, 3), (BOOL, True)):
            with _case(channel=tagChannel):
                data = _sphereData((FLOAT, tagChannel), lambda coord: (_sphereDistance(coord), tagValue))
                geometry = trinity.Tr2VoxelGeometry(data, 0)
                geometry.method = trinity.Tr2VoxelGeometryMethod.SurfaceNets
                geometry.components = components.DataTags
                with TempRes():
                    _, untagged = _build(geometry, "res:/voxels1.cmf")
                    geometry.dataTagChannelIndex = 1
                    granny, tagged = _build(geometry, "res:/voxels2.cmf")
                    self.assertEqual(granny.GetMeshVertexElements(0), [POSITION, EXTRA_DATA])
                    self.assertNotEqual(tagged, untagged)

    def test_unsupportedDataTagChannelStillBuilds(self):
        data = _sphereData((FLOAT, FLOAT), lambda coord: (_sphereDistance(coord), 1.0))
        geometry = trinity.Tr2VoxelGeometry(data, 0)
        geometry.method = trinity.Tr2VoxelGeometryMethod.SurfaceNets
        geometry.components = trinity.Tr2VoxelGeometryComponents.DataTags
        with TempRes():
            _, untagged = _build(geometry, "res:/voxels1.cmf")
            geometry.dataTagChannelIndex = 1
            _, tagged = _build(geometry, "res:/voxels2.cmf")
        self.assertEqual(tagged, untagged)
