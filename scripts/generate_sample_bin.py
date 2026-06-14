# -*- coding: utf-8 -*-
"""
生成示例协议 .bin 文件，与 Qt 端 BinProtocolLoader 的二进制布局完全一致。

文件布局（全部小端）：
  1) 头 24 字节：magic(8) + version(2) + flags(2) + fieldCount(4) + payloadOffset(4) + reserved(4)
  2) 字段描述表：fieldCount × 24 字节
     每个：nameIdx(4) + type(1) + byteOffset(4) + byteLength(2)
           + bitOffset(1) + bitLength(1) + childCount(2) + reserved(1) + commentIdx(4)
     子字段（Struct）紧随其后，每个也是 24 字节。
  3) 字符串池：count(4) + N × (len(2) + bytes)

用法：python generate_sample_bin.py [输出路径]
默认输出到 data/protocols/sample_protocol.bin
"""
import os
import struct
import sys

MAGIC = b"APROTO01"
# header 24 字节：magic(8) version(2) flags(2) fieldCount(4) payloadOffset(4) stringPoolOffset(4)
HEADER_FMT = "<8sHHIII"
# 字段描述 24 字节（与 loader.cpp 的 kFieldDescSize 对齐）：
#   nameIdx(4) type(1) byteOffset(4) byteLength(2) bitOffset(1) bitLength(1)
#   childCount(2) reserved(1) commentIdx(4) + pad(4) = 24
FIELD_FMT = "<IBIHBBHxI4x"


class StringPool:
    def __init__(self):
        self.entries = []

    def add(self, s):
        b = s.encode("utf-8")
        if b in self.entries:
            return self.entries.index(b)
        self.entries.append(b)
        return len(self.entries) - 1


TYPE_MAP = {
    "UInt8": 0, "UInt16": 1, "UInt32": 2, "Int8": 3, "Int16": 4, "Int32": 5,
    "Float32": 6, "Float64": 7, "ByteArray": 8, "BitField": 9, "String": 10, "Struct": 11,
}


def write_field(out, field, pool):
    """field: dict with name/type/byteOffset/byteLength/bitOffset/bitLength/comment/children"""
    name_idx = pool.add(field["name"])
    type_id = TYPE_MAP[field["type"]]
    children = field.get("children", [])
    comment_idx = pool.add(field.get("comment", ""))
    out += struct.pack(
        FIELD_FMT,
        name_idx, type_id, field["byteOffset"], field["byteLength"],
        field.get("bitOffset", 0), field.get("bitLength", 0),
        len(children), comment_idx,
    )
    for c in children:
        write_field(out, c, pool)


def build_bin(fields):
    pool = StringPool()

    field_table = bytearray()
    for f in fields:
        write_field(field_table, f, pool)

    string_pool = bytearray()
    string_pool += struct.pack("<I", len(pool.entries))
    for s in pool.entries:
        string_pool += struct.pack("<H", len(s)) + s

    payload_offset = 24
    string_pool_offset = payload_offset + len(field_table)
    header = struct.pack(
        HEADER_FMT, MAGIC, 1, 0, len(fields), payload_offset, string_pool_offset,
    )
    return bytes(header) + bytes(field_table) + bytes(string_pool)


def sample_protocol():
    """覆盖各类型的示例协议：一个设备状态帧（与 BinProtocolLoader::makeSample 对应）。"""
    return [
        {"name": "frameId", "type": "UInt16", "byteOffset": 0, "byteLength": 2, "comment": ""},
        {"name": "deviceId", "type": "UInt32", "byteOffset": 2, "byteLength": 4, "comment": ""},
        {"name": "temperature", "type": "Float32", "byteOffset": 6, "byteLength": 4,
         "comment": "温度，单位 0.1℃"},
        {"name": "statusFlags", "type": "BitField", "byteOffset": 10, "byteLength": 1,
         "bitOffset": 0, "bitLength": 4, "comment": "低 4 位状态码"},
        {"name": "mac", "type": "ByteArray", "byteOffset": 11, "byteLength": 6,
         "comment": "MAC 地址"},
        {"name": "deviceName", "type": "String", "byteOffset": 17, "byteLength": 16,
         "comment": "设备名（UTF-8，定长 16 字节）"},
        {"name": "coord", "type": "Struct", "byteOffset": 33, "byteLength": 8,
         "comment": "2 维坐标", "children": [
             {"name": "x", "type": "Int32", "byteOffset": 0, "byteLength": 4, "comment": ""},
             {"name": "y", "type": "Int32", "byteOffset": 4, "byteLength": 4, "comment": ""},
         ]},
    ]


def main():
    out_path = sys.argv[1] if len(sys.argv) > 1 else None
    if not out_path:
        here = os.path.dirname(os.path.abspath(__file__))
        out_path = os.path.join(here, "..", "data", "protocols", "sample_protocol.bin")
        out_path = os.path.normpath(out_path)

    os.makedirs(os.path.dirname(out_path), exist_ok=True)
    data = build_bin(sample_protocol())
    with open(out_path, "wb") as f:
        f.write(data)
    print("written:", out_path, "size:", len(data))


if __name__ == "__main__":
    main()
