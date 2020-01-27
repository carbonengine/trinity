@echo off 
setlocal
virtualenv > nul || pip install virtualenv
rd /s /q env 2> nul
virtualenv env
call env\Scripts\activate
pip install -r requirements.txt --timeout 45
rd /s /q build 2> nul
rd /s /q ..\fxinfo 2> nul
python setup.py py2exe
rd /s /q build
rd /s /q env
