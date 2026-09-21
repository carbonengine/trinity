import contextlib
import shutil
import tempfile

import blue


@contextlib.contextmanager
def TempRes():
    tempdir = tempfile.mkdtemp()
    blue.paths.SetSearchPath('res', '%s;%s' % (tempdir, blue.paths.GetSearchPath('res')))
    try:
        yield
    finally:
        shutil.rmtree(tempdir)
