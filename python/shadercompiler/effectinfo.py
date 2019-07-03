from . import paths, PLATFORM_NAMES, SHADER_MODEL_NAMES, ShaderModel, Platform
import struct


class Stages(object):
    VERTEX_SHADER = 0
    PIXEL_SHADER = 1
    COMPUTE_SHADER = 2
    GEOMETRY_SHADER = 3
    HULL_SHADER = 4
    DOMAIN_SHADER = 5
    COUNT = 6


class _StructStream(object):
    def __init__(self, data):
        self._data = data
        self._offset = 0

    def read(self, fmt):
        result = struct.unpack_from(fmt, self._data, self._offset)
        self._offset += struct.calcsize(fmt)
        return result

    def read_uint8(self):
        return self.read('B')[0]

    def read_uint16(self):
        return self.read('H')[0]

    def read_int32(self):
        return self.read('i')[0]

    def read_uint32(self):
        return self.read('I')[0]

    def read_float(self):
        return self.read('f')[0]

    def read_raw(self, size):
        result = self._data[self._offset:self._offset + size]
        self._offset += size
        return result

    def seek(self, offset):
        self._offset = offset

    def offset(self):
        return self._offset

    def size(self):
        return len(self._data)

    def remaining(self):
        return len(self._data) - self._offset

    def get_data(self, offset, length):
        return self._data[offset:offset + length]


class _StringTable(object):
    def __init__(self, stream):
        table_size = stream.read_uint32()
        self._data = stream.read_raw(table_size)

    def get_string(self, offset):
        try:
            return self._data[offset:self._data.index('\000', offset)]
        except ValueError:
            return self._data[offset:]

    def get_blob(self, offset, size):
        return self._data[offset:offset + size]


class ShaderInput(object):
    def __init__(self, stream):
        self.usage = stream.read_uint8()
        self.register_index = stream.read_uint8()
        self.usage_index = stream.read_uint8()
        self.used_mask = stream.read_uint8()


class ShaderRegister(object):
    def __init__(self, stream):
        self.register_type = stream.read_uint8()
        self.register_index = stream.read_uint32()


_TRINITY_FLOAT_PARAMETERS = {1: 'Tr2FloatParameter', 2: 'Tr2Vector2Parameter', 3: 'Tr2Vector3Parameter',
                             4: 'Tr2Vector4Parameter', 16: 'Tr2Matrix4Parameter'}


class Constant(object):
    def __init__(self, stream, string_table):
        self.name = string_table.get_string(stream.read_uint32())
        self.offset = stream.read_uint32()
        self.size = stream.read_uint32()
        self.type = stream.read_uint8()
        self.dimension = stream.read_uint8()
        self.elements = stream.read_uint32()
        self.is_srgb = stream.read_uint8() != 0
        self.is_autoregister = stream.read_uint8() != 0
        self.default_value = None
        self.trinity_type = None
        if self.type == 0:  # float
            if self.elements > 1:
                self.trinity_type = 'TriFloatArrayParameter'
            else:
                if self.dimension == 16 and 'Reflection' in self.name:
                    self.trinity_type = 'TriVariableParameter'
                else:
                    self.trinity_type = _TRINITY_FLOAT_PARAMETERS.get(self.dimension)

    def read_default_value(self, data):
        if self.type == 0:
            value = []
            if self.elements > 1:
                for i in range(self.elements):
                    value.append(struct.unpack_from('f', data, self.offset + 4 * i)[0])
                self.default_value = value
            else:
                for i in range(self.dimension):
                    try:
                        value.append(struct.unpack_from('f', data, self.offset + 4 * i)[0])
                    except struct.error:
                        value.append(0.0)
                if self.dimension == 16:
                    self.default_value = tuple(value[0:4]), tuple(value[4:9]), tuple(value[8:13]), tuple(value[12:16])
                elif self.dimension == 1:
                    self.default_value = value[0]
                else:
                    self.default_value = tuple(value)


class Resource(object):
    def __init__(self, stream, string_table):
        self.register_index = stream.read_uint8()
        self.name = string_table.get_string(stream.read_uint32())
        self.type = stream.read_uint8()
        self.is_srgb = stream.read_uint8() != 0
        self.is_autoregister = stream.read_uint8() != 0
        if self.type <= 5:
            self.trinity_type = 'TriTextureParameter'
        else:
            self.trinity_type = 'Tr2GeometryBufferParameter'


