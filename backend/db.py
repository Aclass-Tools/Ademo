# -*- coding: utf-8 -*-
"""
数据库连接与初始化。

使用 SQLite 单文件数据库（默认 backend/protocols.db）。
表结构：
  protocol: 协议元信息
  field:    字段表，通过 parent_field_id 自引用实现 Struct 一层嵌套子字段
"""

import os
import sqlite3

DEFAULT_DB_PATH = os.path.join(os.path.dirname(os.path.abspath(__file__)), "protocols.db")


def get_db_path() -> str:
    """数据库文件路径，可通过环境变量 PROTOCOLS_DB 覆盖。"""
    return os.environ.get("PROTOCOLS_DB", DEFAULT_DB_PATH)


def get_conn() -> sqlite3.Connection:
    """返回一个开启外键、row_factory=dict 的连接。"""
    conn = sqlite3.connect(get_db_path())
    conn.row_factory = sqlite3.Row
    conn.execute("PRAGMA foreign_keys = ON")
    return conn


def init_db(seed_sample: bool = True) -> None:
    """建表；若 seed_sample 为 True 且数据库为空，则导入示例协议。"""
    schema = """
    CREATE TABLE IF NOT EXISTS protocol (
        id         INTEGER PRIMARY KEY AUTOINCREMENT,
        name       TEXT    NOT NULL,
        version    INTEGER NOT NULL DEFAULT 1,
        flags      INTEGER NOT NULL DEFAULT 0,
        transport  TEXT    NOT NULL DEFAULT 'A0',  -- A0 / A3 / Modbus
        updated_at TEXT    NOT NULL
    );

    CREATE TABLE IF NOT EXISTS field (
        id               INTEGER PRIMARY KEY AUTOINCREMENT,
        protocol_id      INTEGER NOT NULL,
        parent_field_id  INTEGER,          -- NULL = 顶层字段；非空 = 该 Struct 字段的子字段（仅一层）
        seq              INTEGER NOT NULL, -- 同一父下的顺序，从 0 起
        name             TEXT    NOT NULL,
        type             TEXT    NOT NULL, -- FieldType 名：UInt8..Struct
        byte_offset      INTEGER NOT NULL DEFAULT 0,
        byte_length      INTEGER NOT NULL DEFAULT 0,
        bit_offset       INTEGER NOT NULL DEFAULT 0,
        bit_length       INTEGER NOT NULL DEFAULT 0,
        comment          TEXT    NOT NULL DEFAULT '',
        FOREIGN KEY (protocol_id)     REFERENCES protocol(id) ON DELETE CASCADE,
        FOREIGN KEY (parent_field_id) REFERENCES field(id)    ON DELETE CASCADE
    );

    CREATE INDEX IF NOT EXISTS idx_field_protocol ON field(protocol_id);
    CREATE INDEX IF NOT EXISTS idx_field_parent   ON field(parent_field_id);
    """
    with get_conn() as conn:
        conn.executescript(schema)

    if seed_sample:
        # 只在数据库为空时导入示例，避免重复导入。
        with get_conn() as conn:
            count = conn.execute("SELECT COUNT(*) AS c FROM protocol").fetchone()["c"]
        if count == 0:
            from init_db import seed_sample_protocol
            seed_sample_protocol()
