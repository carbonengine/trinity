# Copyright (c) CCP 2012
"""
Displays an interactive window with compiled shader.
"""
import wx
import yaml
import os
import subprocess
import sys
import tempfile


def OnItemSelected(inputType, annotationList, stage):
    """
    Returns event callback for constant/texture/etc. list.
    """


    def callback(event):
        """
        Callback for constant/texture/etc. list. Updates annotation list
        """
        annotationList.DeleteAllItems()
        try:
            if inputType.GetSelection() == 0:
                c = stage['constants'][event.GetIndex()]
                for annotation in c['annotations']:
                    index = annotationList.InsertStringItem(sys.maxint, annotation['name'])
                    annotationList.SetStringItem(index, 1, annotation['annotationType'])
                    annotationList.SetStringItem(index, 2, str(annotation['value']))
            elif inputType.GetSelection() == 1:
                c = stage['textures'][event.GetIndex()]
                for annotation in c['annotations']:
                    index = annotationList.InsertStringItem(sys.maxint, annotation['name'])
                    annotationList.SetStringItem(index, 1, annotation['annotationType'])
                    annotationList.SetStringItem(index, 2, str(annotation['value']))
            elif inputType.GetSelection() == 3:
                c = stage['uavs'][event.GetIndex()]
                for annotation in c['annotations']:
                    index = annotationList.InsertStringItem(sys.maxint, annotation['name'])
                    annotationList.SetStringItem(index, 1, annotation['annotationType'])
                    annotationList.SetStringItem(index, 2, str(annotation['value']))
        except:
            pass
    return callback
    

