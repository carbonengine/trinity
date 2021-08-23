#!/usr/bin/env bash
if [ ! -f /usr/local/bin/python ]; then
    echo "Building script into an app on OSX requires a standalone Python installation"
    echo "Install Python 2.7 one from python.org"
    exit 1
fi

virtualenv --version > /dev/null || pip install virtualenv
rm -Rf env
virtualenv -p /usr/local/bin/python env
source env/bin/activate
pip install -r requirements-macos.txt
rm -Rf build
rm -Rf ../fxinfo/macOS
python setup.py py2app --packages=wx --excludes=numpy
rm -Rf build
rm -Rf env
