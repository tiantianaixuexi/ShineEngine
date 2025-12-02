# Three.js MCP Lite - 无依赖代码生成器

## 🎯 核心理念

**MCP工具 ≠ 必须依赖Three.js运行时**

我们提供了两种MCP实现方式：

### 🔴 完整版本（mcp-server/）
- **依赖**: Three.js库
- **功能**: 实际执行3D操作，返回渲染结果
- **适用**: 需要实时3D预览的专业应用

### 🟢 Lite版本（mcp-server-lite/）
- **依赖**: 无（纯代码生成）
- **功能**: 生成Three.js代码，返回代码片段
- **适用**: 学习、教育、代码生成

## 🚀 Lite版本优势

### ✅ 零依赖运行
```bash
# 只需安装MCP SDK
npm install @modelcontextprotocol/sdk uuid
# 无需安装Three.js！
```

### ✅ 生成可复用代码
```javascript
// AI生成的代码，直接复制到项目中使用
const geometry = new THREE.BoxGeometry(2, 1, 1);
const material = new THREE.MeshBasicMaterial({ color: 0xff0000 });
const mesh = new THREE.Mesh(geometry, material);
scene.add(mesh);
```

### ✅ 学习和教学友好
- 代码透明可见
- 可以逐步学习Three.js
- 支持代码修改和定制

## 📋 使用场景对比

| 场景 | 完整版本 | Lite版本 |
|------|----------|----------|
| **学习Three.js** | ❌（黑盒执行） | ✅（代码可见） |
| **快速原型** | ❌（需要完整环境） | ✅（纯代码） |
| **代码生成** | ❌（内部执行） | ✅（直接输出） |
| **专业3D应用** | ✅（实时渲染） | ❌（无渲染） |
| **教育教学** | ❌（复杂依赖） | ✅（轻量简单） |
| **资源受限** | ❌（大库依赖） | ✅（无依赖） |

## 🔄 工作流程对比

### 完整版本流程
```
AI请求 → MCP服务器 → Three.js执行 → 渲染结果 → 返回图像
```

### Lite版本流程
```
AI请求 → MCP服务器 → 生成代码 → 返回代码 → 用户复制使用
```

## 📁 项目结构

```
mcp-server-lite/
├── src/
│   ├── index.ts              # 主服务器（代码生成器）
│   ├── tools/
│   │   └── geometry-code.ts  # 几何体代码生成
│   └── utils/
│       └── id-manager.ts     # ID生成器
├── package.json              # 轻量依赖
├── tsconfig.json             # TypeScript配置
└── README.md                 # 详细说明
```

## 🎯 当前支持的功能

### ✅ 几何体代码生成
- 立方体（BoxGeometry）
- 球体（SphereGeometry）
- 平面（PlaneGeometry）
- 圆柱体（CylinderGeometry）
- 圆锥体（ConeGeometry）
- 圆环（TorusGeometry）

### ✅ 场景代码生成
- 完整场景设置代码
- 相机和光源配置
- 渲染器初始化
- 可选动画循环

## 🔧 安装和运行

```bash
# 进入lite版本目录
cd mcp-server-lite

# 安装依赖（只有MCP SDK）
npm install

# 构建
npm run build

# 运行
npm start
```

## 📝 配置MCP客户端

```json
{
  "mcpServers": {
    "threejs-lite": {
      "command": "node",
      "args": ["F:/three.js/mcp-server-lite/dist/index.js"]
    }
  }
}
```

## 🎨 使用示例

### 1. 生成几何体代码
```json
// 请求
{
  "tool": "three_create_box_geometry",
  "arguments": {
    "width": 2,
    "height": 1,
    "depth": 1
  }
}

// 响应
{
  "geometryId": "geometry_abc123",
  "code": "const geometry = new THREE.BoxGeometry(2, 1, 1, 1, 1, 1);",
  "description": "Box geometry creation code"
}
```

### 2. 生成完整场景
```json
// 请求
{
  "tool": "three_generate_scene_code",
  "arguments": {
    "includeSetup": true,
    "includeRenderer": true,
    "includeAnimation": false
  }
}

// 响应包含完整的Three.js场景代码
```

## 🔮 扩展计划

### 短期目标（1-2周）
- ✅ 完成几何体代码生成
- ⏳ 添加材质代码生成
- ⏳ 添加对象操作代码生成
- ⏳ 添加光源代码生成

### 长期目标（1个月）
- ⏳ 支持完整场景代码生成
- ⏳ 添加动画代码生成
- ⏳ 支持代码优化和格式化
- ⏳ 添加错误检查和验证

## 💡 设计哲学

### 🎯 为什么选择代码生成？
1. **学习价值**: 用户可以看到实际的Three.js代码
2. **灵活性**: 生成的代码可以修改和定制
3. **轻量级**: 无需庞大的3D库依赖
4. **可移植**: 代码可以在任何支持Three.js的环境中使用

### 🚀 为什么不直接执行？
1. **依赖复杂**: Three.js库较大，需要WebGL环境
2. **性能开销**: 服务器需要维护3D上下文
3. **学习障碍**: 用户看不到内部实现
4. **扩展限制**: 难以支持所有Three.js功能

## 🔍 与完整版本的关系

**它们不是竞争关系，而是互补关系：**

- **Lite版本** = 代码生成器，适合学习和原型
- **完整版本** = 3D运行时，适合专业应用

用户可以：
1. 先用Lite版本学习和生成代码
2. 再用完整版本进行实时3D开发
3. 在同一个项目中混合使用

## 🎉 总结

**MCP工具可以不依赖Three.js运行时！**

通过代码生成的方式，我们提供了：
- ✅ **零依赖**的轻量级MCP服务器
- ✅ **透明可学习**的Three.js代码生成
- ✅ **灵活可定制**的代码输出
- ✅ **学习友好**的开发体验

这证明了MCP协议的强大灵活性，可以根据不同需求提供多种实现方式。