class UAV(object):
    def __init__(self, stream, string_table):
        self.register_index = stream.read_uint8()
        self.name = string_table.get_string(stream.read_uint32())
        self.type = stream.read_uint8()
        self.is_srgb = False
        self.is_autoregister = stream.read_uint8() != 0
        if self.type <= 5:
            self.trinity_type = 'TriTextureParameter'
        else:
            self.trinity_type = 'Tr2GeometryBufferParameter'


class Sampler(object):
    def __init__(self, stream, string_table, version):
        self.register_index = stream.read_uint8()
        if version >= 4:
            self.name = string_table.get_string(stream.read_uint32())
        else:
            self.name = ''
        self.is_comparison = stream.read_uint8() != 0
        self.min_filter = stream.read_uint8()
        self.max_filter = stream.read_uint8()
        self.mip_filter = stream.read_uint8()
        self.address_u = stream.read_uint8()
        self.address_v = stream.read_uint8()
        self.address_w = stream.read_uint8()
        self.mip_lod_bias = stream.read_float()
        self.max_anisotropy = stream.read_uint8()
        self.comparison_func = stream.read_uint8()
        self.border_color = stream.read_float(), stream.read_float(), stream.read_float(), stream.read_float()
        self.min_lod = stream.read_float()
        self.max_lod = stream.read_float()
        if version < 4:
            stream.read_uint8()


class Stage(object):
    def __init__(self, stream, string_table, version):
        self.stage = stream.read_uint8()

        self.inputs = []
        for input_index in xrange(stream.read_uint8()):
            self.inputs.append(ShaderInput(stream))

        self.registers = []
        if version > 8:
            for input_index in xrange(stream.read_uint8()):
                self.registers.append(ShaderRegister(stream))

        if version < 5:
            stream.seek(stream.read_uint32() + stream.offset())
            stream.seek(stream.read_uint32() + stream.offset())
        else:
            stream.read_uint32()
            stream.read_uint32()
            stream.read_uint32()
            stream.read_uint32()

        if version >= 3:
            self.thread_group_size = stream.read_uint32(), stream.read_uint32(), stream.read_uint32()
        else:
            self.thread_group_size = None, None, None

        self.constants = []
        for i in xrange(stream.read_uint32()):
            self.constants.append(Constant(stream, string_table))

        constant_value_size = stream.read_uint32()
        if version < 5:
            constant_value_offset = stream.offset()
            stream.seek(stream.offset() + constant_value_size)
            const_data = stream.get_data(constant_value_offset, constant_value_size)
        else:
            constant_value_offset = stream.read_uint32()
            const_data = string_table.get_blob(constant_value_offset, constant_value_size)
        for const in self.constants:
            const.read_default_value(const_data)

        self.resources = []
        for i in xrange(stream.read_uint8()):
            self.resources.append(Resource(stream, string_table))

        self.samplers = []
        for i in xrange(stream.read_uint8()):
            self.samplers.append(Sampler(stream, string_table, version))

        self.uavs = []
        if version >= 3:
            for i in xrange(stream.read_uint8()):
                self.uavs.append(UAV(stream, string_table))

        self.annotations = {}
        if version >= 8:
            for j in xrange(stream.read_uint8()):
                annotation = Annotation(stream, string_table)
                self.annotations[annotation.name] = annotation


class Pass(object):
    def __init__(self, stream, string_table, version):
        self.stages = {}
        """:type: dict[str, Stage]"""

        stage_count = stream.read_uint8()
        if stage_count > Stages.COUNT:
            raise RuntimeError('too many stages')

        for stage_index in xrange(stage_count):
            stage = Stage(stream, string_table, version)
            self.stages[stage.stage] = stage

        self.states = {}
        state_count = stream.read_uint8()
        for i in xrange(state_count):
            state = stream.read_uint32()
            value = stream.read_uint32()
            self.states[state] = value


class Technique(object):
    def __init__(self, stream, string_table, version):
        self.passes = []
        """:type: list[Pass]"""
        self.name = 'Main'
        if version > 6:
            self.name = string_table.get_string(stream.read_uint32())
        pass_count = stream.read_uint8()
        for i in xrange(pass_count):
            self.passes.append(Pass(stream, string_table, version))


