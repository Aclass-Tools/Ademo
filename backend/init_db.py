# -*- coding: utf-8 -*-
"""
首次初始化：建库 + 导入一份示例协议。

- 可作为模块被 db.init_db(seed_sample=True) 调用（seed_sample_protocol）。
- 也可作为脚本独立运行：python init_db.py
"""

import sys

import db
import models


# 示例协议字段定义，与 scripts/generate_sample_bin.py 的 sample_protocol()
# 以及 BinProtocolLoader::makeSample() 一致，覆盖全部 7 种类型。
SAMPLE_FIELDS = [
    {"name": "frameId", "type": "UInt16", "byte_offset": 0, "byte_length": 2, "comment": ""},
    {"name": "deviceId", "type": "UInt32", "byte_offset": 2, "byte_length": 4, "comment": ""},
    {"name": "temperature", "type": "Float32", "byte_offset": 6, "byte_length": 4,
     "comment": "温度，单位 0.1℃"},
    {"name": "statusFlags", "type": "BitField", "byte_offset": 10, "byte_length": 1,
     "bit_offset": 0, "bit_length": 4, "comment": "低 4 位状态码"},
    {"name": "mac", "type": "ByteArray", "byte_offset": 11, "byte_length": 6,
     "comment": "MAC 地址"},
    {"name": "deviceName", "type": "String", "byte_offset": 17, "byte_length": 16,
     "comment": "设备名（UTF-8，定长 16 字节）"},
    {"name": "coord", "type": "Struct", "byte_offset": 33, "byte_length": 8,
     "comment": "2 维坐标", "children": [
         {"name": "x", "type": "Int32", "byte_offset": 0, "byte_length": 4, "comment": ""},
         {"name": "y", "type": "Int32", "byte_offset": 4, "byte_length": 4, "comment": ""},
     ]},
]


def seed_sample_protocol() -> dict:
    """导入示例协议（如已存在同名则跳过）。返回新建或已存在的协议 dict。"""
    # 避免重复导入：按名字查一次。
    with db.get_conn() as conn:
        row = conn.execute(
            "SELECT id FROM protocol WHERE name = ?", ("sample_protocol",)
        ).fetchone()
    if row:
        return models.get_protocol(row["id"])

    proto = models.create_protocol("sample_protocol", version=1, flags=0)
    models.replace_fields(proto["id"], SAMPLE_FIELDS)
    return proto


def main() -> int:
    db.init_db(seed_sample=False)
    proto = seed_sample_protocol()
    print(f"初始化完成：数据库 {db.get_db_path()}")
    print(f"示例协议 id={proto['id']} name={proto['name']}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
