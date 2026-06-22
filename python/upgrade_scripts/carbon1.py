# Copyright © 2024 CCP ehf.

"""
Upgrade script for resource files for Carbon 1.0. This script will remove the fidelityFX and useSpaceObjectData
attributes from all .red files in the given directory.
"""
import argparse
import os
import subprocess


class BaseFileUpgrade(object):
    def AppliesToFile(self, filename: str) -> bool:
        return False

    def UpgradeFile(self, filename: str):
        pass


def _GetIndent(line: str) -> int:
    return len(line) - len(line.lstrip())


def _RemoveAttribute(contents: str, attribute: str):
    found = contents.find(attribute + ':')
    if found == -1:
        raise StopIteration()
    start = contents.rfind('\n', 0, found)
    indent = _GetIndent(contents[start + 1:found])
    end = contents.find('\n', found)
    while _GetIndent(contents[end + 1:]) > indent:
        end = contents.find('\n', end + 1)
    return contents[:start + 1] + contents[end + 1:]


class RemoveAttribute(BaseFileUpgrade):
    def __init__(self, attribute: str):
        self.attribute = attribute

    def AppliesToFile(self, filename):
        if not filename.lower().endswith('.red'):
            return False
        with open(filename, 'r') as f:
            contents = f.read()
        return self.attribute in contents

    def UpgradeFile(self, filename):
        with open(filename, 'r') as f:
            contents = f.read()
        while True:
            try:
                contents = _RemoveAttribute(contents, self.attribute)
            except StopIteration:
                break
        with open(filename, 'w') as f:
            f.write(contents)

class ConvertRocks(BaseFileUpgrade):
    def AppliesToFile(self, filename):
        if not filename.lower().endswith('.red'):
            return False
        with open(filename, 'r') as f:
            contents = f.read()
        return "static/rocks.fx" in contents

    def UpgradeFile(self, filename):
        print("Please update '", filename, "' manually by removing the EveTransform (the type of this object should be Tr2InstancedMesh)")


def UpgradeFilesInDirectory(directory: str, upgrades: list):
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
    UpgradeFilesInDirectory(args.directory, [RemoveAttribute('fidelityFX'), RemoveAttribute('useSpaceObjectData'), ConvertRocks()])
