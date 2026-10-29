# Copyright © 2026 CCP ehf.

import math
import unittest

import trinity


def _LinearCurve():
    # currentValue == time over [0, 10]
    curve = trinity.Tr2CurveScalar()
    curve.AddKey(0.0, 0.0, trinity.Tr2CurveInterpolation.LINEAR)
    curve.AddKey(10.0, 10.0, trinity.Tr2CurveInterpolation.LINEAR)
    return curve


def _LinearVectorCurve():
    # currentValue == (t, 2t, 3t) over [0, 10]
    curve = trinity.Tr2CurveVector3()
    curve.AddKey(0.0, (0.0, 0.0, 0.0), trinity.Tr2CurveInterpolation.LINEAR)
    curve.AddKey(10.0, (10.0, 20.0, 30.0), trinity.Tr2CurveInterpolation.LINEAR)
    return curve


def _LinearColorCurve():
    # currentValue == (t, 2t, 3t, 4t) / 10 over [0, 10]
    curve = trinity.Tr2CurveColor()
    curve.AddKey(0.0, (0.0, 0.0, 0.0, 0.0), trinity.Tr2CurveInterpolation.LINEAR)
    curve.AddKey(10.0, (1.0, 2.0, 3.0, 4.0), trinity.Tr2CurveInterpolation.LINEAR)
    return curve


def _Bind(curveSet, source, sourceAttribute, timeOffset=0.0, destination=None):
    if destination is None:
        destination = trinity.Tr2FloatParameter()
    binding = trinity.TriValueBinding()
    binding.sourceObject = source
    binding.sourceAttribute = sourceAttribute
    binding.destinationObject = destination
    binding.destinationAttribute = 'value'
    binding.timeOffset = timeOffset
    curveSet.bindings.append(binding)
    return destination


def _ApplyAt(curveSet, time):
    curveSet.scaledTime = time
    curveSet.Apply()


