import logging
import os
import subprocess
import sys
import tempfile
import wx
import wx.dataview

import yaml

from inputs import CreateInputsView
import rga
from stats import CreateStatsView


def _GetShaderCompilerPath():
    if getattr(sys, 'frozen', False):
        exeDir = os.path.dirname(sys.executable)
    else:
        exeDir = os.path.dirname(__file__)
    return os.path.join(exeDir, '..', 'ShaderCompiler.exe')


def _GetShaderInfo(path, defines):
    tempFile = tempfile.NamedTemporaryFile(delete=False)
    tempFile.close()
    cmdLine = [_GetShaderCompilerPath(), '/no_warnings', '/no_permutations', '/single', '/listing', tempFile.name]
    for k, v in defines.items():
        cmdLine.append('/define')
        cmdLine.append(k)
        cmdLine.append(str(v))
    cmdLine.append(path)

    tempOutput = tempfile.NamedTemporaryFile(delete=False)
    tempOutput.close()
    try:
        cmdLine.append(tempOutput.name)

        startupinfo = subprocess.STARTUPINFO()
        startupinfo.dwFlags = subprocess.CREATE_NEW_CONSOLE | 1
        startupinfo.wShowWindow = 0

        subprocess.check_output(cmdLine, startupinfo=startupinfo)

        tempFile = open(tempFile.name)
        yamlStr = tempFile.read()
        tempFile.close()

        return yaml.load(yamlStr, Loader=yaml.CLoader)
    finally:
        os.unlink(tempFile.name)


def _GetShaderPermutations(path):
    cmdLine = [_GetShaderCompilerPath(), '/single', '/permutations', path]

    startupinfo = subprocess.STARTUPINFO()
    startupinfo.dwFlags = subprocess.CREATE_NEW_CONSOLE | 1
    startupinfo.wShowWindow = 0

    output = subprocess.check_output(cmdLine, startupinfo=startupinfo)
    return yaml.load(output, Loader=yaml.CLoader) or {}


class _FileData(object):
    def __init__(self, path, node):
        self.path = path
        self.node = node
        self.currentDefines = {}
        self.permutations = {}
        self.compiledData = {}

    def GetFileData(self):
        return self

    def GetInfo(self):
        return self.compiledData[_FreezeDict(self.currentDefines)]

    def GetParentData(self):
        return None

    def GetRootData(self):
        return self

    def GetNodePath(self):
        return [self.path]


class _TechniqueData(object):
    def __init__(self, fd, node, techniqueName):
        self.fileData = fd
        self.node = node
        self.techniqueName = techniqueName

    def GetFileData(self):
        return self.fileData

    def GetInfo(self):
        fd = self.fileData.GetInfo()
        for each in fd['permutation']['techniques']:
            if each['name'] == self.techniqueName:
                return each
        return None

    def GetParentData(self):
        return self.fileData

    def GetRootData(self):
        return self.fileData.GetRootData()

    def GetNodePath(self):
        return self.fileData.GetNodePath() + [self.techniqueName]


class _PassData(object):
    def __init__(self, techniqueData, node, passIndex):
        self.techniqueData = techniqueData
        self.node = node
        self.passIndex = passIndex

    def GetFileData(self):
        return self.techniqueData.GetFileData()

    def GetInfo(self):
        td = self.techniqueData.GetInfo()
        if td:
            return td['passes'][self.passIndex]
        return None

    def GetParentData(self):
        return self.techniqueData

    def GetRootData(self):
        return self.techniqueData.GetRootData()

    def GetNodePath(self):
        return self.techniqueData.GetNodePath() + [self.passIndex]


class _StageData(object):
    def __init__(self, passData, node, stageIndex):
        self.passData = passData
        self.node = node
        self.stageIndex = stageIndex

    def GetFileData(self):
        return self.passData.GetFileData()

    def GetInfo(self):
        pd = self.passData.GetInfo()
        if pd:
            return pd[self.stageIndex]
        return None

    def GetNodePath(self):
        return self.passData.GetNodePath() + [self.stageIndex]

    def GetParentData(self):
        return self.passData

    def GetRootData(self):
        return self.passData.GetRootData()


