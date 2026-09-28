# Copyright (c) 2023 CCP ehf.

class Platform(object):
    DX11 = 2
    DX12 = 6


PLATFORM_NAMES = {Platform.DX11: 'dx11', Platform.DX12: 'dx12'}


class ShaderModel(object):
    LO = 3
    HI = 4
    DEPTH = 5


SHADER_MODEL_NAMES = {ShaderModel.LO: 'lo', ShaderModel.HI: 'hi', ShaderModel.DEPTH: 'depth'}