class TestCurveSetBindingTimeOffset(unittest.TestCase):
    def setUp(self):
        self.curve = _LinearCurve()
        self.curveSet = trinity.TriCurveSet()
        self.curveSet.curves.append(self.curve)

    def test_zero_offset_copies_curve_set_time(self):
        destination = _Bind(self.curveSet, self.curve, 'currentValue')
        _ApplyAt(self.curveSet, 5.0)
        self.assertAlmostEqual(destination.value, 5.0, places=4)

    def test_offset_samples_curve_at_shifted_time(self):
        lagging = _Bind(self.curveSet, self.curve, 'currentValue', timeOffset=2.0)
        leading = _Bind(self.curveSet, self.curve, 'currentValue', timeOffset=-1.5)
        _ApplyAt(self.curveSet, 5.0)
        self.assertAlmostEqual(lagging.value, 3.0, places=4)
        self.assertAlmostEqual(leading.value, 6.5, places=4)

    def test_offset_binding_leaves_shared_curve_alone(self):
        _Bind(self.curveSet, self.curve, 'currentValue', timeOffset=2.0)
        plain = _Bind(self.curveSet, self.curve, 'currentValue')
        _ApplyAt(self.curveSet, 5.0)
        self.assertAlmostEqual(self.curve.currentValue, 5.0, places=4)
        self.assertAlmostEqual(plain.value, 5.0, places=4)

    def test_offset_binding_leaves_curve_of_another_curve_set_alone(self):
        owner = trinity.TriCurveSet()
        curve = _LinearCurve()
        owner.curves.append(curve)
        _ApplyAt(owner, 8.0)

        destination = _Bind(self.curveSet, curve, 'currentValue', timeOffset=2.0)
        _ApplyAt(self.curveSet, 5.0)

        self.assertAlmostEqual(destination.value, 3.0, places=4)
        self.assertAlmostEqual(curve.currentValue, 8.0, places=4)

    def test_offset_samples_vector_curve(self):
        curve = _LinearVectorCurve()
        self.curveSet.curves.append(curve)
        destination = _Bind(self.curveSet, curve, 'currentValue', timeOffset=2.0,
                            destination=trinity.Tr2Vector3Parameter())
        _ApplyAt(self.curveSet, 5.0)
        for actual, expected in zip(destination.value, (3.0, 6.0, 9.0)):
            self.assertAlmostEqual(actual, expected, places=4)

    def test_offset_samples_one_component_of_vector_curve(self):
        curve = _LinearVectorCurve()
        self.curveSet.curves.append(curve)
        y = _Bind(self.curveSet, curve, 'currentValue.y', timeOffset=2.0)
        z = _Bind(self.curveSet, curve, 'currentValue.z', timeOffset=2.0)
        _ApplyAt(self.curveSet, 5.0)
        self.assertAlmostEqual(y.value, 6.0, places=4)
        self.assertAlmostEqual(z.value, 9.0, places=4)

    def test_offset_samples_color_curve(self):
        curve = _LinearColorCurve()
        self.curveSet.curves.append(curve)
        destination = _Bind(self.curveSet, curve, 'currentValue', timeOffset=-5.0,
                            destination=trinity.Tr2Vector4Parameter())
        _ApplyAt(self.curveSet, 2.0)
        for actual, expected in zip(destination.value, (0.7, 1.4, 2.1, 2.8)):
            self.assertAlmostEqual(actual, expected, places=4)

    def test_offset_samples_quaternion_curve(self):
        # 0 to 90 degrees about z over 10 seconds: halfway is 45 degrees,
        # whether the curve slerps or normalises a lerp.
        curve = trinity.Tr2CurveQuaternion()
        curve.AddKey(0.0, (0.0, 0.0, 0.0, 1.0))
        curve.AddKey(10.0, (0.0, 0.0, math.sin(math.pi / 4.0), math.cos(math.pi / 4.0)))
        self.curveSet.curves.append(curve)
        destination = _Bind(self.curveSet, curve, 'currentValue', timeOffset=2.0,
                            destination=trinity.Tr2Vector4Parameter())
        _ApplyAt(self.curveSet, 7.0)
        expected = (0.0, 0.0, math.sin(math.pi / 8.0), math.cos(math.pi / 8.0))
        for actual, wanted in zip(destination.value, expected):
            self.assertAlmostEqual(actual, wanted, places=4)

    def test_offset_samples_euler_rotation_curve(self):
        curve = trinity.Tr2CurveEulerRotation()
        curve.AddKey(0.0, (0.0, 0.0, 0.0), trinity.Tr2CurveInterpolation.LINEAR)
        curve.AddKey(10.0, (1.0, 0.5, 0.25), trinity.Tr2CurveInterpolation.LINEAR)
        self.curveSet.curves.append(curve)
        destination = _Bind(self.curveSet, curve, 'currentValue', timeOffset=2.0,
                            destination=trinity.Tr2Vector4Parameter())
        _ApplyAt(self.curveSet, 7.0)
        sampled = tuple(destination.value)
        # The curve's own value two seconds earlier
        _ApplyAt(self.curveSet, 5.0)
        for actual, expected in zip(sampled, curve.currentValue):
            self.assertAlmostEqual(actual, expected, places=4)

    def test_offset_on_long_vector_curve_finds_each_channels_key_segment(self):
        curve = trinity.Tr2CurveVector3()
        for key in range(64):
            curve.AddKey(float(key), (float(key), float(key * 2), float(key * 3)),
                         trinity.Tr2CurveInterpolation.LINEAR)
        curveSet = trinity.TriCurveSet()
        curveSet.curves.append(curve)
        now = _Bind(curveSet, curve, 'currentValue', destination=trinity.Tr2Vector3Parameter())
        earlier = _Bind(curveSet, curve, 'currentValue', timeOffset=20.0,
                        destination=trinity.Tr2Vector3Parameter())
        for step in range(0, 400):
            time = 20.0 + step * 0.1
            _ApplyAt(curveSet, time)
            for scale, actual in zip((1.0, 2.0, 3.0), now.value):
                self.assertAlmostEqual(actual, time * scale, places=3)
            for scale, actual in zip((1.0, 2.0, 3.0), earlier.value):
                self.assertAlmostEqual(actual, (time - 20.0) * scale, places=3)

    def test_offset_on_long_curve_finds_the_right_key_segment(self):
        # Sampling one curve at two times far apart keeps missing its cached key
        # segment, so this goes through the curve's key search each time.
        curve = trinity.Tr2CurveScalar()
        for key in range(64):
            curve.AddKey(float(key), float(key * 2), trinity.Tr2CurveInterpolation.LINEAR)
        curveSet = trinity.TriCurveSet()
        curveSet.curves.append(curve)
        now = _Bind(curveSet, curve, 'currentValue')
        earlier = _Bind(curveSet, curve, 'currentValue', timeOffset=20.0)
        for step in range(0, 400):
            time = 20.0 + step * 0.1
            _ApplyAt(curveSet, time)
            self.assertAlmostEqual(now.value, time * 2.0, places=3)
            self.assertAlmostEqual(earlier.value, (time - 20.0) * 2.0, places=3)

    def test_offset_wraps_into_looped_time_range(self):
        destination = _Bind(self.curveSet, self.curve, 'currentValue', timeOffset=2.0)
        self.curveSet.SetTimeRange(0.0, 4.0, True)
        _ApplyAt(self.curveSet, 1.0)
        # 1 - 2 = -1 wraps to 3 inside [0, 4)
        self.assertAlmostEqual(destination.value, 3.0, places=4)

    def test_offset_without_range_is_not_wrapped(self):
        destination = _Bind(self.curveSet, self.curve, 'currentValue', timeOffset=2.0)
        _ApplyAt(self.curveSet, 7.0)
        self.assertAlmostEqual(destination.value, 5.0, places=4)

    def test_offset_on_non_curve_source_copies_value_as_is(self):
        source = trinity.Tr2FloatParameter()
        source.value = 42.0
        destination = _Bind(self.curveSet, source, 'value', timeOffset=2.0)
        _ApplyAt(self.curveSet, 5.0)
        self.assertAlmostEqual(destination.value, 42.0, places=4)

    def test_offset_on_curve_attribute_other_than_current_value_is_ignored(self):
        self.curve.timeOffset = 0.25
        destination = _Bind(self.curveSet, self.curve, 'timeOffset', timeOffset=2.0)
        _ApplyAt(self.curveSet, 5.0)
        self.assertAlmostEqual(destination.value, 0.25, places=4)


if __name__ == '__main__':
    unittest.main()