def OnInputType(inputType, itemList, annotationList, stage):
    """
    Returns event callback for constant/texture/etc. combobox selection.
    """


    def callback(event):
        """
        Callback for constant/texture/etc. list. Updates constant/texture/etc. list
        """
        itemList.DeleteAllItems()
        annotationList.DeleteAllItems()
        itemList.DeleteAllColumns()
        if inputType.GetSelection() == 0:
            # constants
            itemList.InsertColumn(0, 'Name')
            itemList.InsertColumn(1, 'Type')
            itemList.InsertColumn(2, 'Autoregister')
            itemList.InsertColumn(3, 'sRGB')
            if 'constants' in stage:
                for c in stage['constants']:
                    index = itemList.InsertStringItem(sys.maxint, c['name'])
                    constType = c['constantType']
                    if c['dimension'] > 1:
                        if c['dimension'] == 16:
                            constType += '4x4'
                        else:
                            constType += '%s' % c['dimension']
                    if c['arrayElements'] > 1:
                        constType += '[%s]' % c['arrayElements']
                    itemList.SetStringItem(index, 1, constType)
                    itemList.SetStringItem(index, 2, 'Yes' if c['autoregister'] else 'No')
                    itemList.SetStringItem(index, 3, 'Yes' if c['sRGB'] else 'No')
        elif inputType.GetSelection() == 1:
            # textures
            itemList.InsertColumn(0, 'Register')
            itemList.InsertColumn(1, 'Name')
            itemList.InsertColumn(2, 'Type')
            itemList.InsertColumn(3, 'Autoregister')
            itemList.InsertColumn(4, 'sRGB')
            if 'textures' in stage:
                for c in stage['textures']:
                    index = itemList.InsertStringItem(sys.maxint, str(c['register']))
                    itemList.SetStringItem(index, 1, c['name'])
                    itemList.SetStringItem(index, 2, c['textureType'])
                    itemList.SetStringItem(index, 3, 'Yes' if c['autoregister'] else 'No')
                    itemList.SetStringItem(index, 4, 'Yes' if c['sRGB'] else 'No')
        elif inputType.GetSelection() == 2:
            # samplers
            itemList.InsertColumn(0, 'Register')
            itemList.InsertColumn(1, 'Register')
            itemList.InsertColumn(2, 'MinFilter')
            itemList.InsertColumn(3, 'MagFilter')
            itemList.InsertColumn(4, 'MipFilter')
            itemList.InsertColumn(5, 'AddressU')
            itemList.InsertColumn(6, 'AddressV')
            itemList.InsertColumn(7, 'AddressW')
            itemList.InsertColumn(8, 'MaxAnisotropy')
            itemList.InsertColumn(9, 'MinLOD')
            itemList.InsertColumn(10, 'MaxLOD')
            itemList.InsertColumn(11, 'MipLODBias')
            itemList.InsertColumn(12, 'Comparison')
            itemList.InsertColumn(13, 'Border Color')
            
            filters = {0: 'None', 1: 'Point', 2: 'Linear', 3: 'Anisotropic', 6: 'PyramidalQuad', 7: 'GaussianQuad', 8: 'ConvolutionMono'}
            addressModes = {1: 'Wrap', 2: 'Mirror', 3: 'Clamp', 4: 'Border', 5: 'MirrorOnce'}
            if 'samplers' in stage:
                for c in stage['samplers']:
                    index = itemList.InsertStringItem(sys.maxint, str(c['register']))
                    itemList.SetStringItem(index, 1, c.get('name', ''))
                    itemList.SetStringItem(index, 2, filters[c['minFilter']])
                    itemList.SetStringItem(index, 3, filters[c['magFilter']])
                    itemList.SetStringItem(index, 4, filters[c['mipFilter']])
                    itemList.SetStringItem(index, 5, addressModes[c['addressU']])
                    itemList.SetStringItem(index, 6, addressModes[c['addressV']])
                    itemList.SetStringItem(index, 7, addressModes[c['addressW']])
                    itemList.SetStringItem(index, 8, str(c['maxAnisotropy']))
                    itemList.SetStringItem(index, 9, str(c['minLOD']))
                    itemList.SetStringItem(index, 10, str(c['maxLOD']))
                    itemList.SetStringItem(index, 11, str(c['mipLODBias']))
                    itemList.SetStringItem(index, 12, str(c['comparison']))
                    itemList.SetStringItem(index, 13, str(c['borderColor']))
        elif inputType.GetSelection() == 3:
            # uavs
            itemList.InsertColumn(0, 'Register')
            itemList.InsertColumn(1, 'Name')
            itemList.InsertColumn(2, 'Type')
            itemList.InsertColumn(3, 'Autoregister')
            if 'uavs' in stage:
                for c in stage['uavs']:
                    index = itemList.InsertStringItem(sys.maxint, str(c['register']))
                    itemList.SetStringItem(index, 1, c['name'])
                    itemList.SetStringItem(index, 2, c['resourceType'])
                    itemList.SetStringItem(index, 3, 'Yes' if c['autoregister'] else 'No')
        else:
            # pipeline inputs
            itemList.InsertColumn(0, 'Register')
            itemList.InsertColumn(1, 'Name')
            itemList.InsertColumn(2, 'Index')
            itemList.InsertColumn(3, 'Used Mask')
            usages = [
                'POSITION', 	
                'COLOR',
                'NORMAL',
                'TANGENT',
                'BITANGENT',
                'TEXCOORD',
                'BLENDINDICES',
                'BLENDWEIGHTS']

            if 'inputs' in stage:
                for c in stage['inputs']:
                    index = itemList.InsertStringItem(sys.maxint, str(c['register']))
                    itemList.SetStringItem(index, 1, usages[c['name']])
                    itemList.SetStringItem(index, 2, str(c['index']))
                    itemList.SetStringItem(index, 3, bin(c['usedMask']))
    return callback


