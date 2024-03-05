import os
import re
from . import Platform, ShaderModel, PLATFORM_NAMES, SHADER_MODEL_NAMES


def get_compiled_path(effect_path, shader_model=ShaderModel.DEPTH, platform=Platform.DX11):
    if not effect_path.lower().endswith('.fx'):
        raise ValueError('invalid effect extension')
    components = re.split(r'([/\\])', effect_path)
    for i in xrange(len(components) - 1, -1, -1):
        if components[i].lower() == 'effect':
            components[i] += '.%s' % PLATFORM_NAMES[platform]
            break
    else:
        raise ValueError('malformed effect path')
    components[-1] = '%s.sm_%s' % (os.path.splitext(components[-1])[0], SHADER_MODEL_NAMES[shader_model])
    return ''.join(components)