_PLATFORMS = (
    {'name': 'DirectX 11', 'value': 2},
    {'name': 'DirectX 9', 'value': 1},
    {'name': 'GLES 2', 'value': 3},
    {'name': 'DirectX 12', 'value': 6},
    {'name': 'Vulkan', 'value': 7},
    {'name': 'OpenGL 3', 'value': 8},
    {'name': 'OpenGL 4', 'value': 9},
)
_SHADER_MODELS = {'name': 'DEPTH', 'value': 5}, {'name': 'HIGH', 'value': 4}, {'name': 'LOW', 'value': 3}


class OptionsPanel(wx.Panel):
    def __init__(self, *args, **kwargs):
        super(OptionsPanel, self).__init__(*args, **kwargs)
        self._fileData = None

    def SetFile(self, fd):
        if self._fileData == fd:
            return
        self._fileData = fd
        self._RefreshUI()

    def _AddCombo(self, name, options):
        cb = wx.ComboBox(self, choices=[], style=wx.CB_READONLY)
        cb.SetLabel(name)
        selected = self._fileData.currentDefines.get(name, None)
        for option in options:
            cb.Append(option['name'], option['value'])
            if option['value'] == selected:
                cb.SetSelection(cb.GetCount() - 1)
        cb.Bind(wx.EVT_COMBOBOX, self._OnPermutation)
        return cb

    def _RefreshUI(self):
        self.Freeze()
        try:
            self.DestroyChildren()

            sizer = wx.GridBagSizer(2, 2)
            sizer.Add(wx.StaticText(self, label='Platform'), (0, 0))
            self._platform = self._AddCombo('PLATFORM', _PLATFORMS)
            sizer.Add(self._platform, (0, 1), flag=wx.EXPAND)

            sizer.Add(wx.StaticText(self, label='Shader Model'), (1, 0))
            self._sm = self._AddCombo('SHADERMODEL', _SHADER_MODELS)
            sizer.Add(self._sm, (1, 1), flag=wx.EXPAND)

            if self._fileData:
                for i, name in enumerate(sorted(self._fileData.permutations.keys())):
                    description = self._fileData.permutations[name]
                    sizer.Add(wx.StaticText(self, label=name), (i + 2, 0))
                    cb = self._AddCombo(name, description['options'])
                    sizer.Add(cb, (i + 2, 1), flag=wx.EXPAND)

            sizer.AddGrowableCol(1)
            self.SetSizer(sizer)
            self.Layout()
        finally:
            self.Thaw()

    def GetCommonDefines(self):
        return {'PLATFORM': _PLATFORMS[self._platform.GetSelection()][1],
                'SHADERMODEL': _SHADER_MODELS[self._sm.GetSelection()][1]}

    def _OnPermutation(self, evt):
        cb = evt.GetEventObject()
        if self._fileData:
            self._fileData.currentDefines[cb.GetLabel()] = cb.GetClientData(cb.GetSelection())

            self.GetParent().Recompile(self._fileData)


_SHADER_STAGE_NAMES = {'vs': 'Vertex Shader', 'ps': 'Pixel Shader', 'gs': 'Geometry Shader', 'cs': 'Compute Shader',
                       'hs': 'Hull Shader', 'ds': 'Domain Shader'}

_shaderInfoPage = ''


class ShaderInfoPanel(wx.Panel):
    def __init__(self, parent, si):
        super(ShaderInfoPanel, self).__init__(parent)
        info = si.GetInfo()
        nb = wx.Notebook(self)

        nb.AddPage(CreateStatsView(nb, info), 'Stats')

        asm = wx.TextCtrl(nb, style=wx.TE_MULTILINE | wx.TE_READONLY | wx.TE_RICH2 | wx.TE_DONTWRAP,
                          value=info['original']['asm'])
        asm.SetFont(wx.Font(10, wx.FONTFAMILY_TELETYPE, wx.FONTSTYLE_NORMAL, wx.FONTWEIGHT_NORMAL))
        nb.AddPage(asm, 'Assembly')
        if _shaderInfoPage == 'Assembly':
            nb.SetSelection(nb.GetPageCount() - 1)

        if 'source' in info['original']:
            src = wx.TextCtrl(nb, style=wx.TE_MULTILINE | wx.TE_READONLY | wx.TE_RICH2 | wx.TE_DONTWRAP,
                              value=info['original']['source'])
            src.SetFont(wx.Font(10, wx.FONTFAMILY_TELETYPE, wx.FONTSTYLE_NORMAL, wx.FONTWEIGHT_NORMAL))
            nb.AddPage(src, 'Source')
            if _shaderInfoPage == 'Source':
                nb.SetSelection(nb.GetPageCount() - 1)

        nb.AddPage(CreateInputsView(nb, info), 'Inputs')
        if _shaderInfoPage == 'Inputs':
            nb.SetSelection(nb.GetPageCount() - 1)

        sizer = wx.BoxSizer(wx.VERTICAL)
        sizer.Add(nb, 1, wx.EXPAND)
        self.SetSizer(sizer)

        nb.Bind(wx.EVT_NOTEBOOK_PAGE_CHANGED, self._OnPageChanged)
        self._notebook = nb

    def _OnPageChanged(self, evt):
        global _shaderInfoPage
        _shaderInfoPage = evt.GetEventObject().GetPageText(evt.GetSelection())