class ShaderWindow(wx.Frame):
    """
    Window with compiled shader and its situation flags.
    """
    
    
    def __init__(self, shaderFileName):
        wx.Frame.__init__(self, None, -1, '', size=(1024, 900))

        panel = wx.Panel(self)
        
        slashIndex = max(shaderFileName.rfind('/'), shaderFileName.rfind('\\'))
        self.SetTitle(shaderFileName[slashIndex + 1:])

        self.isShaderMaterial = shaderFileName.lower().find('graphics\\shaders') != -1
        if self.isShaderMaterial:
            baseName = shaderFileName[0:shaderFileName.lower().find('graphics\\shaders') + len('graphics\\shaders')]
            relName = shaderFileName[len(baseName):]
            self.redFileName = baseName + '\\ShaderDescriptions' + relName
            if self.redFileName[-3:] == '.fx':
                self.redFileName = self.redFileName[0:-2] + 'red'
        self.shaderFileName = shaderFileName

        hbox = wx.BoxSizer(wx.HORIZONTAL)

        self.notebook = wx.Notebook(panel)
        self.overview = wx.Panel(self.notebook)
        self.notebook.AddPage(self.overview, "Overview")

        hbox.Add(self.notebook, 1, wx.ALL | wx.EXPAND, 2)
        vbox = wx.BoxSizer(wx.VERTICAL)
        
        self.platform = wx.ComboBox(panel, choices = ['DX9', 'DX11', 'GLES2'], style=wx.CB_READONLY)
        self.platform.Select(0)
        self.platform.Bind(wx.EVT_COMBOBOX, self.OnPermutation)
        vbox.Add(self.platform, 0, wx.ALL | wx.EXPAND, 2)
        if self.isShaderMaterial:
            self.permutationCtrl = wx.TextCtrl(panel, -1, '0', style=wx.TE_PROCESS_ENTER)
            self.permutationCtrl.Bind(wx.EVT_TEXT_ENTER, self.OnPermutation)
            vbox.Add(self.permutationCtrl, 0, wx.ALL | wx.EXPAND, 2)

            self.base = wx.RadioBox(panel, choices=['dec', 'hex'], majorDimension=2, style=wx.RA_SPECIFY_COLS | wx.NO_BORDER)
            self.base.Bind(wx.EVT_RADIOBOX, self.OnPermutation)
            vbox.Add(self.base, 0, wx.ALL | wx.EXPAND, 2)
            
            self.situations = wx.CheckListBox(panel)
            self.situations.Bind(wx.EVT_CHECKLISTBOX, self.OnSituation)
            vbox.Add(self.situations, 1, wx.ALL | wx.EXPAND, 2)
        else:
            self.sm = wx.RadioBox(
                panel, 
                choices=['SM_DEPTH', 'SM_HIGH', 'SM_LOW'],
                majorDimension=1, 
                style=wx.RA_SPECIFY_COLS | wx.NO_BORDER)
            self.sm.Bind(wx.EVT_RADIOBOX, self.OnPermutation)
            vbox.Add(self.sm, 0, wx.ALL | wx.EXPAND, 2)
        hbox.Add(vbox, 0, wx.EXPAND)

        panel.SetSizer(hbox)

        if self.isShaderMaterial:
            self.GetSituations()

        self.Centre()
        
        self.CompileShader(1)


    def GetSituations(self):
        self.defines = []
        redFile = yaml.load(file(self.redFileName).read())
        if 'permuteTags' in redFile:
            for tag in redFile['permuteTags']: 
                self.situations.Append(tag['name'])
                self.defines.append((tag['permuteDefineString'], tag['tagBits']))


    def CompileShader(self, platform):
        tempFile = tempfile.NamedTemporaryFile(delete=False)
        tempFile.close()
        cmdLine = os.path.dirname(__file__) + r'\ShaderCompiler.exe /no_warnings /single /listing %s ' % tempFile.name
        cmdLine += ' /define PLATFORM %s' % platform
        if self.isShaderMaterial:
            for index in range(self.situations.GetCount()):
                cmdLine += " /define "
                cmdLine += self.defines[index][0]
                if self.situations.IsChecked(index):
                    cmdLine += " 1"
                else:
                    cmdLine += " 0"
        else:
            cmdLine += ' /define SHADERMODEL '
            cmdLine += '%s' % (5 - self.sm.GetSelection())
        cmdLine += " " + self.shaderFileName

        tempOutput = tempfile.NamedTemporaryFile(delete=False)
        tempOutput.close()
        
        cmdLine += ' '
        cmdLine += tempOutput.name
        
        compiles = subprocess.call(cmdLine) == 0
        
        for i in range(1, self.notebook.GetPageCount()):
            self.notebook.DeletePage(1)
            
        self.overview.DestroyChildren()
        vbox = wx.BoxSizer(wx.VERTICAL)
        if compiles:
            tempFile = open(tempFile.name)
            yamlStr = tempFile.read()
            tempFile.close()
            os.unlink(tempFile.name)
            contents = yaml.load(yamlStr)
            shaderTypes = ['VS', 'PS']
            if platform == 2:
                shaderTypes = ['VS', 'PS', 'GS', 'HS', 'DS', 'CS']
            passCount = len(contents['permutation']['passes'])
            columnCount = len(shaderTypes) + 1
            rowCount = 0
            for passData in contents['permutation']['passes']:
                rowCount += 2
                if passData is not None:
                    for stage in passData:
                        if 'patched' in stage:
                            rowCount += 1
            sz = wx.GridSizer(rowCount, columnCount)
            for i in range(passCount):
                sz.Add(wx.StaticText(self.overview, label='Pass %s' % (i + 1)), 0, wx.ALL | wx.EXPAND, 2)
                for s in shaderTypes:
                    sz.Add(wx.StaticText(self.overview, label=s, style=wx.ALIGN_RIGHT), 0, wx.ALL | wx.EXPAND, 2)
                sz.Add(wx.StaticText(self.overview, label='Instructions'), 0, wx.ALL | wx.EXPAND, 2)
                hasPatched = False
                for s in shaderTypes:
                    count = 0
                    thisStage = None
                    if contents['permutation']['passes'][i] is not None:
                        for stage in contents['permutation']['passes'][i]:
                            if stage['profile'][0:2].upper() == s:
                                if 'instructionCount' in stage['original']:
                                    count = stage['original']['instructionCount']
                                thisStage = stage
                    sz.Add(wx.StaticText(self.overview, label='%s' % count, style=wx.ALIGN_RIGHT), 0, wx.ALL | wx.EXPAND, 2)
                    if thisStage is not None:
                        if 'patched' in thisStage:
                            hasPatched = True
                        self.CreateStageUI(thisStage, contents, s, i)
                if hasPatched:
                    sz.Add(wx.StaticText(self.overview, label='Patched'), 0, wx.ALL | wx.EXPAND, 2)
                    for s in shaderTypes:
                        count = 0
                        for stage in contents['permutation']['passes'][i]:
                            if stage['profile'][0:2].upper() == s:
                                count = stage['patched']['instructionCount']
                        sz.Add(wx.StaticText(self.overview, label='%s' % count, style=wx.ALIGN_RIGHT), 0, wx.ALL | wx.EXPAND, 2)
            vbox.Add(sz, 0, wx.ALL | wx.EXPAND, 20)
            self.overview.SetSizer(vbox)
        else:
            msg = wx.StaticText(self.overview, label='Error compiling shader', style=wx.ALIGN_CENTRE)
            vbox.Add(msg, 1, wx.ALIGN_CENTER | wx.ALL, 20)
            self.overview.SetSizer(vbox)


    def OnSituation(self, event):
        self.CompileShader(self.platform.GetSelection() + 1)
        self.overview.Layout()
        self.UpdatePermutation()


    def CreateStageUI(self, thisStage, contents, shaderTypeName, passIndex):
        stageInfo = wx.Notebook(self.notebook)

        info = wx.ListCtrl(stageInfo, -1, style=wx.LC_REPORT)
        info.InsertColumn(0, 'Property', width=300)
        info.InsertColumn(1, 'Original')
        info.InsertColumn(2, 'Patched')
        for k in thisStage['original']:
            if k != 'asm' and k != 'source':
                index = info.InsertStringItem(sys.maxint, k)
                info.SetStringItem(index, 1, '%s' % thisStage['original'][k])
                if 'patched' in thisStage:
                    info.SetStringItem(index, 2, '%s' % thisStage['patched'][k])
        stageInfo.AddPage(info, "Information")

        source = wx.Panel(stageInfo)
        sizer = wx.BoxSizer(wx.VERTICAL)
        if 'patched' in thisStage:
            shaderType = wx.ComboBox(source, choices=['Original', 'Patched'], style=wx.CB_READONLY)
            shaderType.Select(0)
            sizer.Add(shaderType, 0, wx.ALL | wx.EXPAND, 2)
        sourceCode = thisStage['original']['source'] if 'source' in thisStage['original'] else contents['permutation']['source']
        text = wx.TextCtrl(source, value=sourceCode, style=wx.TE_MULTILINE|wx.TE_READONLY|wx.TE_DONTWRAP)
        text.SetFont(wx.Font(10, wx.FONTFAMILY_TELETYPE, wx.FONTSTYLE_NORMAL, wx.FONTWEIGHT_NORMAL))
        if 'patched' in thisStage:
        
        
            def OnSourceSelect(thisStage, shaderType, text):
            
            
                def callback(event):
                    key = 'original' if shaderType.GetSelection() == 0 else 'patched'
                    sourceCode = thisStage[key]['source'] if 'source' in thisStage[key] else contents['permutation']['source']
                    text.SetValue(sourceCode)
                return callback
            shaderType.Bind(wx.EVT_COMBOBOX, OnSourceSelect(thisStage, shaderType, text))

        sizer.Add(text, 1, wx.ALL | wx.EXPAND, 2)
        source.SetSizer(sizer)
        stageInfo.AddPage(source, "Source")
        
        asm = wx.Panel(stageInfo)
        sizer = wx.BoxSizer(wx.VERTICAL)
        if 'patched' in thisStage:
            shaderType = wx.ComboBox(asm, choices=['Original', 'Patched'], style=wx.CB_READONLY)
            shaderType.Select(0)
            sizer.Add(shaderType, 0, wx.ALL | wx.EXPAND, 2)
        text = wx.TextCtrl(asm, value=thisStage['original']['asm'], style=wx.TE_MULTILINE|wx.TE_READONLY|wx.TE_DONTWRAP)
        text.SetFont(wx.Font(10, wx.FONTFAMILY_TELETYPE, wx.FONTSTYLE_NORMAL, wx.FONTWEIGHT_NORMAL))
        if 'patched' in thisStage:
        
        
            def OnAsmSourceSelect(thisStage, shaderType, text):
            
            
                def callback(event):
                    asmCode = thisStage['original']['asm'] if shaderType.GetSelection() == 0 else thisStage['patched']['asm']
                    text.SetValue(asmCode)
                return callback
            shaderType.Bind(wx.EVT_COMBOBOX, OnAsmSourceSelect(thisStage, shaderType, text))

        sizer.Add(text, 1, wx.ALL | wx.EXPAND, 2)
        asm.SetSizer(sizer)
        stageInfo.AddPage(asm, "Asm")
        stageInfo.SetSelection(2)

        inputs = wx.Panel(stageInfo)
        sizer = wx.BoxSizer(wx.VERTICAL)
        inputType = wx.ComboBox(inputs, choices=['Constants', 'Textures', 'Samplers', 'UAVs', 'Pipline Inputs'], style=wx.CB_READONLY)
        sizer.Add(inputType, 0, wx.ALL | wx.EXPAND, 2)
        
        itemList = wx.ListCtrl(inputs, style=wx.LC_REPORT| wx.LC_SINGLE_SEL)
        sizer.Add(itemList, 1, wx.ALL | wx.EXPAND, 2)
        
        sizer.Add(wx.StaticText(inputs, label='Annotations:'), 0, wx.TOP | wx.EXPAND, 20)
        
        annotationList = wx.ListCtrl(inputs, style=wx.LC_REPORT)
        annotationList.InsertColumn(0, 'Name')
        annotationList.InsertColumn(1, 'Type')
        annotationList.InsertColumn(2, 'Value')
        sizer.Add(annotationList, 0, wx.ALL | wx.EXPAND, 2)
        
        inputType.Bind(wx.EVT_COMBOBOX, OnInputType(inputType, itemList, annotationList, thisStage))
        inputType.Select(0)
        OnInputType(inputType, itemList, annotationList, thisStage)(None)

        itemList.Bind(wx.EVT_LIST_ITEM_SELECTED, OnItemSelected(inputType, annotationList, thisStage))

        inputs.SetSizer(sizer)
        stageInfo.AddPage(inputs, "Inputs")
        self.notebook.AddPage(stageInfo, 'Pass %s/%s' % (passIndex + 1, shaderTypeName))
        
        
    def OnPermutation(self, event):
        if self.isShaderMaterial:
            try:
                if self.base.GetSelection() == 0:
                    permutation = int(self.permutationCtrl.Value)
                else:
                    permutation = int(self.permutationCtrl.Value, 16)
            except:
                return
            for index in range(self.situations.GetCount()):
                if (permutation & self.defines[index][1]) == self.defines[index][1]:
                    self.situations.Check(index, True)
                else:
                    self.situations.Check(index, False)
        self.CompileShader(self.platform.GetSelection() + 1)
        self.overview.Layout()
        
        
    def UpdatePermutation(self):
        perm = 0
        for index in range(self.situations.GetCount()):
            if self.situations.IsChecked(index):
                perm += self.defines[index][1]
        if self.base.GetSelection() == 0:
            self.permutationCtrl.Value = str(perm)
        else:
            self.permutationCtrl.Value = hex(perm)
    
    
#f = ShaderWindow(r'c:\depot\games\branches\development\MAIN\eve\client\res\Graphics\Effect\Managed\Space\SpecialFX\Boosters\Booster.fx')
#f.Show()

if __name__ == '__main__':
    app = wx.App(0)
    redFileName = sys.argv[1]
    f = ShaderWindow(redFileName)
    f.Show()
    app.SetTopWindow(f)
    app.MainLoop()
