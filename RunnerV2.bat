@echo off
echo ================== クリーン開始 ==================
make clean
if errorlevel 1 (
    echo クリーンに失敗しました。処理を中断します。
    pause
    exit /b 1
) else (
    echo クリーン完了！
)

echo ==================  ビルド開始  ==================
make
echo ==================    終了    ==================
if errorlevel 1 (
    echo ビルドに失敗しました。エラー内容を確認してください。
    pause
    exit /b 1
) else (
    echo ビルド完了！
    exit /b 0
)
