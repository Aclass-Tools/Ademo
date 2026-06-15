# -*- coding: utf-8 -*-
"""
协议编辑后端（Flask）。

职责：
  - 托管前端静态页（static/）。
  - 提供 protocol / field 的 REST CRUD。
  - 按数据库内容实时生成 .bin 供 Qt 下载（GET /api/protocols/<id>/bin）。

启动：python app.py  →  http://127.0.0.1:5000
"""

import os
import sys
from urllib.parse import quote

from flask import Flask, jsonify, request, send_from_directory, Response, abort

# 让本目录在 sys.path 最前（run_backend.pyw 场景下 cwd 可能不是 backend）。
_HERE = os.path.dirname(os.path.abspath(__file__))
if _HERE not in sys.path:
    sys.path.insert(0, _HERE)

import db  # noqa: E402
import models  # noqa: E402
import bin_writer  # noqa: E402

# 启动前确保数据库已建表并导入示例。
db.init_db(seed_sample=True)

app = Flask(__name__, static_folder="static", static_url_path="/static")


# --------------------------------------------------------------------------- #
# 前端入口
# --------------------------------------------------------------------------- #

@app.route("/")
def index():
    """根路径返回前端单页（static/index.html）。"""
    return send_from_directory(app.static_folder, "index.html")


@app.route("/api/health")
def health():
    return jsonify({"status": "ok", "service": "protocol-editor-backend"})


# --------------------------------------------------------------------------- #
# protocol CRUD
# --------------------------------------------------------------------------- #

@app.route("/api/protocols", methods=["GET"])
def protocols_list():
    return jsonify(models.list_protocols())


@app.route("/api/protocols", methods=["POST"])
def protocols_create():
    body = request.get_json(silent=True) or {}
    name = (body.get("name") or "").strip()
    if not name:
        return jsonify({"error": "name 不能为空"}), 400
    proto = models.create_protocol(
        name=name,
        version=int(body.get("version", 1)),
        flags=int(body.get("flags", 0)),
    )
    return jsonify(proto), 201


@app.route("/api/protocols/<int:pid>", methods=["GET"])
def protocols_get(pid: int):
    proto = models.get_protocol(pid)
    if not proto:
        abort(404)
    fields = models.get_fields(pid)
    return jsonify({**proto, "fields": fields})


@app.route("/api/protocols/<int:pid>", methods=["PUT"])
def protocols_update(pid: int):
    proto = models.get_protocol(pid)
    if not proto:
        abort(404)
    body = request.get_json(silent=True) or {}
    updated = models.update_protocol(
        pid,
        name=body.get("name"),
        version=body.get("version"),
        flags=body.get("flags"),
        transport=body.get("transport"),
    )
    return jsonify(updated)


@app.route("/api/protocols/<int:pid>", methods=["DELETE"])
def protocols_delete(pid: int):
    if not models.delete_protocol(pid):
        abort(404)
    return jsonify({"deleted": pid})


# --------------------------------------------------------------------------- #
# field 操作
# --------------------------------------------------------------------------- #

@app.route("/api/protocols/<int:pid>/fields", methods=["GET"])
def fields_get(pid: int):
    if not models.get_protocol(pid):
        abort(404)
    return jsonify(models.get_fields(pid))


@app.route("/api/protocols/<int:pid>/fields", methods=["PUT"])
def fields_replace(pid: int):
    """整体替换字段表。请求体为顶层字段数组（可含 children）。"""
    if not models.get_protocol(pid):
        abort(404)
    fields = request.get_json(silent=True)
    if not isinstance(fields, list):
        return jsonify({"error": "请求体应为字段数组"}), 400
    try:
        result = models.replace_fields(pid, fields)
    except ValueError as e:
        return jsonify({"error": str(e)}), 400
    return jsonify(result)


# --------------------------------------------------------------------------- #
# bin 生成与下载（核心接口）
# --------------------------------------------------------------------------- #

@app.route("/api/protocols/<int:pid>/bin", methods=["GET"])
def protocols_bin(pid: int):
    """按数据库当前内容生成 .bin 并返回。Qt 端用这个接口下载。"""
    proto = models.get_protocol(pid)
    if not proto:
        abort(404)
    fields = models.get_fields(pid)
    data = bin_writer.build_protocol_bin(proto, fields)

    # 文件名用协议名，做 URL 编码以兼容中文。
    filename = f"{proto['name']}.bin"
    disposition = f"attachment; filename=\"{filename}\"; filename*=UTF-8''{quote(filename)}"
    resp = Response(data, mimetype="application/octet-stream")
    resp.headers["Content-Disposition"] = disposition
    resp.headers["Content-Length"] = str(len(data))
    return resp


if __name__ == "__main__":
    # debug=False 适合日常使用；需要热重载时改 True。
    app.run(host="127.0.0.1", port=5000, debug=False)
