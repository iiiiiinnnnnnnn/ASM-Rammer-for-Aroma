@echo off
echo ================== �N���[���J�n ==================
make clean
if errorlevel 1 (
    echo �N���[���Ɏ��s���܂����B�����𒆒f���܂��B
    pause
    exit /b 1
) else (
    echo �N���[�������I
)

echo ==================  �r���h�J�n  ==================
make
echo ==================    �I��    ==================
if errorlevel 1 (
    echo �r���h�Ɏ��s���܂����B�G���[���e���m�F���Ă��������B
    pause
    exit /b 1
) else (
    echo �r���h�����I
    exit /b 0
)
