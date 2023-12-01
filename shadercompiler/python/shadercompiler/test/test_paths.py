import unittest2

import shadercompiler.paths as paths


class TestPaths(unittest2.TestCase):
    def test_get_compiled_path_for_empty_raises(self):
        with self.assertRaises(ValueError):
            paths.get_compiled_path('')

    def test_get_compiled_path_for_not_effect_raises(self):
        with self.assertRaises(ValueError):
            paths.get_compiled_path('res:/test.txt')

    def test_get_compiled_path_for_invalid_location_raises(self):
        with self.assertRaises(ValueError):
            paths.get_compiled_path('res:/test.fx')

    def test_get_compiled_path_changes_extension(self):
        self.assertFalse(paths.get_compiled_path('res:/effect/test.fx').endswith('.fx'))

    def test_get_compiled_path_changes_effect_folder(self):
        self.assertFalse('/effect/' in paths.get_compiled_path('res:/effect/test.fx'))

