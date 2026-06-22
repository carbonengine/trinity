# Copyright © 2025 CCP ehf.

"""
Upgrade script for resource files for Trinity 2.1. This script adds the default bloom luminance value to all .red files
in the given directory.
"""
import argparse
import os
import subprocess
import blue
import trinity


class BaseFileUpgrade(object):
    def AppliesToFile(self, filename):
        return False

    def UpgradeFile(self, filename):
        pass


class InjectLuminanceThreshold(BaseFileUpgrade):
    def AppliesToFile(self, filename):
        if not filename.lower().endswith('.red'):
            return False
        with open(filename, 'r') as f:
            contents = f.read()
        return "Tr2PPBloomEffect" in contents

    def UpgradeFile(self, filename):
        obj = blue.resMan.LoadObject(filename)
        modified = False
        for bloom in blue.FindInterface(obj, 'Tr2PPBloomEffect'):
            # The new default for luminanceThreshold is -1.0, so we need to restore the previous default of 0.0
            if bloom.luminanceThreshold == -1.0:
                bloom.luminanceThreshold = 0.0
                modified = True
        if modified:
            blue.resMan.SaveObject(obj, filename)


def UpgradeFilesInDirectory(directory, upgrades):
    toUpgrade = []
    for root, _, files in os.walk(directory):
        for file in files:
            filePath = os.path.join(root, file)
            upgradesForFile = []
            for upgrade in upgrades:
                if upgrade.AppliesToFile(filePath):
                    upgradesForFile.append(upgrade)
            if upgradesForFile:
                toUpgrade.append((filePath, upgradesForFile))
    if not toUpgrade:
        return
    p = subprocess.Popen(['p4', '-b', str(len(toUpgrade) + 1), '-x', '-', 'edit'], cwd=directory, stdin=subprocess.PIPE)
    _ = p.communicate('\n'.join(x[0] for x in toUpgrade).encode())
    for file, upgrades in toUpgrade:
        print('Upgrading', file)
        for upgrade in upgrades:
            upgrade.UpgradeFile(file)


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('directory')
    args = parser.parse_args()
    UpgradeFilesInDirectory(args.directory, [InjectLuminanceThreshold()])
