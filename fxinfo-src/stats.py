import wx.dataview

import rga


class _StatNode(object):
    def __init__(self, parent, name, value):
        self.parent = parent
        self.name = name
        self.value = value
        self.children = []


# noinspection PyMethodOverriding
class StatModel(wx.dataview.PyDataViewModel):
    def __init__(self, stageInfo):
        super(StatModel, self).__init__()
        self._stageInfo = stageInfo
        self._parents = []
        self._rgaStats = None

        self._AddStats(stageInfo['original'].get('stats', {}), None, self._parents)

        if 'amdStats' in stageInfo:
            if isinstance(stageInfo['amdStats'], dict):
                n = _StatNode(None, 'AMD Stats', '')
                self._AddStats(stageInfo['amdStats'], n, n.children)
                self._parents.append(n)
            elif stageInfo['amdStats'] == 'error':
                self._parents.append(_StatNode(None, 'AMD Stats', 'error'))
        else:
            targets = wx.Config.Get().Read(rga.RGA_TARGETS_KEY)
            if targets:
                targets = targets.split(' ')
            else:
                targets = ()
            if rga.GetRgaStats(stageInfo, self._OnRgaDone, self._OnRgaFail,
                               wx.Config.Get().Read(rga.RGA_PATH_KEY), targets):
                self._rgaStats = _StatNode(None, 'AMD Stats', 'compiling')
                self._parents.append(self._rgaStats)

    def _OnRgaDone(self, results):
        self._stageInfo['amdStats'] = results
        if not self._rgaStats:
            return
        self._rgaStats.value = ''
        self._AddStats(results, self._rgaStats, self._rgaStats.children)
        self.ItemChanged(self.ObjectToItem(self._rgaStats))
        for each in self._rgaStats.children:
            self.ItemAdded(self.ObjectToItem(self._rgaStats), self.ObjectToItem(each))

    def _OnRgaFail(self, _):
        self._stageInfo['amdStats'] = 'error'
        if not self._rgaStats:
            return
        self._rgaStats.value = 'error'
        self.ItemChanged(self.ObjectToItem(self._rgaStats))

    def _AddStats(self, stats, parent, parentList):
        for name in sorted(stats.keys()):
            value = stats[name]
            if isinstance(value, dict):
                n = _StatNode(parent, name, '')
                self._AddStats(value, n, n.children)
                if n.children:
                    parentList.append(n)
            elif value:
                parentList.append(_StatNode(parent, name, value))

    def GetColumnCount(self):
        return 2

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
        else:
            return node.value


def CreateStatsView(parent, stageInfo):
    inputs = wx.dataview.DataViewCtrl(parent, style=wx.dataview.DV_SINGLE | wx.dataview.DV_ROW_LINES)
    model = StatModel(stageInfo)
    inputs.AppendTextColumn('Name', 0, width=200, flags=wx.dataview.DATAVIEW_COL_RESIZABLE|wx.dataview.DATAVIEW_COL_SORTABLE)
    inputs.AppendTextColumn('Value', 1, width=100, flags=wx.dataview.DATAVIEW_COL_RESIZABLE|wx.dataview.DATAVIEW_COL_SORTABLE)
    inputs.AssociateModel(model)

    roots = []
    model.GetChildren(None, roots)
    for each in roots:
        inputs.Expand(each)

    return inputs
