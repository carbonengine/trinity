import os
import re


PLATFORMS = 'dx9', 'dx11', 'gles2'
SHADER_MODELS = 'sm_lo', 'sm_hi', 'sm_depth'


def get_compiled_path(effect_path, shader_model='sm_depth', platform='dx11'):
    if not effect_path.lower().endswith('.fx'):
        raise ValueError('invalid effect extension')
    components = re.split(r'([/\\])', effect_path)
    for i in xrange(len(components) - 1, -1, -1):
        if components[i].lower() == 'effect':
            components[i] += '.%s' % platform
            break
    else:
        raise ValueError('malformed effect path')
    components[-1] = '%s.%s' % (os.path.splitext(components[-1])[0], shader_model)
    return ''.join(components)
