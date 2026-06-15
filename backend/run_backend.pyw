# -*- coding: utf-8 -*-
"""
无控制台启动入口（.pyw 用 pythonw 运行，不弹黑窗）。
内部直接 import 并运行 app.py 的 Flask 服务。
适合做成开机/桌面快捷方式。

注意：.pyw 不会显示 print 输出，调试时请改用 run_backend.bat。
"""

import os
import sys

# 确保 backend 目录在 sys.path 里（双击运行时 cwd 可能不对）。
_here = os.path.dirname(os.path.abspath(__file__))
if _here not in sys.path:
    sys.path.insert(0, _here)

import app  # noqa: E402  侧效：执行 app.py 顶层代码（含 app.run）

# app.run() 是阻塞的，到这里说明服务已退出。
sys.exit(0)
