# Three.js MCP工具集

这是一个基于Model Context Protocol (MCP)的Three.js工具集，允许AI（如Claude/ChatGPT）通过MCP服务器调用Three.js的3D图形功能。

## 📋 项目概述

本项目将Three.js的核心3D图形功能封装为MCP工具，使AI能够：
- 创建和管理3D场景
- 创建各种几何体和材质
- 添加光源和相机
- 渲染场景并导出图像

## 🚀 快速开始

### 1. 安装依赖

```bash
npm install @modelcontextprotocol/sdk three uuid
npm install -D typescript @types/node
```

### 2. MCP工具定义

工具定义文件：`mcp-tools.json`

包含以下核心工具：

#### 场景管理
- `three_create_scene` - 创建场景
- `three_add_object_to_scene` - 添加对象到场景
- `three_remove_object_from_scene` - 从场景移除对象
- `three_get_scene_objects` - 获取场景对象列表

#### 几何体
- `three_create_box_geometry` - 创建立方体
- `three_create_sphere_geometry` - 创建球体
- `three_create_plane_geometry` - 创建平面
- `three_create_cylinder_geometry` - 创建圆柱体
- `three_create_cone_geometry` - 创建圆锥体

#### 材质
- `three_create_basic_material` - 创建基础材质
- `three_create_standard_material` - 创建标准PBR材质

#### 对象
- `three_create_mesh` - 创建网格对象
- `three_set_object_position` - 设置对象位置
- `three_set_object_rotation` - 设置对象旋转
- `three_set_object_scale` - 设置对象缩放
- `three_get_object_info` - 获取对象信息

#### 相机
- `three_create_perspective_camera` - 创建透视相机
- `three_create_orthographic_camera` - 创建正交相机

#### 渲染
- `three_create_renderer` - 创建渲染器
- `three_render_scene` - 渲染场景
- `three_get_renderer_canvas` - 获取渲染结果图像

## 📖 使用示例

### 示例1：创建一个简单的立方体场景

```json
// 1. 创建场景
{
  "tool": "three_create_scene",
  "arguments": {
    "background": "#87CEEB"
  }
}
// 返回: { "sceneId": "scene_abc123" }

// 2. 创建立方体几何体
{
  "tool": "three_create_box_geometry",
  "arguments": {
    "width": 1,
    "height": 1,
    "depth": 1
  }
}
// 返回: { "geometryId": "geometry_xyz789" }

// 3. 创建材质
{
  "tool": "three_create_basic_material",
  "arguments": {
    "color": "#ff0000"
  }
}
// 返回: { "materialId": "material_def456" }

// 4. 创建网格对象
{
  "tool": "three_create_mesh",
  "arguments": {
    "geometryId": "geometry_xyz789",
    "materialId": "material_def456",
    "position": { "x": 0, "y": 0, "z": 0 }
  }
}
// 返回: { "objectId": "object_ghi012" }

// 5. 添加对象到场景
{
  "tool": "three_add_object_to_scene",
  "arguments": {
    "sceneId": "scene_abc123",
    "objectId": "object_ghi012"
  }
}

// 6. 创建相机
{
  "tool": "three_create_perspective_camera",
  "arguments": {
    "fov": 75,
    "aspect": 1,
    "position": { "x": 0, "y": 0, "z": 5 },
    "lookAt": { "x": 0, "y": 0, "z": 0 }
  }
}
// 返回: { "cameraId": "camera_jkl345" }

// 7. 创建渲染器
{
  "tool": "three_create_renderer",
  "arguments": {
    "width": 800,
    "height": 600,
    "antialias": true
  }
}
// 返回: { "rendererId": "renderer_mno678" }

// 8. 渲染场景
{
  "tool": "three_render_scene",
  "arguments": {
    "rendererId": "renderer_mno678",
    "sceneId": "scene_abc123",
    "cameraId": "camera_jkl345"
  }
}

// 9. 获取渲染结果
{
  "tool": "three_get_renderer_canvas",
  "arguments": {
    "rendererId": "renderer_mno678"
  }
}
// 返回: { "image": "data:image/png;base64,..." }
```

### 示例2：创建多个对象

