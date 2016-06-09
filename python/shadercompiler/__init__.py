
class Platform(object):
    DX9 = 0
    DX11 = 1
    GLES2 = 3
    ORBIS = 4
    GL4 = 6

PLATFORM_NAMES = {Platform.DX9: 'dx9', Platform.DX11: 'dx11', Platform.GLES2: 'gles2', Platform.ORBIS: 'orbis',
                 Platform.GL4: 'gl4'}


class ShaderModel(object):
    LO = 3
    HI = 4
    DEPTH = 5

SHADER_MODEL_NAMES = {ShaderModel.LO: 'lo', ShaderModel.HI: 'hi', ShaderModel.DEPTH: 'depth'}