class ErrorPanel(wx.Panel):
    def __init__(self, parent, errors):
        super(ErrorPanel, self).__init__(parent)

        sizer = wx.BoxSizer(wx.VERTICAL)
        sizer.Add(wx.StaticText(self, label='Compile errors'), 0, wx.EXPAND)
        sizer.Add(wx.TextCtrl(self, value=errors, style=wx.TE_MULTILINE | wx.TE_BESTWRAP | wx.TE_READONLY), 1, wx.EXPAND)
        self.SetSizer(sizer)


def _AddToolBarTool(tb, label, help):
    if getattr(sys, 'frozen', False):
        return tb.AddTool(-1, label, wx.NullBitmap, shortHelp=help)
    else:
        t = tb.AddSimpleTool(-1, wx.NullBitmap, shortHelpString=help)
        t.SetLabel(label)
        return t


class MainPanel(wx.Panel):
    def __init__(self, *args, **kwargs):
        super(MainPanel, self).__init__(*args, **kwargs)

        sizer = wx.BoxSizer(wx.HORIZONTAL)

        self._tree = wx.TreeCtrl(self, style=wx.TR_HIDE_ROOT | wx.TR_LINES_AT_ROOT | wx.TR_HAS_BUTTONS | wx.TR_TWIST_BUTTONS | wx.TR_NO_LINES)
        sizer.Add(self._tree, 1, wx.EXPAND)
        self._main = wx.Panel(self)
        sizer.Add(self._main, 2, wx.EXPAND)

        tb = wx.ToolBar(self, style=wx.TB_HORZ_TEXT | wx.TB_NOICONS | wx.TB_NODIVIDER | wx.TB_FLAT)
        openBtn = _AddToolBarTool(tb, 'Open...', 'Open Shader')
        reloadBtn = _AddToolBarTool(tb, 'Reload', 'Reload Shader')
        tb.AddStretchableSpace()
        settingsBtn = _AddToolBarTool(tb, 'Settings...', 'Open Settings Dialog')

        self.Bind(wx.EVT_TOOL, self._OnOpen, id=openBtn.GetId())
        self.Bind(wx.EVT_TOOL, self._OnRecompile, id=reloadBtn.GetId())
        self.Bind(wx.EVT_TOOL, self._OnSettings, id=settingsBtn.GetId())
        tb.Realize()

        vsizer = wx.BoxSizer(wx.VERTICAL)
        vsizer.Add(tb, 0, wx.EXPAND)

        self._options = OptionsPanel(self)
        vsizer.Add(self._options, 1, wx.EXPAND)

        sizer.Add(vsizer, 1, wx.EXPAND)
        self.SetSizer(sizer)
        self._tree.Bind(wx.EVT_TREE_SEL_CHANGED, self._OnItemSelected)

        self._tree.AddRoot('root')

        self.SetMinSize((800, 400))

    def AddFile(self, path):
        fn = self._tree.AppendItem(self._tree.GetRootItem(), os.path.basename(path))
        fd = _FileData(path, fn)
        self._tree.SetItemPyData(fn, fd)
        fd.permutations = _GetShaderPermutations(path)
        fd.currentDefines['PLATFORM'] = _PLATFORMS[0]['value']
        fd.currentDefines['SHADERMODEL'] = _SHADER_MODELS[0]['value']
        for k, desc in fd.permutations.items():
            v = desc['options'][0]['value']
            if 'default' in desc:
                for each in desc['options']:
                    if each['name'] == desc['default']:
                        v = each['value']
                        break
            fd.currentDefines[k] = v
        self.Recompile(fd)
        self._tree.ExpandAllChildren(fn)

    def _OnItemSelected(self, _):
        fd = self._tree.GetItemPyData(self._tree.GetSelection())
        self.Freeze()
        try:
            self._options.SetFile(fd.GetFileData())
            self._main.DestroyChildren()
            if isinstance(fd, _StageData):
                sizer = wx.BoxSizer(wx.VERTICAL)
                sizer.Add(ShaderInfoPanel(self._main, fd), 1, wx.EXPAND)
                self._main.SetSizer(sizer)
                self._main.Layout()
            elif isinstance(fd, _FileData):
                si = fd.compiledData.get(_FreezeDict(fd.currentDefines), {})
                if 'errors' in si:
                    sizer = wx.BoxSizer(wx.VERTICAL)
                    sizer.Add(ErrorPanel(self._main, si['errors']), 1, wx.EXPAND)
                    self._main.SetSizer(sizer)
                    self._main.Layout()
        finally:
            self.Thaw()

    def Recompile(self, fd):
        if self._tree.GetSelection():
            pfd = self._tree.GetItemPyData(self._tree.GetSelection())
            path = pfd.GetNodePath() if pfd and pfd.GetRootData() == fd else []
        else:
            path = []
        key = _FreezeDict(fd.currentDefines)
        if key in fd.compiledData:
            si = fd.compiledData[key]
        else:
            try:
                si = _GetShaderInfo(fd.path, fd.currentDefines)
            except subprocess.CalledProcessError as e:
                si = {'errors': e.output}
            fd.compiledData[key] = si
        self._PopulateNodes(fd, si)
        if path:
            self._SelectPath(fd.node, path)

    def _PopulateNodes(self, fd, si):
        self._tree.DeleteChildren(fd.node)
        if 'errors' in si:
            return
        for each in si['permutation']['techniques']:
            tn = self._tree.AppendItem(fd.node, each['name'])
            td = _TechniqueData(fd, tn, each['name'])
            self._tree.SetItemPyData(tn, td)
            if len(each['passes']) > 1:
                for i, p in enumerate(each['passes']):
                    pn = self._tree.AppendItem(tn, 'Pass #%s' % (i + 1))
                    pd = _PassData(td, pn, i)
                    self._tree.SetItemPyData(pn, pd)
                    for j, s in enumerate(p):
                        name = s['profile'][:2]
                        sn = self._tree.AppendItem(pn, _SHADER_STAGE_NAMES.get(name.lower(), name))
                        self._tree.SetItemPyData(sn, _StageData(pd, sn, j))
            elif each['passes']:
                pd = _PassData(td, None, 0)
                for j, s in enumerate(each['passes'][0]):
                    name = s['profile'][:2]
                    sn = self._tree.AppendItem(tn, _SHADER_STAGE_NAMES.get(name.lower(), name))
                    self._tree.SetItemPyData(sn, _StageData(pd, sn, j))

    def _SelectPath(self, node, path):
        nodes = _GetDescendants(self._tree, node)
        for each in nodes:
            if self._tree.GetItemPyData(each).GetNodePath() == path:
                self._tree.SelectItem(each)
                self._OnItemSelected(None)
                return
        self._tree.SelectItem(node)
        self._OnItemSelected(None)

    def _OnOpen(self, _):
        fd = wx.FileDialog(self, 'Select Effect File', wildcard='Effects (*.fx)|*.fx',
                           style=wx.FD_OPEN | wx.FD_FILE_MUST_EXIST)
        if fd.ShowModal() != wx.ID_OK:
            return

        self.AddFile(fd.GetPath())

    def _OnSettings(self, _):
        dlg = Settings(self)
        if dlg.ShowModal() == wx.ID_OK:
            wx.Config.Get().Write(rga.RGA_PATH_KEY, dlg.rgaPath.GetValue())
            wx.Config.Get().Write(rga.RGA_TARGETS_KEY, dlg.rgaTargets.GetValue())

    def _OnRecompile(self, _):
        pfd = self._tree.GetItemPyData(self._tree.GetSelection())
        fd = pfd.GetRootData()
        fd.compiledData.clear()
        self.Freeze()
        try:
            self.Recompile(fd)
        finally:
            self.Thaw()