```json
// 创建多个几何体
{
  "tool": "three_create_sphere_geometry",
  "arguments": { "radius": 0.5 }
}
// 返回: { "geometryId": "geometry_sphere1" }

{
  "tool": "three_create_cylinder_geometry",
  "arguments": { "radiusTop": 0.3, "radiusBottom": 0.3, "height": 1 }
}
// 返回: { "geometryId": "geometry_cylinder1" }

// 创建多个材质
{
  "tool": "three_create_basic_material",
  "arguments": { "color": "#00ff00" }
}
// 返回: { "materialId": "material_green" }

{
  "tool": "three_create_basic_material",
  "arguments": { "color": "#0000ff" }
}
// 返回: { "materialId": "material_blue" }

// 创建多个网格对象并设置不同位置
{
  "tool": "three_create_mesh",
  "arguments": {
    "geometryId": "geometry_sphere1",
    "materialId": "material_green",
    "position": { "x": -2, "y": 0, "z": 0 }
  }
}
// 返回: { "objectId": "object_sphere1" }

{
  "tool": "three_create_mesh",
  "arguments": {
    "geometryId": "geometry_cylinder1",
    "materialId": "material_blue",
    "position": { "x": 2, "y": 0, "z": 0 }
  }
}
// 返回: { "objectId": "object_cylinder1" }
```

## 🏗️ MCP服务器实现

### 基础结构

```typescript
import { Server } from '@modelcontextprotocol/sdk/server/index.js';
import { StdioServerTransport } from '@modelcontextprotocol/sdk/server/stdio.js';
import * as THREE from 'three';
import { v4 as uuidv4 } from 'uuid';

// 状态存储
const scenes = new Map<string, THREE.Scene>();
const objects = new Map<string, THREE.Object3D>();
const geometries = new Map<string, THREE.BufferGeometry>();
const materials = new Map<string, THREE.Material>();
const cameras = new Map<string, THREE.Camera>();
const renderers = new Map<string, THREE.WebGLRenderer>();

// 创建MCP服务器
const server = new Server({
  name: 'threejs-mcp-server',
  version: '1.0.0',
}, {
  capabilities: {
    tools: {},
  },
});

// 注册工具
server.setRequestHandler(ListToolsRequestSchema, async () => {
  // 加载 mcp-tools.json 并返回工具列表
});

server.setRequestHandler(CallToolRequestSchema, async (request) => {
  const { name, arguments: args } = request.params;
  
  switch (name) {
    case 'three_create_scene':
      return handleCreateScene(args);
    case 'three_create_box_geometry':
      return handleCreateBoxGeometry(args);
    // ... 其他工具处理
  }
});

// 启动服务器
async function main() {
  const transport = new StdioServerTransport();
  await server.connect(transport);
}

main();
```

### 工具处理示例

```typescript
async function handleCreateScene(args: any) {
  const scene = new THREE.Scene();
  
  if (args.background) {
    scene.background = new THREE.Color(args.background);
  }
  
  if (args.fog) {
    if (args.fog.type === 'linear') {
      scene.fog = new THREE.Fog(
        new THREE.Color(args.fog.color),
        args.fog.near,
        args.fog.far
      );
    } else if (args.fog.type === 'exponential') {
      scene.fog = new THREE.FogExp2(
        new THREE.Color(args.fog.color),
        args.fog.far
      );
    }
  }
  
  const sceneId = `scene_${uuidv4()}`;
  scenes.set(sceneId, scene);
  
  return {
    content: [{
      type: 'text',
      text: JSON.stringify({ sceneId }),
    }],
  };
}

async function handleCreateBoxGeometry(args: any) {
  const geometry = new THREE.BoxGeometry(
    args.width || 1,
    args.height || 1,
    args.depth || 1,
    args.widthSegments || 1,
    args.heightSegments || 1,
    args.depthSegments || 1
  );
  
  const geometryId = `geometry_${uuidv4()}`;
  geometries.set(geometryId, geometry);
  
  return {
    content: [{
      type: 'text',
      text: JSON.stringify({ geometryId }),
    }],
  };
}
```

## 📝 工具列表（共57个工具）

### 场景管理工具（5个）
- ✅ `three_create_scene` - 创建场景
- ✅ `three_add_object_to_scene` - 添加对象
- ✅ `three_remove_object_from_scene` - 移除对象
- ✅ `three_get_scene_objects` - 获取场景对象列表
- ✅ `three_set_scene_background` - 设置场景背景

