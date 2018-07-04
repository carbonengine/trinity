import wx.dataview


class _InputNode(object):
    def __init__(self, parent, name, register, valueType, value):
        self.parent = parent
        self.name = name
        self.register = register
        self.type = valueType
        self.value = value
        self.children = []


def _GetConstantType(c):
    result = c['constantType']
    if c.get('dimension', 0) > 1:
        result += str(c['dimension'])
    if c.get('arrayElements', 0) > 1:
        result += '[%s]' % c['arrayElements']
    return result


def _GetUsedMaskLabel(mask):
    result = ''
    for i, l in enumerate('XYZW'):
        if mask & (1 << i):
            result += l
        else:
            result += '-'
    return result


# noinspection PyMethodOverriding
class ShaderInputModel(wx.dataview.PyDataViewModel):
    def __init__(self, stageInfo):
        super(ShaderInputModel, self).__init__()
        self._stageInfo = stageInfo
        self._parents = []
        if 'constants' in stageInfo:
            node = _InputNode(None, 'Constants', -1, '', '')
            for x in stageInfo['constants']:
                cn = _InputNode(node, x['name'], -1, _GetConstantType(x), '')
                node.children.append(cn)
                for y in x.get('annotations', ()):
                    cn.children.append(_InputNode(cn, y['name'], -1, y['annotationType'], y['value']))
            self._parents.append(node)
        if 'textures' in stageInfo:
            node = _InputNode(None, 'Textures', -1, '', '')
            for x in stageInfo['textures']:
                cn = _InputNode(node, x['name'], x['register'], x['textureType'], '')
                node.children.append(cn)
                for y in x.get('annotations', ()):
                    cn.children.append(_InputNode(cn, y['name'], -1, y['annotationType'], y['value']))
            self._parents.append(node)
        if 'samplers' in stageInfo:
            node = _InputNode(None, 'Samplers', -1, '', '')
            for x in stageInfo['samplers']:
                cn = _InputNode(node, x['name'], x['register'], '', '')
                node.children.append(cn)
                for y in x:
                    if y in ('name', 'register'):
                        continue
                    cn.children.append(_InputNode(cn, y, -1, '', str(x[y])))
            self._parents.append(node)
        if 'inputs' in stageInfo:
            node = _InputNode(None, 'Pipeline Inputs', -1, '', '')
            for x in stageInfo['inputs']:
                usages = [
                    'POSITION',
                    'COLOR',
                    'NORMAL',
                    'TANGENT',
                    'BITANGENT',
                    'TEXCOORD',
                    'BLENDINDICES',
                    'BLENDWEIGHTS']

                cn = _InputNode(node, '%s%s' % (usages[x['name']], x['index']), x['register'], '', _GetUsedMaskLabel(x['usedMask']))
                node.children.append(cn)
            self._parents.append(node)


    def GetColumnCount(self):
        return 4

    def GetChildren(self, parent, children):
        if not parent:
            for each in self._parents:
                children.append(self.ObjectToItem(each))
            return len(self._parents)
        node = self.ItemToObject(parent)
        for each in node.children:
            children.append(self.ObjectToItem(each))
        return len(node.children)

    def IsContainer(self, item):
        if not item:
            return True
        node = self.ItemToObject(item)
        return bool(node.children)

    def HasContainerColumns(self, _):
        return True

    def GetParent(self, item):
        if not item:
            return wx.dataview.NullDataViewItem
        node = self.ItemToObject(item)
        if node.parent:
            return self.ObjectToItem(node.parent)
        else:
            return wx.dataview.NullDataViewItem

    def GetValue(self, item, col):
        node = self.ItemToObject(item)
        if col == 0:
            return node.name
        elif col == 1:
            return node.register if node.register != -1 else ''
        elif col == 2:
            return node.type
        else:
            return node.value


def CreateInputsView(parent, stageInfo):
    inputs = wx.dataview.DataViewCtrl(parent, style=wx.dataview.DV_SINGLE | wx.dataview.DV_ROW_LINES)
    inputs.AssociateModel(ShaderInputModel(stageInfo))
    inputs.AppendTextColumn('Name', 0, width=200, flags=wx.dataview.DATAVIEW_COL_RESIZABLE|wx.dataview.DATAVIEW_COL_SORTABLE)
    inputs.AppendTextColumn('Register', 1, width=100, flags=wx.dataview.DATAVIEW_COL_RESIZABLE|wx.dataview.DATAVIEW_COL_SORTABLE)
    inputs.AppendTextColumn('Type', 2, width=100, flags=wx.dataview.DATAVIEW_COL_RESIZABLE|wx.dataview.DATAVIEW_COL_SORTABLE)
    inputs.AppendTextColumn('Value', 3, width=200, flags=wx.dataview.DATAVIEW_COL_RESIZABLE|wx.dataview.DATAVIEW_COL_SORTABLE)
    return inputs
