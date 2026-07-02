# Copyright (c) 2023 CCP ehf.

import shutil
import tempfile
import unittest2

import shadercompiler.compiler as compiler
from shadercompiler.effectinfo import EffectInfo


MINIMAL_SHADER = """
float4 vs(): POSITION
{
    return 0;
}

float4 ps(): COLOR
{
    return 0;
}

technique t
{
    pass p
    {
        VertexShader = compile vs_3_0 vs();
        PixelShader = compile ps_3_0 ps();
    }
}
"""

SHADER_WITH_PARAMETER = """
float4 param
<
    bool SasUiVisible = true;
    string MyAnnotation = "blah-blah";
>;

float4 vs(): POSITION
{
    return param;
}

float4 ps(): COLOR
{
    return 0;
}

technique t
{
    pass p
    {
        VertexShader = compile vs_3_0 vs();
        PixelShader = compile ps_3_0 ps();
    }
}
"""


class TestCompiler(unittest2.TestCase):
    tempDir = None

    @classmethod
    def setUpClass(cls):
        cls.tempDir = tempfile.mkdtemp()

    @classmethod
    def tearDownClass(cls):
        shutil.rmtree(cls.tempDir)

    def _compile_and_parse(self, source, *args, **kwargs):
        src_path = tempfile.mktemp(dir=TestCompiler.tempDir)
        dest_path = tempfile.mktemp(dir=TestCompiler.tempDir)
        with open(src_path, 'w') as f:
            f.write(source)
        compiler.compile_shader(src_path, dest_path, *args, **kwargs)
        return EffectInfo(dest_path).get_shader()

    def test_compiling_empty_shader_raises(self):
        with self.assertRaises(compiler.CompilerError):
            self._compile_and_parse("", defines={'PLATFORM': 2})

    def test_compiling_invalid_shader_raises(self):
        with self.assertRaises(compiler.CompilerError):
            self._compile_and_parse("blah", defines={'PLATFORM': 2})

    def test_can_compile_shader(self):
        info = self._compile_and_parse(MINIMAL_SHADER, defines={'PLATFORM': 2})
        self.assertEqual(len(info.techniques), 1)
        self.assertEqual(len(info.techniques[0].passes), 1)

    def test_can_access_parameters(self):
        info = self._compile_and_parse(SHADER_WITH_PARAMETER, defines={'PLATFORM': 2})
        self.assertEqual(info.parameters['param']['MyAnnotation'], 'blah-blah')
        self.assertEqual(info.parameters['param'].trinity_type, 'Tr2Vector4Parameter')
