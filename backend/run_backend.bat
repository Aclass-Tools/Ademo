@echo off
REM 启动协议编辑后端（Flask + SQLite）。
REM 用法：双击或在命令行运行 backend\run_backend.bat
REM 首次运行会自动安装 Flask 并初始化数据库。

setlocal
cd /d "%~dp0"

REM 1) 确保 Flask 已安装（仅首次）。
python -c "import flask" 2>nul
if errorlevel 1 (
    echo [setup] 首次运行，正在安装 Flask ...
    python -m pip install -r requirements.txt
    if errorlevel 1 (
        echo [error] Flask 安装失败，请检查 pip 与网络。
        pause
        exit /b 1
    )
)

REM 2) 启动服务。app.py 内部会在启动前调用 db.init_db()。
echo [run] 协议编辑后端启动中：http://127.0.0.1:5000
python app.py

endlocal
