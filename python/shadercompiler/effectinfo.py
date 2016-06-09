from . import paths, PLATFORM_NAMES, SHADER_MODEL_NAMES
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


class _StringTable(object):
    def __init__(self, stream):
        table_size = stream.read_uint32()
        self._data = stream.read_raw(table_size)

    def get_string(self, offset):
        try:
            return self._data[offset:self._data.index('\000', offset)]
        except ValueError:
            return self._data[offset:]


class ShaderInput(object):
    def __init__(self, stream):
        self.usage = stream.read_uint8()
        self.register_index = stream.read_uint8()
        self.usage_index = stream.read_uint8()
        self.used_mask = stream.read_uint8()


_TRINITY_FLOAT_PARAMETERS = {1: 'Tr2FloatParameter', 2: 'Tr2Vector2Parameter', 3: 'Tr2Vector3Parameter',
                             4: 'Tr2Vector4Parameter'}


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
        self.trinity_type = None
        if self.type == 0:  # float
            if self.elements > 1:
                self.trinity_type = 'TriFloatArrayParameter'
            else:
                try:
                    self.trinity_type = _TRINITY_FLOAT_PARAMETERS[self.dimension]
                except KeyError:
                    pass


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

        stream.seek(stream.read_uint32() + stream.offset())
        stream.seek(stream.read_uint32() + stream.offset())

        if version >= 3:
            self.thread_group_size = stream.read_uint32(), stream.read_uint32(), stream.read_uint32()
        else:
            self.thread_group_size = None, None, None

        self.constants = []
        for i in xrange(stream.read_uint32()):
            self.constants.append(Constant(stream, string_table))

        constant_value_size = stream.read_uint32()
        stream.seek(stream.offset() + constant_value_size)

        self.resources = []
        for i in xrange(stream.read_uint8()):
            self.resources.append(Resource(stream, string_table))

        self.samplers = []
        for i in xrange(stream.read_uint8()):
            self.samplers.append(Sampler(stream, string_table, version))

        self.uavs = []
        for i in xrange(stream.read_uint8()):
            self.uavs.append(UAV(stream, string_table))


class Pass(object):
    def __init__(self, stream, string_table, version):
        self.stages = {}

        stage_count = stream.read_uint8()
        if stage_count > Stages.COUNT:
            raise RuntimeError('too many stages')

        for stage_index in xrange(stage_count):
            stage = Stage(stream, string_table, version)
            self.stages[stage.stage] = stage

        self.states = {}
        state_count = stream.read_uint8()
        for i in xrange(state_count):
            state = stream.read_uint8()
            value = stream.read_uint8()
            self.states[state] = value


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


class _Parameter(object):
    def __init__(self, constant, annotation):
        self.constant = constant
        self.annotation = annotation
        self.name = constant.name
        self.trinity_type = constant.trinity_type

    def __getitem__(self, item):
        return self.annotation[item]

    def update(self, other):
        for k, v in other.annotation.annotations.iteritems():
            if k not in self.annotation.annotations:
                self.annotation.annotations[k] = v


def _merge_parameters(destination, other):
    for k, v in other.iteritems():
        if k not in destination:
            destination[k] = v
        else:
            destination[k].update(v)


class EffectInfo(object):
    def __init__(self, path, permutation=None):
        if path.lower().endswith('.fx'):
            first = True
            for platform in PLATFORM_NAMES.iterkeys():
                for sm in SHADER_MODEL_NAMES.iterkeys():
                    if first:
                        self._load(paths.get_compiled_path(path, sm, platform), permutation)
                        first = False
                    else:
                        try:
                            e = EffectInfo(paths.get_compiled_path(path, sm, platform), permutation)
                        except IOError:
                            continue
                        _merge_parameters(self.parameters, e.parameters)
                        _merge_parameters(self.resources, e.resources)
                        _merge_parameters(self.uavs, e.uavs)
        else:
            self._load(path, permutation)

    def _load(self, path, permutation):
        with open(path, 'rb') as f:
            data = f.read()
        stream = _StructStream(data)
        version = stream.read_uint32()
        if version < 2 or version > 4:
            raise RuntimeError('unsupported effect file version')
        header_size = stream.read_uint32()
        if header_size == 0:
            raise RuntimeError('effect file contains no compiled effects')
        if header_size * 3 * 4 + 4 > stream.remaining():
            raise RuntimeError('invalid header size')

        if permutation is None:
            stream.read_uint32()
            offset = stream.read_uint32()
        else:
            for i in xrange(header_size):
                p = stream.read_uint32()
                offset = stream.read_uint32()
                if p == permutation:
                    break
            else:
                raise ValueError('permutation not found')
        if offset > stream.size():
            raise RuntimeError('invalid offset')

        stream.seek(2 * 4 + header_size * 3 * 4)
        string_table = _StringTable(stream)

        stream.seek(offset)
        pass_count = stream.read_uint8()

        self.passes = []
        for pass_index in xrange(pass_count):
            self.passes.append(Pass(stream, string_table, version))

        self.annotations = {}
        for i in xrange(stream.read_uint16()):
            annotation = ParameterAnnotation(stream, string_table)
            self.annotations[annotation.name] = annotation

        self.parameters = self._extract_parameters('constants')
        self.resources = self._extract_parameters('resources')
        self.uavs = self._extract_parameters('uavs')
        self.resources.update(self.uavs)

    def _extract_parameters(self, stage_attr):
        result = {}
        for p in self.passes:
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
