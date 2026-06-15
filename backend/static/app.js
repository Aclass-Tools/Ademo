// 协议编辑器前端（Vue 3 全局构建）。
// 两个视图：协议列表（#/）、协议编辑器（#/edit/<id>）。
// 与后端 REST API 对接，字段语义与 Qt FieldType 一一对应。

const { createApp, reactive, computed, onMounted, watch } = Vue;

// 与 FieldType 枚举对应的类型表（顺序与 Qt 端 protocolfieldtablemodel 一致）。
const FIELD_TYPES = [
  "UInt8", "UInt16", "UInt32", "Int8", "Int16", "Int32",
  "Float32", "Float64", "ByteArray", "BitField", "String", "Struct",
];

// 简易 hash 路由：#/  与  #/edit/<id>
function parseRoute() {
  const h = location.hash.replace(/^#/, "");
  const m = h.match(/^\/edit\/(\d+)$/);
  if (m) return { name: "edit", id: Number(m[1]) };
  return { name: "list", id: null };
}

const api = {
  async listProtocols() {
    const r = await fetch("/api/protocols");
    return r.json();
  },
  async createProtocol(name) {
    const r = await fetch("/api/protocols", {
      method: "POST", headers: { "Content-Type": "application/json" },
      body: JSON.stringify({ name }),
    });
    if (!r.ok) throw new Error((await r.json()).error || "创建失败");
    return r.json();
  },
  async deleteProtocol(id) {
    const r = await fetch(`/api/protocols/${id}`, { method: "DELETE" });
    if (!r.ok) throw new Error("删除失败");
  },
  async getProtocol(id) {
    const r = await fetch(`/api/protocols/${id}`);
    if (!r.ok) throw new Error("加载失败");
    return r.json(); // { ...meta, fields: [...] }
  },
  async updateProtocol(id, patch) {
    const r = await fetch(`/api/protocols/${id}`, {
      method: "PUT", headers: { "Content-Type": "application/json" },
      body: JSON.stringify(patch),
    });
    if (!r.ok) throw new Error("保存元信息失败");
    return r.json();
  },
  async saveFields(id, fields) {
    const r = await fetch(`/api/protocols/${id}/fields`, {
      method: "PUT", headers: { "Content-Type": "application/json" },
      body: JSON.stringify(fields),
    });
    if (!r.ok) throw new Error((await r.json()).error || "保存字段失败");
    return r.json();
  },
  binUrl(id) { return `/api/protocols/${id}/bin`; },
};

const app = createApp({
  setup() {
    const route = reactive(parseRoute());
    window.addEventListener("hashchange", () => Object.assign(route, parseRoute()));

    // ---- 列表视图状态 ----
    const protocols = reactive([]);
    const listLoading = reactive({ v: false });
    const listMsg = reactive({ text: "", kind: "" });

    async function loadProtocols() {
      listLoading.v = true;
      listMsg.text = "";
      try {
        const data = await api.listProtocols();
        protocols.splice(0, protocols.length, ...data);
        if (data.length === 0) listMsg.text = "暂无协议，点击右上角新建。";
      } catch (e) {
        listMsg.text = "后端连接失败：" + e.message;
        listMsg.kind = "err";
      } finally {
        listLoading.v = false;
      }
    }

    async function createProtocol() {
      const name = prompt("协议名称：", "new_protocol");
      if (!name) return;
      try {
        const p = await api.createProtocol(name.trim());
        protocols.push(p);
        goEdit(p.id);
      } catch (e) { alert(e.message); }
    }

    async function deleteProtocol(p) {
      if (!confirm(`确定删除协议「${p.name}」？该操作不可撤销。`)) return;
      try {
        await api.deleteProtocol(p.id);
        const i = protocols.findIndex(x => x.id === p.id);
        if (i >= 0) protocols.splice(i, 1);
      } catch (e) { alert(e.message); }
    }

    function downloadBin(p) {
      // 直接触发浏览器下载。
      location.href = api.binUrl(p.id);
    }

    // ---- 编辑器视图状态 ----
    const editor = reactive({
      loading: false,
      saving: false,
      proto: null,     // { id, name, version, flags }
      fields: [],      // [{ name,type,byte_offset,..., children:[] }]
      msg: { text: "", kind: "" },
    });

    function blankField(type = "UInt8") {
      return {
        name: "field", type,
        byte_offset: 0, byte_length: typeSizeDefault(type),
        bit_offset: 0, bit_length: 0, comment: "",
        children: [],
      };
    }
    // 各类型默认字节长度，便于新建字段时给个合理初值。
    function typeSizeDefault(t) {
      return { UInt8: 1, UInt16: 2, UInt32: 4, Int8: 1, Int16: 2, Int32: 4,
               Float32: 4, Float64: 8, ByteArray: 1, BitField: 1, String: 16, Struct: 0 }[t] || 0;
    }

    function onTypeChange(f) {
      // 切到 Struct 时确保有 children；其它类型清空 children。
      if (f.type === "Struct" && f.children.length === 0) {
        f.children = [blankField("Int32"), blankField("Int32")];
        f.children[0].name = "x";
        f.children[1].name = "y";
      } else if (f.type !== "Struct") {
        f.children.splice(0, f.children.length);
      }
    }

    function addField() { editor.fields.push(blankField()); }
    function removeField(i) { editor.fields.splice(i, 1); }
    function moveField(i, dir) {
      const j = i + dir;
      if (j < 0 || j >= editor.fields.length) return;
      const tmp = editor.fields[i];
      editor.fields[i] = editor.fields[j];
      editor.fields[j] = tmp;
    }
    function addChild(f) { f.children.push(blankField("Int32")); }
    function removeChild(f, i) { f.children.splice(i, 1); }

    // 帧大小：顶层字段 byte_offset + byte_length 的最大值（与 BinProtocol::frameSize 一致）。
    const frameSize = computed(() => {
      let tail = 0;
      for (const f of editor.fields) {
        const end = Number(f.byte_offset || 0) + Number(f.byte_length || 0);
        if (end > tail) tail = end;
      }
      return tail;
    });

    async function loadEditor(id) {
      editor.loading = true;
      editor.msg = { text: "", kind: "" };
      try {
        const data = await api.getProtocol(id);
        editor.proto = { id: data.id, name: data.name, version: data.version, flags: data.flags, transport: data.transport || "A0" };
        // 后端返回的 fields 已含 children；补齐缺失字段，避免 v-model 报错。
        editor.fields.splice(0, editor.fields.length,
          ...(data.fields || []).map(normalizeField));
      } catch (e) {
        editor.msg = { text: "加载失败：" + e.message, kind: "err" };
      } finally {
        editor.loading = false;
      }
    }
    function normalizeField(f) {
      return {
        name: f.name || "field",
        type: f.type || "UInt8",
        byte_offset: Number(f.byte_offset || 0),
        byte_length: Number(f.byte_length || 0),
        bit_offset: Number(f.bit_offset || 0),
        bit_length: Number(f.bit_length || 0),
        comment: f.comment || "",
        children: (f.children || []).map(normalizeField),
      };
    }

    async function saveAll() {
      editor.saving = true;
      editor.msg = { text: "", kind: "" };
      try {
        await api.updateProtocol(editor.proto.id, {
          name: editor.proto.name,
          version: Number(editor.proto.version),
          flags: Number(editor.proto.flags),
          transport: editor.proto.transport || "A0",
        });
        // 提交给后端前剥离 children 之外的多余键（这里 children 是需要的）。
        const payload = editor.fields.map(f => ({
          name: f.name, type: f.type,
          byte_offset: Number(f.byte_offset), byte_length: Number(f.byte_length),
          bit_offset: Number(f.bit_offset), bit_length: Number(f.bit_length),
          comment: f.comment,
          children: (f.children || []).map(c => ({
            name: c.name, type: c.type,
            byte_offset: Number(c.byte_offset), byte_length: Number(c.byte_length),
            bit_offset: Number(c.bit_offset), bit_length: Number(c.bit_length),
            comment: c.comment,
          })),
        }));
        await api.saveFields(editor.proto.id, payload);
        editor.msg = { text: "已保存。", kind: "ok" };
      } catch (e) {
        editor.msg = { text: "保存失败：" + e.message, kind: "err" };
      } finally {
        editor.saving = false;
      }
    }

    function saveAndDownload() {
      // 先保存，再下载（确保 bin 是最新字段）。
      saveAll().then(() => {
        if (editor.msg.kind === "ok") location.href = api.binUrl(editor.proto.id);
      });
    }

    // 路由切换时加载数据。
    watch(() => route.name + "/" + route.id, () => {
      if (route.name === "edit" && route.id != null) loadEditor(route.id);
    });
    onMounted(() => {
      if (route.name === "list") loadProtocols();
      else if (route.name === "edit" && route.id != null) loadEditor(route.id);
    });
    // 返回列表时刷新一次，方便看到最新 updated_at。
    watch(() => route.name, (n) => { if (n === "list") loadProtocols(); });

    function goEdit(id) { location.hash = `/edit/${id}`; }
    function goList() { location.hash = "/"; }
    function fmtTime(s) { return s ? s.replace("T", " ") : ""; }

    return {
      route, protocols, listLoading, listMsg,
      loadProtocols, createProtocol, deleteProtocol, downloadBin, goEdit, goList, fmtTime,
      editor, FIELD_TYPES, frameSize,
      addField, removeField, moveField, addChild, removeChild, onTypeChange, typeSizeDefault,
      saveAll, saveAndDownload,
    };
  },

  template: /* html */ `
  <div class="app">
    <!-- 顶栏 -->
    <div class="topbar">
      <div>
        <h1>协议编辑器</h1>
        <div class="hint">Web 编辑 → SQLite 持久化 → 生成 .bin → Qt 加载</div>
      </div>
      <div class="toolbar">
        <button class="ghost" @click="goList" v-if="route.name==='edit'">← 返回列表</button>
        <button @click="createProtocol" v-if="route.name==='list'">+ 新建协议</button>
        <button class="ghost" @click="loadProtocols" v-if="route.name==='list'">刷新</button>
      </div>
    </div>

    <!-- ===== 列表视图 ===== -->
    <template v-if="route.name==='list'">
      <div class="banner info" v-if="listLoading.v"><span class="spinner"></span>正在连接后端…</div>
      <div class="banner err" v-else-if="listMsg.kind==='err'">{{ listMsg.text }}</div>

      <div class="proto-grid" v-if="protocols.length">
        <div class="proto-card" v-for="p in protocols" :key="p.id">
          <div class="name">{{ p.name }}</div>
          <div class="meta">id={{ p.id }} · v{{ p.version }} · 更新 {{ fmtTime(p.updated_at) }}</div>
          <div class="actions">
            <button class="sm" @click="goEdit(p.id)">编辑</button>
            <button class="sm ghost" @click="downloadBin(p)">下载 .bin</button>
            <button class="sm danger" @click="deleteProtocol(p)">删除</button>
          </div>
        </div>
      </div>
      <div class="empty" v-else-if="!listLoading.v && listMsg.kind!=='err'">{{ listMsg.text || '暂无协议' }}</div>
    </template>

    <!-- ===== 编辑器视图 ===== -->
    <template v-if="route.name==='edit'">
      <div class="banner info" v-if="editor.loading"><span class="spinner"></span>加载中…</div>
      <div class="banner ok" v-if="editor.msg.kind==='ok'">{{ editor.msg.text }}</div>
      <div class="banner err" v-if="editor.msg.kind==='err'">{{ editor.msg.text }}</div>

      <div class="editor" v-if="editor.proto">
        <!-- 元信息 -->
        <div class="section">
          <h2>协议元信息</h2>
          <div class="form-row">
            <label>名称</label><input v-model="editor.proto.name" />
            <label>version</label><input type="number" v-model.number="editor.proto.version" />
            <label>flags</label><input type="number" v-model.number="editor.proto.flags" />
            <label>传输协议</label>
            <select v-model="editor.proto.transport">
              <option value="A0">A0（CRC16）</option>
              <option value="A3">A3（0x0D0A 结尾）</option>
              <option value="Modbus">Modbus（TCP/RTU）</option>
            </select>
          </div>
        </div>

        <!-- 字段表 -->
        <div class="section">
          <h2>字段定义</h2>
          <table class="fields">
            <thead>
              <tr>
                <th class="col-num">序号</th>
                <th>名称</th>
                <th>类型</th>
                <th class="col-num">字节偏移</th>
                <th class="col-num">字节长度</th>
                <th class="col-num">位偏移</th>
                <th class="col-num">位长度</th>
                <th>备注</th>
                <th class="col-act">操作</th>
              </tr>
            </thead>
            <tbody>
              <template v-for="(f, i) in editor.fields" :key="i">
                <tr>
                  <td class="col-num">{{ i }}</td>
                  <td><input v-model="f.name" /></td>
                  <td>
                    <select v-model="f.type" @change="onTypeChange(f)">
                      <option v-for="t in FIELD_TYPES" :key="t" :value="t">{{ t }}</option>
                    </select>
                  </td>
                  <td><input type="number" v-model.number="f.byte_offset" /></td>
                  <td><input type="number" v-model.number="f.byte_length" /></td>
                  <td><input type="number" v-model.number="f.bit_offset" /></td>
                  <td><input type="number" v-model.number="f.bit_length" /></td>
                  <td><input v-model="f.comment" /></td>
                  <td class="col-act">
                    <button class="sm ghost" @click="moveField(i,-1)">↑</button>
                    <button class="sm ghost" @click="moveField(i,1)">↓</button>
                    <button class="sm danger" @click="removeField(i)">删</button>
                  </td>
                </tr>
                <!-- Struct 子字段（一层） -->
                <tr v-if="f.type==='Struct'" class="children-row">
                  <td colspan="9">
                    <div class="children-title">子字段（相对父字段偏移）</div>
                    <table class="fields">
                      <thead>
                        <tr>
                          <th class="col-num">#</th><th>名称</th><th>类型</th>
                          <th class="col-num">偏移</th><th class="col-num">长度</th>
                          <th class="col-num">位偏移</th><th class="col-num">位长度</th>
                          <th>备注</th><th class="col-act">操作</th>
                        </tr>
                      </thead>
                      <tbody>
                        <tr v-for="(c, ci) in f.children" :key="ci">
                          <td class="col-num">{{ ci }}</td>
                          <td><input v-model="c.name" /></td>
                          <td>
                            <select v-model="c.type">
                              <option v-for="t in FIELD_TYPES" v-if="t!=='Struct'" :key="t" :value="t">{{ t }}</option>
                            </select>
                          </td>
                          <td><input type="number" v-model.number="c.byte_offset" /></td>
                          <td><input type="number" v-model.number="c.byte_length" /></td>
                          <td><input type="number" v-model.number="c.bit_offset" /></td>
                          <td><input type="number" v-model.number="c.bit_length" /></td>
                          <td><input v-model="c.comment" /></td>
                          <td class="col-act">
                            <button class="sm danger" @click="removeChild(f,ci)">删</button>
                          </td>
                        </tr>
                      </tbody>
                    </table>
                    <div style="margin-top:6px;"><button class="sm ghost" @click="addChild(f)">+ 添加子字段</button></div>
                  </td>
                </tr>
              </template>
            </tbody>
          </table>
          <div style="margin-top:10px;">
            <button class="ghost" @click="addField">+ 添加字段</button>
          </div>
        </div>

        <div class="frame-info">帧大小：<b>{{ frameSize }}</b> 字节（取顶层字段末尾最大值，与 Qt 端 <code>BinProtocol::frameSize()</code> 一致）</div>

        <div class="toolbar">
          <button @click="saveAll" :disabled="editor.saving">{{ editor.saving ? '保存中…' : '保存' }}</button>
          <button @click="saveAndDownload" :disabled="editor.saving">保存并下载 .bin</button>
        </div>
      </div>
    </template>
  </div>
  `,
});

app.mount("#app");
