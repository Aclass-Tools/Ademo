# -*- coding: utf-8 -*-
"""
按 Qt 端 BinProtocolLoader 的二进制布局生成 .bin。

布局（全部小端，与 src/models/binprotocolformat.h + binprotocolloader.cpp 严格一致）：
  1) 头 24 字节：magic(8) + version(2) + flags(2) + fieldCount(4) + payloadOffset(4) + stringPoolOffset(4)
  2) 字段描述表：顶层字段后紧跟各 Struct 的子字段，每条 24 字节
     nameIdx(4) type(1) byteOffset(4) byteLength(2) bitOffset(1) bitLength(1)
     childCount(2) reserved(1) commentIdx(4) pad(4)
  3) 字符串池：count(4) + N × (len(2) + utf8 bytes)

本文件移植自 scripts/generate_sample_bin.py，输入改为协议 dict（含 fields/children），
方便 Flask 直接用 models.get_fields() 的结果调用。
"""

import struct

MAGIC = b"APROTO01"
# 头 24 字节：magic(8) version(2) flags(2) fieldCount(4) payloadOffset(4) stringPoolOffset(4)
HEADER_FMT = "<8sHHIII"
# 字段描述 24 字节（与 binprotocolloader.cpp 的 kFieldDescSize 对齐）：
#   nameIdx(4) type(1) byteOffset(4) byteLength(2) bitOffset(1) bitLength(1)
#   childCount(2) reserved(1) commentIdx(4) + pad(4) = 24
FIELD_FMT = "<IBIHBBHxI4x"

TYPE_MAP = {
    "UInt8": 0, "UInt16": 1, "UInt32": 2, "Int8": 3, "Int16": 4, "Int32": 5,
    "Float32": 6, "Float64": 7, "ByteArray": 8, "BitField": 9, "String": 10, "Struct": 11,
}


class StringPool:
    """字符串池：内容相同的字符串复用同一序号（与 Qt 端 StringPool 语义一致）。"""

    def __init__(self):
        self.entries: list[bytes] = []

    def add(self, s: str) -> int:
        b = s.encode("utf-8")
        if b in self.entries:
            return self.entries.index(b)
        self.entries.append(b)
        return len(self.entries) - 1


def _write_field(out: bytearray, field: dict, pool: StringPool) -> None:
    """写一个字段描述（含其子字段，子字段紧随其后）。"""
    name_idx = pool.add(field["name"])
    type_id = TYPE_MAP[field["type"]]
    children = field.get("children", []) or []
    comment_idx = pool.add(field.get("comment", "") or "")
    out += struct.pack(
        FIELD_FMT,
        name_idx, type_id,
        int(field.get("byte_offset", 0)), int(field.get("byte_length", 0)),
        int(field.get("bit_offset", 0)), int(field.get("bit_length", 0)),
        len(children), comment_idx,
    )
    for c in children:
        _write_field(out, c, pool)


def build_bin(version: int, flags: int, fields: list[dict]) -> bytes:
    """根据协议元信息 + 顶层字段（含 children）生成 .bin 字节串。"""
    pool = StringPool()

    field_table = bytearray()
    for f in fields:
        _write_field(field_table, f, pool)

    string_pool = bytearray()
    string_pool += struct.pack("<I", len(pool.entries))
    for s in pool.entries:
        string_pool += struct.pack("<H", len(s)) + s

    payload_offset = 24
    string_pool_offset = payload_offset + len(field_table)
    header = struct.pack(
        HEADER_FMT, MAGIC, version & 0xFFFF, flags & 0xFFFF,
        len(fields), payload_offset, string_pool_offset,
    )
    return bytes(header) + bytes(field_table) + bytes(string_pool)


def build_protocol_bin(protocol: dict, fields: list[dict]) -> bytes:
    """便捷入口：接受 models.get_protocol + models.get_fields 的结果。

    transport 编码进 flags 低 2 位（0=A0, 1=A3, 2=Modbus）。
    """
    transport_map = {"A0": 0, "A3": 1, "Modbus": 2}
    transport = protocol.get("transport", "A0")
    transport_bits = transport_map.get(transport, 0)
    # 清掉 flags 低 2 位再写入 transport。
    flags = (int(protocol.get("flags", 0)) & ~0x3) | (transport_bits & 0x3)
    return build_bin(
        version=int(protocol.get("version", 1)),
        flags=flags,
        fields=fields,
    )
