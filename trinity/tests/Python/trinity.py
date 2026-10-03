# Copyright © 2026 CCP ehf.

def _import_trinity():
    import blue
    import os

    triPlatform = os.getenv("TRINITYPLATFORM", "stub")
    flavor = os.getenv("TRINITYFLAVOR", "release")
    if flavor == "release":
        name = "_trinity_%s" % triPlatform
    else:
        name = "_trinity_%s_%s" % (triPlatform, flavor)
    mod = __import__(name)
    for memberName in dir(mod):
        globals()[memberName] = getattr(mod, memberName)
    globals()['platform'] = triPlatform

_import_trinity()
