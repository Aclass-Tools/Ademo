# -*- coding: utf-8 -*-
"""
数据访问层：protocol / field 的增删改查。

返回值统一为 dict / list[dict]，字段命名与 .bin 格式对应：
  name, type, byte_offset, byte_length, bit_offset, bit_length, comment, children
子字段以 children 列表挂在 Struct 字段下（仅一层）。
"""

import sqlite3
from datetime import datetime
from typing import Optional

from db import get_conn


# 与 FieldType 枚举一致，供校验用。
VALID_TYPES = {
    "UInt8", "UInt16", "UInt32", "Int8", "Int16", "Int32",
    "Float32", "Float64", "ByteArray", "BitField", "String", "Struct",
}


def _now() -> str:
    return datetime.now().isoformat(timespec="seconds")


# ---------- protocol ----------

def list_protocols() -> list[dict]:
    with get_conn() as conn:
        rows = conn.execute(
            "SELECT id, name, version, flags, transport, updated_at FROM protocol ORDER BY id"
        ).fetchall()
    return [dict(r) for r in rows]


def get_protocol(protocol_id: int) -> Optional[dict]:
    with get_conn() as conn:
        row = conn.execute(
            "SELECT id, name, version, flags, transport, updated_at FROM protocol WHERE id = ?",
            (protocol_id,),
        ).fetchone()
    return dict(row) if row else None


def create_protocol(name: str, version: int = 1, flags: int = 0,
                    transport: str = "A0") -> dict:
    with get_conn() as conn:
        cur = conn.execute(
            "INSERT INTO protocol (name, version, flags, transport, updated_at) VALUES (?, ?, ?, ?, ?)",
            (name, version, flags, transport, _now()),
        )
        protocol_id = cur.lastrowid
    return get_protocol(protocol_id)


def update_protocol(protocol_id: int, name: Optional[str] = None,
                    version: Optional[int] = None, flags: Optional[int] = None,
                    transport: Optional[str] = None) -> Optional[dict]:
    existing = get_protocol(protocol_id)
    if not existing:
        return None
    new_name = existing["name"] if name is None else name
    new_version = existing["version"] if version is None else version
    new_flags = existing["flags"] if flags is None else flags
    new_transport = existing["transport"] if transport is None else transport
    with get_conn() as conn:
        conn.execute(
            "UPDATE protocol SET name = ?, version = ?, flags = ?, transport = ?, updated_at = ? WHERE id = ?",
            (new_name, new_version, new_flags, new_transport, _now(), protocol_id),
        )
    return get_protocol(protocol_id)


def delete_protocol(protocol_id: int) -> bool:
    with get_conn() as conn:
        cur = conn.execute("DELETE FROM protocol WHERE id = ?", (protocol_id,))
        return cur.rowcount > 0


# ---------- field ----------

def _row_to_field(row: sqlite3.Row) -> dict:
    """把数据库行映射成对外字段 dict（snake_case -> 业务命名）。"""
    return {
        "id": row["id"],
        "name": row["name"],
        "type": row["type"],
        "byte_offset": row["byte_offset"],
        "byte_length": row["byte_length"],
        "bit_offset": row["bit_offset"],
        "bit_length": row["bit_length"],
        "comment": row["comment"],
        "seq": row["seq"],
    }


def get_fields(protocol_id: int) -> list[dict]:
    """返回协议的顶层字段列表，Struct 字段带 children（一层）。"""
    with get_conn() as conn:
        tops = conn.execute(
            "SELECT * FROM field WHERE protocol_id = ? AND parent_field_id IS NULL "
            "ORDER BY seq",
            (protocol_id,),
        ).fetchall()
        children = conn.execute(
            "SELECT * FROM field WHERE protocol_id = ? AND parent_field_id IS NOT NULL "
            "ORDER BY parent_field_id, seq",
            (protocol_id,),
        ).fetchall()

    # 按父分组。
    by_parent: dict[int, list[dict]] = {}
    for c in children:
        by_parent.setdefault(c["parent_field_id"], []).append(_row_to_field(c))

    result = []
    for t in tops:
        f = _row_to_field(t)
        f["children"] = by_parent.get(t["id"], [])
        result.append(f)
    return result


def replace_fields(protocol_id: int, fields: list[dict]) -> list[dict]:
    """整体替换协议的字段表（含子字段）。

    fields: 顶层字段数组，每项可含 children（仅 Struct 有效，一层）。
    校验：type 必须在 VALID_TYPES 内。
    事务内完成：先删旧、再插新，保证原子性。
    """
    # 先做整体校验，任何一项不合法都拒绝，避免删了一半。
    def _validate(f: dict, is_child: bool) -> None:
        if not f.get("name"):
            raise ValueError("字段 name 不能为空")
        t = f.get("type")
        if t not in VALID_TYPES:
            raise ValueError(f"非法字段类型：{t}")
        if is_child and t == "Struct":
            raise ValueError("子字段不支持 Struct（仅支持一层嵌套）")

    for f in fields:
        _validate(f, is_child=False)
        for c in f.get("children", []):
            _validate(c, is_child=True)

    # SQLite 的 ? 占位符不直接接受 keyword，逐字段读取。
    with get_conn() as conn:
        # 外键级联删除：删顶层会把子字段一起带走；这里显式全删更稳妥。
        conn.execute("DELETE FROM field WHERE protocol_id = ?", (protocol_id,))

        for seq, f in enumerate(fields):
            cur = conn.execute(
                "INSERT INTO field "
                "(protocol_id, parent_field_id, seq, name, type, byte_offset, "
                " byte_length, bit_offset, bit_length, comment) "
                "VALUES (?, NULL, ?, ?, ?, ?, ?, ?, ?, ?)",
                (protocol_id, seq, f["name"], f["type"],
                 int(f.get("byte_offset", 0)), int(f.get("byte_length", 0)),
                 int(f.get("bit_offset", 0)), int(f.get("bit_length", 0)),
                 f.get("comment", "")),
            )
            parent_id = cur.lastrowid
            for cseq, c in enumerate(f.get("children", [])):
                conn.execute(
                    "INSERT INTO field "
                    "(protocol_id, parent_field_id, seq, name, type, byte_offset, "
                    " byte_length, bit_offset, bit_length, comment) "
                    "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?)",
                    (protocol_id, parent_id, cseq, c["name"], c["type"],
                     int(c.get("byte_offset", 0)), int(c.get("byte_length", 0)),
                     int(c.get("bit_offset", 0)), int(c.get("bit_length", 0)),
                     c.get("comment", "")),
                )

        conn.execute(
            "UPDATE protocol SET updated_at = ? WHERE id = ?",
            (_now(), protocol_id),
        )

    return get_fields(protocol_id)
