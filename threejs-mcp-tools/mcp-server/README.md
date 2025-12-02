# Three.js MCP Server

MCP服务器实现，允许AI通过Model Context Protocol调用Three.js的3D图形功能。

## 安装

```bash
cd mcp-server
npm install
npm run build
```

## 运行

```bash
npm start
```

## 开发

```bash
npm run dev  # 监听模式编译
```

## 项目结构

```
mcp-server/
├── src/
│   ├── index.ts              # 主服务器文件
│   ├── state/
│   │   └── store.ts          # 状态存储
│   ├── utils/
│   │   ├── id-manager.ts     # ID生成器
│   │   └── helpers.ts        # 辅助函数
│   └── tools/
│       ├── scene.ts          # 场景工具
│       ├── geometry.ts       # 几何体工具
│       ├── material.ts       # 材质工具
│       ├── object.ts         # 对象工具
│       ├── light.ts          # 光源工具
│       ├── camera.ts         # 相机工具
│       ├── renderer.ts       # 渲染器工具
│       └── texture.ts        # 纹理工具
├── dist/                     # 编译输出
├── package.json
├── tsconfig.json
└── README.md
```

## 配置MCP客户端

### Claude Desktop

在 `claude_desktop_config.json` 中添加：

```json
{
  "mcpServers": {
    "threejs": {
      "command": "node",
      "args": ["F:/three.js/mcp-server/dist/index.js"]
    }
  }
}
```

注意：请将路径替换为实际的项目路径。

## 功能

服务器实现了57个Three.js工具，包括：

- ✅ 场景管理（5个工具）
- ✅ 几何体创建（13个工具）
- ✅ 材质创建（8个工具）
- ✅ 对象操作（9个工具）
- ✅ 光源管理（9个工具）
- ✅ 相机控制（5个工具）
- ✅ 渲染功能（4个工具）
- ✅ 纹理处理（3个工具）

详见 [MCP_README.md](../MCP_README.md)