### 几何体工具（13个）
- ✅ `three_create_box_geometry` - 立方体
- ✅ `three_create_sphere_geometry` - 球体
- ✅ `three_create_plane_geometry` - 平面
- ✅ `three_create_cylinder_geometry` - 圆柱体
- ✅ `three_create_cone_geometry` - 圆锥体
- ✅ `three_create_torus_geometry` - 圆环
- ✅ `three_create_torus_knot_geometry` - 圆环结
- ✅ `three_create_capsule_geometry` - 胶囊体
- ✅ `three_create_circle_geometry` - 圆形
- ✅ `three_create_ring_geometry` - 环形
- ✅ `three_create_tetrahedron_geometry` - 四面体
- ✅ `three_create_octahedron_geometry` - 八面体
- ✅ `three_create_icosahedron_geometry` - 二十面体
- ✅ `three_create_dodecahedron_geometry` - 十二面体

### 材质工具（8个）
- ✅ `three_create_basic_material` - 基础材质
- ✅ `three_create_standard_material` - 标准PBR材质
- ✅ `three_create_physical_material` - 物理材质（高级PBR）
- ✅ `three_create_lambert_material` - Lambert材质
- ✅ `three_create_phong_material` - Phong材质
- ✅ `three_create_line_material` - 线材质
- ✅ `three_create_points_material` - 点材质
- ✅ `three_set_material_texture` - 设置材质纹理

### 对象工具（9个）
- ✅ `three_create_mesh` - 创建网格
- ✅ `three_create_line` - 创建线对象
- ✅ `three_create_points` - 创建点对象
- ✅ `three_set_object_position` - 设置位置
- ✅ `three_set_object_rotation` - 设置旋转
- ✅ `three_set_object_scale` - 设置缩放
- ✅ `three_get_object_info` - 获取对象信息
- ✅ `three_clone_object` - 克隆对象
- ✅ `three_dispose_object` - 释放对象资源

### 相机工具（5个）
- ✅ `three_create_perspective_camera` - 透视相机
- ✅ `three_create_orthographic_camera` - 正交相机
- ✅ `three_set_camera_position` - 设置相机位置
- ✅ `three_set_camera_look_at` - 设置相机朝向
- ✅ `three_set_camera_fov` - 设置相机视野

### 渲染工具（4个）
- ✅ `three_create_renderer` - 创建渲染器
- ✅ `three_render_scene` - 渲染场景
- ✅ `three_get_renderer_canvas` - 获取渲染结果
- ✅ `three_set_renderer_size` - 设置渲染器尺寸

### 光源工具（9个）
- ✅ `three_create_ambient_light` - 环境光
- ✅ `three_create_directional_light` - 方向光（平行光）
- ✅ `three_create_point_light` - 点光源
- ✅ `three_create_spot_light` - 聚光灯
- ✅ `three_create_hemisphere_light` - 半球光
- ✅ `three_create_rect_area_light` - 矩形区域光
- ✅ `three_set_light_color` - 设置光源颜色
- ✅ `three_set_light_intensity` - 设置光源强度
- ✅ `three_set_light_position` - 设置光源位置

### 纹理工具（3个）
- ✅ `three_load_texture` - 从URL加载纹理
- ✅ `three_create_data_texture` - 创建数据纹理
- ✅ `three_set_material_texture` - 为材质设置纹理

### 导出工具（1个）
- ✅ `three_export_scene_json` - 导出场景为JSON

### 动画工具（待实现）
- ⏳ `three_create_animation_mixer` - 创建动画混合器
- ⏳ `three_play_animation` - 播放动画
- ⏳ `three_stop_animation` - 停止动画

## 🔧 配置MCP客户端

### Claude Desktop配置

在 `claude_desktop_config.json` 中添加：

```json
{
  "mcpServers": {
    "threejs": {
      "command": "node",
      "args": ["path/to/mcp-threejs-server/dist/index.js"]
    }
  }
}
```

## 📚 文档

- [MCP工具开发计划](./MCP_TOOLS_PLAN.md) - 详细的开发计划和设计文档
- [工具定义文件](./mcp-tools.json) - 完整的工具JSON Schema定义

## 🤝 贡献

欢迎贡献！请查看开发计划文档了解待实现的功能。

## 📄 许可证

MIT License