class AnnotationType(object):
    BOOL = 0
    INT = 1
    FLOAT = 2
    STRING = 3


class Annotation(object):
    def __init__(self, stream, string_table):
        self.name = string_table.get_string(stream.read_uint32())
        self.type = stream.read_uint8()
        if self.type == AnnotationType.BOOL:
            self.value = stream.read_uint32() != 0
        elif self.type == AnnotationType.INT:
            self.value = stream.read_int32()
        elif self.type == AnnotationType.FLOAT:
            self.value = stream.read_float()
        elif self.type == AnnotationType.STRING:
            self.value = string_table.get_string(stream.read_uint32())
        else:
            raise RuntimeError('invalid annotation type')


class ParameterAnnotation(object):
    def __init__(self, stream, string_table):
        self.name = string_table.get_string(stream.read_uint32())
        self.annotations = {}
        for j in xrange(stream.read_uint8()):
            annotation = Annotation(stream, string_table)
            self.annotations[annotation.name] = annotation

    def __getitem__(self, item):
        return self.annotations[item].value

    def get(self, name, default):
        if name in self.annotations:
            return self.annotations[name].value
        return default


class _Parameter(object):
    def __init__(self, constant, annotation):
        self.constant = constant
        self.annotation = annotation
        self.name = constant.name
        self.trinity_type = constant.trinity_type

    def __getitem__(self, item):
        return self.annotation[item]

    def get_annotation(self, name, default=None):
        return self.annotation.annotations.get(name, default)

    def update(self, other):
        if isinstance(self.constant, Resource):
            if self.constant.type == 5: # typeless texture
                self.constant = other.constant
        for k, v in other.annotation.annotations.iteritems():
            if k not in self.annotation.annotations:
                self.annotation.annotations[k] = v


def _merge_parameters(destination, other):
    for k, v in other.iteritems():
        if k not in destination:
            destination[k] = v
        else:
            destination[k].update(v)


class Permutation(object):
    STATIC = 0
    DYNAMIC = 1

    def __init__(self, stream, string_table, version):
        self.name = string_table.get_string(stream.read_uint32())
        self.default_index = stream.read_uint8()
        self.description = string_table.get_string(stream.read_uint32())
        if version > 5:
            self.type = stream.read_uint8()
        else:
            self.type = Permutation.STATIC
        count = stream.read_uint8()
        self.options = []
        for i in xrange(count):
            self.options.append(string_table.get_string(stream.read_uint32()))


class ShaderInfo(object):
    def __init__(self, stream, string_table, version, options):
        self.techniques = []
        """:type: list[Technique]"""

        if version > 6:
            technique_count = stream.read_uint8()
            for i in xrange(technique_count):
                self.techniques.append(Technique(stream, string_table, version))
        else:
            self.techniques.append(Technique(stream, string_table, version))

        self.annotations = {}
        for i in xrange(stream.read_uint16()):
            annotation = ParameterAnnotation(stream, string_table)
            self.annotations[annotation.name] = annotation

        self.parameters = self._extract_parameters('constants')
        self.resources = self._extract_parameters('resources')
        self.uavs = self._extract_parameters('uavs')
        self.resources.update(self.uavs)
        self.samplers = self._extract_samplers()
        self.options = options

    def _extract_parameters(self, stage_attr):
        result = {}
        for t in self.techniques:
            for p in t.passes:
                for stage in p.stages.itervalues():
                    for const in getattr(stage, stage_attr):
                        annotation = self.annotations.get(const.name)
                        if not annotation:
                            continue
                        try:
                            if not annotation['SasUiVisible']:
                                continue
                        except KeyError:
                            continue
                        result.setdefault(const.name, _Parameter(const, annotation))
        return result

    def _extract_samplers(self):
        result = {}
        for t in self.techniques:
            for p in t.passes:
                for stage in p.stages.itervalues():
                    for sampler in stage.samplers:
                        result[sampler.name] = sampler
        return result


