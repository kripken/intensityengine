SET OLD_PATH=%PATH%
SET PATH=windows\dll;Python25;C:\PYTHON25\;%PATH%

SET OLD_PYTHONHOME=%PYTHONHOME%
SET PYTHONHOME=Python25\lib;Python25\DLLs;%PYTHONHOME%

SET OLD_PYTHONPATH=%PYTHONPATH%
SET PYTHONPATH=Python25\lib;Python25\DLLs;%PYTHONPATH%

cbuild\src\client\Release\Intensity_CClient -r %* > out_client 2>&1

echo "(If a problem occurred, look in out_client)"

pause

SET PATH=%OLD_PATH%
SET PYTHONHOME=%OLD_PYTHONHOME%
SET PYTHONPATH=%OLD_PYTHONPATH%