def _GetDescendants(treeCtrl, node):
    result = []
    item, cookie = treeCtrl.GetFirstChild(node)
    while item.IsOk():
        result.append(item)
        result.extend(_GetDescendants(treeCtrl, item))
        item, cookie = treeCtrl.GetNextChild(node, cookie)
    return result


def _FreezeDict(d):
    return tuple(d.items())


# noinspection PyMethodOverriding
class ExeFileValidator(wx.PyValidator):
    def Clone(self):
        return self.__class__()

    def Validate(self, window):
        ctrl = self.GetWindow()
        value = ctrl.GetValue()
        if value and (not os.path.exists(value) or not value.lower().endswith('.exe')):
            ctrl.SetBackgroundColour('pink')
            ctrl.SetFocus()
            ctrl.Refresh()
            return False
        else:
            ctrl.SetBackgroundColour(wx.SystemSettings.GetColour(wx.SYS_COLOUR_WINDOW))
            ctrl.Refresh()
            return True

    def TransferToWindow(self):
        return True

    def TransferFromWindow(self):
        return True


class Settings(wx.Dialog):
    def __init__(self, parent):
        super(Settings, self).__init__(parent, title='Settings')

        mainSizer = wx.BoxSizer(wx.VERTICAL)

        contentSizer = wx.GridBagSizer(2, 2)
        contentSizer.Add(wx.StaticText(self, label='RGA Path'), (0, 0))

        vsizer = wx.BoxSizer(wx.HORIZONTAL)
        self.rgaPath = wx.TextCtrl(self, value=wx.Config.Get().Read(rga.RGA_PATH_KEY),
                                   validator=ExeFileValidator())
        vsizer.Add(self.rgaPath, 1, wx.EXPAND)
        browse = wx.Button(self, label='Browse...')
        browse.Bind(wx.EVT_BUTTON, self._OnBrowse)
        vsizer.Add(browse, 0, wx.EXPAND)
        contentSizer.Add(vsizer, (0, 1), flag=wx.EXPAND)
        contentSizer.Add(wx.StaticText(self, label='RGA Platform Targets'), (1, 0))
        self.rgaTargets = wx.TextCtrl(self, value=wx.Config.Get().Read(rga.RGA_TARGETS_KEY))
        contentSizer.Add(self.rgaTargets, (1, 1), flag=wx.EXPAND)
        contentSizer.AddGrowableCol(1)

        mainSizer.Add(contentSizer, 1, wx.EXPAND | wx.ALL, 5)

        btnSizer = wx.StdDialogButtonSizer()
        btn = wx.Button(self, wx.ID_OK)
        btn.SetDefault()
        btnSizer.AddButton(btn)

        btn = wx.Button(self, wx.ID_CANCEL)
        btnSizer.AddButton(btn)
        btnSizer.Realize()

        mainSizer.Add(btnSizer, 0, wx.ALIGN_CENTER | wx.ALL, 5)
        self.SetSizerAndFit(mainSizer)
        self.Validate()

    def _OnBrowse(self, _):
        fd = wx.FileDialog(self, 'Select RGA CLI Executable', wildcard='Executables (*.exe)|*.exe',
                           style=wx.FD_OPEN | wx.FD_FILE_MUST_EXIST,
                           defaultFile=self.rgaPath.GetValue() or rga.GetRgaDefaultPath())
        if fd.ShowModal() != wx.ID_OK:
            return
        self.rgaPath.SetValue(fd.GetPath())


if __name__ == '__main__':
    if getattr(sys, 'frozen', False):
        sys.stderr = open(os.path.join(os.path.dirname(sys.executable), 'fxinfo.log'), 'w')
        sys.stdout = sys.stderr

    logging.basicConfig()
    app = wx.App(0)
    app.SetAppName('FXInfo')
    app.SetVendorName('CCP')

    try:
        f = wx.Frame(None, title='FXInfo')
        s = wx.BoxSizer(wx.VERTICAL)
        p = MainPanel(f)
        s.Add(p, 1, wx.EXPAND)
        f.SetSizerAndFit(s)

        for each in sys.argv[1:]:
            p.AddFile(each)

        f.Show()
        app.SetTopWindow(f)
    except:
        logging.exception('Exception when creating main window')
        if getattr(sys, 'frozen', False):
            wx.MessageBox('Could not initialize the main window. See fxinfo.log for details')
    else:
        app.MainLoop()

