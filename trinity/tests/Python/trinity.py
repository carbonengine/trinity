# Copyright © 2026 CCP ehf.

def _import_trinity():
    import blue
    import os

    triPlatform = os.getenv("TRINITYPLATFORM", "stub")
    mod = blue.LoadExtension("_trinity_%s" % triPlatform)
    for memberName in dir(mod):
        globals()[memberName] = getattr(mod, memberName)
    globals()['platform'] = triPlatform

_import_trinity()