class EffectInfo(object):
    def __init__(self, path, platform=Platform.DX11, shader_model=ShaderModel.DEPTH):
        self._stream = None
        self._version = None
        self.permutations = []

        if path.lower().endswith('.fx'):
            self._load(paths.get_compiled_path(path, shader_model, platform))
        else:
            self._load(path)

    def _load(self, path):
        with open(path, 'rb') as f:
            data = f.read()
        self._stream = _StructStream(data)
        stream = self._stream
        version = stream.read_uint32()
        self._version = version
        if version < 2 or version > 9:
            raise RuntimeError('unsupported effect file version')
        if version < 5:
            header_size = stream.read_uint32()
            if header_size == 0:
                raise RuntimeError('effect file contains no compiled effects')
            if header_size * 3 * 4 + 4 > stream.remaining():
                raise RuntimeError('invalid header size')
            self._offsets = {}
            for i in xrange(header_size):
                key = stream.read_uint32()
                self._offsets[key] = (stream.read_uint32(), stream.read_uint32())
            self._string_table = _StringTable(stream)
        else:
            self._string_table = _StringTable(stream)
            permutation_count = stream.read_uint8()
            self.permutations = []
            for i in xrange(permutation_count):
                self.permutations.append(Permutation(stream, self._string_table, version))
            header_size = stream.read_uint32()
            if header_size == 0:
                raise RuntimeError('effect file contains no compiled effects')
            if header_size * 3 * 4 + 4 > stream.remaining():
                raise RuntimeError('invalid header size')
            self._offsets = {}
            for i in xrange(header_size):
                key = stream.read_uint32()
                self._offsets[key] = (stream.read_uint32(), stream.read_uint32())

    def get_shader(self, permutation=0):
        """
        Return particular permutation information
        :param permutation: Either a permutation number (Incarna shaders) or a dict of permutation options
        :type permutation: int|dict
        :rtype: ShaderInfo
        """
        if isinstance(permutation, int):
            self._stream.seek(self._offsets[permutation][0])
        else:
            multiplier = 1
            index = 0
            for each in self.permutations:
                value = each.default_index
                if each.name in permutation:
                    value = each.options.index(permutation[each.name])
                index += value * multiplier
                multiplier *= len(each.options)

            self._stream.seek(self._offsets[index][0])
        return ShaderInfo(self._stream, self._string_table, self._version, self.index_to_options(permutation))

    def index_to_options(self, index):
        options = []
        for each in self.permutations:
            options.append((each, each.options[index % len(each.options)]))
            index /= len(each.options)
        return options


def apply_to_shaders(path, callback):
    """
    Applies a callback to all shader permutations

    :param path: effect path
    :type path: basestring
    :param callback: callback function that is called for each shader permutation
    :type callback: (ShaderInfo)->None
    :return:
    """
    for platform in PLATFORM_NAMES.iterkeys():
        for sm in SHADER_MODEL_NAMES.iterkeys():
            try:
                compiled = paths.get_compiled_path(path, sm, platform)
            except ValueError:
                continue
            try:
                effect = EffectInfo(compiled)
            except IOError:
                continue
            count = 1
            for each in effect.permutations:
                count *= len(each.options)
            for each in xrange(count):
                callback(effect.get_shader(each))


def get_merged_parameters(path, shader_filter=None):
    """
    Returns all referenced parameters and resources from all effect permutations

    :param path: effect path
    :type path: basestring
    :param shader_filter: optional filter for permutations, called with platform, shader model
    :type shader_filter: (int, int, list[(str, Permutation)])->bool
    :return: a tuple of parameters and resources dicts
    :rtype: (dict[str, _Parameter], dict[str, _Parameter], dict[str, Sampler])
    """
    parameters = {}
    resources = {}
    samplers = {}
    has_compiled = False
    for platform in PLATFORM_NAMES.iterkeys():
        for sm in SHADER_MODEL_NAMES.iterkeys():
            try:
                compiled = paths.get_compiled_path(path, sm, platform)
            except ValueError:
                continue
            try:
                effect = EffectInfo(compiled)
            except IOError:
                continue
            has_compiled = True
            count = 1
            for each in effect.permutations:
                count *= len(each.options)
            for each in xrange(count):
                if shader_filter:
                    if not shader_filter(platform, sm, effect.index_to_options(each)):
                        continue
                shader = effect.get_shader(each)
                _merge_parameters(parameters, shader.parameters)
                _merge_parameters(resources, shader.resources)
                samplers.update(shader.samplers)
    if not has_compiled:
        raise IOError('could not find any compiled effect for %s' % path)
    return parameters, resources, samplers
