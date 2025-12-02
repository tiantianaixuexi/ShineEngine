# Three.js MCP工具开发计划

## 一、项目概述

### 1.1 项目目标
构建一个MCP（Model Context Protocol）工具集，使AI（如Claude/ChatGPT）能够通过MCP服务器调用Three.js的3D图形功能。

### 1.2 核心功能分析

基于Three.js源代码分析，核心功能模块包括：

#### 1. 场景管理（Scene Management）
- `Scene` - 创建和管理3D场景
- `Object3D` - 场景图基类，所有对象的父类
- `Group` - 对象组，用于组织场景图

#### 2. 几何体（Geometries）
- `BoxGeometry` - 立方体
- `SphereGeometry` - 球体
- `PlaneGeometry` - 平面
- `CylinderGeometry` - 圆柱体
- `ConeGeometry` - 圆锥体
- `TorusGeometry` - 圆环
- `TorusKnotGeometry` - 圆环结
- `CapsuleGeometry` - 胶囊体
- `CircleGeometry` - 圆形
- `RingGeometry` - 环形
- `TetrahedronGeometry` - 四面体
- `OctahedronGeometry` - 八面体
- `IcosahedronGeometry` - 二十面体
- `DodecahedronGeometry` - 十二面体
- `ExtrudeGeometry` - 挤压几何体
- `LatheGeometry` - 车削几何体
- `ShapeGeometry` - 形状几何体
- `TubeGeometry` - 管道几何体
- `WireframeGeometry` - 线框几何体
- `EdgesGeometry` - 边缘几何体
- `PolyhedronGeometry` - 多面体几何体
- `BufferGeometry` - 自定义缓冲区几何体

#### 3. 材质（Materials）
- `MeshBasicMaterial` - 基础材质
- `MeshStandardMaterial` - 标准PBR材质
- `MeshPhysicalMaterial` - 物理材质
- `MeshLambertMaterial` - Lambert材质
- `MeshPhongMaterial` - Phong材质
- `MeshToonMaterial` - 卡通材质
- `MeshNormalMaterial` - 法线材质
- `MeshMatcapMaterial` - Matcap材质
- `MeshDepthMaterial` - 深度材质
- `MeshDistanceMaterial` - 距离材质
- `LineBasicMaterial` - 基础线材质
- `LineDashedMaterial` - 虚线材质
- `PointsMaterial` - 点材质
- `SpriteMaterial` - 精灵材质
- `ShadowMaterial` - 阴影材质
- `RawShaderMaterial` - 原始着色器材质
- `ShaderMaterial` - 着色器材质

#### 4. 对象（Objects）
- `Mesh` - 网格对象（几何体+材质）
- `Line` - 线对象
- `LineSegments` - 线段对象
- `LineLoop` - 闭合线对象
- `Points` - 点对象
- `Sprite` - 精灵对象
- `Group` - 组对象
- `LOD` - 细节层次对象
- `SkinnedMesh` - 蒙皮网格
- `InstancedMesh` - 实例化网格
- `BatchedMesh` - 批处理网格

#### 5. 光源（Lights）
- `AmbientLight` - 环境光
- `DirectionalLight` - 方向光（平行光）
- `PointLight` - 点光源
- `SpotLight` - 聚光灯
- `RectAreaLight` - 矩形区域光
- `HemisphereLight` - 半球光
- `LightProbe` - 光照探针

#### 6. 相机（Cameras）
- `PerspectiveCamera` - 透视相机
- `OrthographicCamera` - 正交相机
- `CubeCamera` - 立方体相机
- `ArrayCamera` - 阵列相机
- `StereoCamera` - 立体相机

#### 7. 渲染器（Renderers）
- `WebGLRenderer` - WebGL渲染器
- `WebGPURenderer` - WebGPU渲染器（如果可用）

#### 8. 纹理（Textures）
- `Texture` - 基础纹理
- `DataTexture` - 数据纹理
- `CanvasTexture` - 画布纹理
- `VideoTexture` - 视频纹理
- `CubeTexture` - 立方体纹理
- `CompressedTexture` - 压缩纹理

#### 9. 动画（Animation）
- `AnimationMixer` - 动画混合器
- `AnimationClip` - 动画片段
- `AnimationAction` - 动画动作
- `KeyframeTrack` - 关键帧轨道

#### 10. 加载器（Loaders）
- `TextureLoader` - 纹理加载器
- `ObjectLoader` - 对象加载器
- `BufferGeometryLoader` - 几何体加载器
- `MaterialLoader` - 材质加载器
- `AnimationLoader` - 动画加载器
- `ImageLoader` - 图像加载器
- `AudioLoader` - 音频加载器

---

## 二、MCP工具设计

### 2.1 工具分类

将Three.js功能封装为以下MCP工具类别：

#### 类别1：场景管理工具（Scene Management）
1. `three_create_scene` - 创建新场景
2. `three_set_scene_background` - 设置场景背景
3. `three_set_scene_fog` - 设置场景雾效
4. `three_add_object_to_scene` - 向场景添加对象
5. `three_remove_object_from_scene` - 从场景移除对象
6. `three_get_scene_objects` - 获取场景中的所有对象

#### 类别2：几何体工具（Geometry）
7. `three_create_box_geometry` - 创建立方体几何体
8. `three_create_sphere_geometry` - 创建球体几何体
9. `three_create_plane_geometry` - 创建平面几何体
10. `three_create_cylinder_geometry` - 创建圆柱体几何体
11. `three_create_cone_geometry` - 创建圆锥体几何体
12. `three_create_torus_geometry` - 创建圆环几何体
13. `three_create_capsule_geometry` - 创建胶囊体几何体
14. `three_create_circle_geometry` - 创建圆形几何体
15. `three_create_ring_geometry` - 创建环形几何体
16. `three_create_polyhedron_geometry` - 创建多面体几何体

#### 类别3：材质工具（Material）
17. `three_create_basic_material` - 创建基础材质
18. `three_create_standard_material` - 创建标准PBR材质
19. `three_create_physical_material` - 创建物理材质
20. `three_set_material_color` - 设置材质颜色
21. `three_set_material_texture` - 设置材质纹理
22. `three_set_material_property` - 设置材质属性

#### 类别4：对象工具（Objects）
23. `three_create_mesh` - 创建网格对象
24. `three_create_line` - 创建线对象
25. `three_create_points` - 创建点对象
26. `three_set_object_position` - 设置对象位置
27. `three_set_object_rotation` - 设置对象旋转
28. `three_set_object_scale` - 设置对象缩放
29. `three_set_object_visible` - 设置对象可见性

#### 类别5：光源工具（Lights）
30. `three_create_ambient_light` - 创建环境光
31. `three_create_directional_light` - 创建方向光
32. `three_create_point_light` - 创建点光源
33. `three_create_spot_light` - 创建聚光灯
34. `three_set_light_color` - 设置光源颜色
35. `three_set_light_intensity` - 设置光源强度
36. `three_set_light_position` - 设置光源位置

#### 类别6：相机工具（Cameras）
37. `three_create_perspective_camera` - 创建透视相机
38. `three_create_orthographic_camera` - 创建正交相机
39. `three_set_camera_position` - 设置相机位置
40. `three_set_camera_look_at` - 设置相机朝向
41. `three_set_camera_fov` - 设置相机视野

#### 类别7：渲染工具（Rendering）
42. `three_create_renderer` - 创建渲染器
43. `three_render_scene` - 渲染场景
44. `three_set_renderer_size` - 设置渲染器尺寸
45. `three_get_renderer_canvas` - 获取渲染器画布
46. `three_export_scene_image` - 导出场景图像

#### 类别8：纹理工具（Textures）
47. `three_load_texture` - 加载纹理
48. `three_create_data_texture` - 创建数据纹理
49. `three_set_texture_wrap` - 设置纹理包裹模式
50. `three_set_texture_filter` - 设置纹理过滤模式

#### 类别9：动画工具（Animation）
51. `three_create_animation_mixer` - 创建动画混合器
52. `three_create_animation_clip` - 创建动画片段
53. `three_play_animation` - 播放动画
54. `three_stop_animation` - 停止动画
55. `three_update_animation` - 更新动画

#### 类别10：工具工具（Utilities）
56. `three_get_object_info` - 获取对象信息
57. `three_clone_object` - 克隆对象
58. `three_dispose_object` - 释放对象资源
59. `three_export_scene_json` - 导出场景JSON
60. `three_import_scene_json` - 导入场景JSON

---

## 三、MCP工具接口设计

### 3.1 工具定义格式

每个MCP工具遵循以下JSON Schema格式：

```json
{
  "name": "tool_name",
  "description": "工具描述（中英文）",
  "inputSchema": {
    "type": "object",
    "properties": {
      "param1": {
        "type": "string|number|boolean|object|array",
        "description": "参数描述"
      }
    },
    "required": ["param1"]
  }
}
```

### 3.2 对象引用系统

由于MCP工具是无状态的，需要设计对象引用系统：

- **场景ID**: `scene_{uuid}` - 场景的唯一标识
- **对象ID**: `object_{uuid}` - 对象的唯一标识
- **几何体ID**: `geometry_{uuid}` - 几何体的唯一标识
- **材质ID**: `material_{uuid}` - 材质的唯一标识
- **光源ID**: `light_{uuid}` - 光源的唯一标识
- **相机ID**: `camera_{uuid}` - 相机的唯一标识
- **渲染器ID**: `renderer_{uuid}` - 渲染器的唯一标识

### 3.3 状态管理

MCP服务器需要维护一个状态存储：
- 场景存储：`scenes: Map<sceneId, Scene>`
- 对象存储：`objects: Map<objectId, Object3D>`
- 几何体存储：`geometries: Map<geometryId, Geometry>`
- 材质存储：`materials: Map<materialId, Material>`
- 渲染器存储：`renderers: Map<rendererId, Renderer>`

---

## 四、分阶段实现计划

### 阶段一：核心场景管理（MVP）
**目标**: 实现最基本的场景创建和对象管理功能

**工具列表**:
1. `three_create_scene` ✅
2. `three_create_box_geometry` ✅
3. `three_create_basic_material` ✅
4. `three_create_mesh` ✅
5. `three_add_object_to_scene` ✅
6. `three_create_perspective_camera` ✅
7. `three_create_renderer` ✅
8. `three_render_scene` ✅

**预计时间**: 4-6小时

### 阶段二：扩展几何体和材质
**目标**: 添加更多几何体类型和材质类型

**工具列表**:
- 所有几何体创建工具
- 所有材质创建工具
- 材质属性设置工具

**预计时间**: 6-8小时

### 阶段三：光源和相机
**目标**: 完善光照和相机系统

**工具列表**:
- 所有光源创建工具
- 所有相机创建工具
- 光源和相机属性设置工具

**预计时间**: 4-6小时

### 阶段四：高级功能
**目标**: 添加纹理、动画、加载器等高级功能

**工具列表**:
- 纹理相关工具
- 动画相关工具
- 加载器工具
- 导出/导入工具

**预计时间**: 8-10小时

---

## 五、技术实现

### 5.1 MCP服务器结构

```
mcp-threejs/
├── src/
│   ├── server.ts              # MCP服务器主文件
│   ├── tools/                 # 工具实现
│   │   ├── scene.ts           # 场景管理工具
│   │   ├── geometry.ts        # 几何体工具
│   │   ├── material.ts        # 材质工具
│   │   ├── object.ts          # 对象工具
│   │   ├── light.ts           # 光源工具
│   │   ├── camera.ts          # 相机工具
│   │   ├── renderer.ts        # 渲染器工具
│   │   ├── texture.ts         # 纹理工具
│   │   └── animation.ts       # 动画工具
│   ├── state/                 # 状态管理
│   │   └── store.ts           # 状态存储
│   └── utils/                 # 工具函数
│       └── id-manager.ts      # ID管理器
├── tools.json                 # MCP工具定义文件
├── package.json
└── README.md
```

### 5.2 依赖项

- `@modelcontextprotocol/sdk` - MCP SDK
- `three` - Three.js库
- `uuid` - UUID生成

### 5.3 实现语言

- **TypeScript** - 类型安全，更好的开发体验
- **Node.js** - 运行环境

---

## 六、工具定义示例

### 示例1：创建场景

```json
{
  "name": "three_create_scene",
  "description": "创建一个新的Three.js场景。Create a new Three.js scene.",
  "inputSchema": {
    "type": "object",
    "properties": {
      "background": {
        "type": "string",
        "description": "场景背景颜色（十六进制格式，如'#000000'）。Scene background color in hex format (e.g., '#000000').",
        "default": null
      }
    },
    "required": []
  }
}
```

### 示例2：创建立方体几何体

```json
{
  "name": "three_create_box_geometry",
  "description": "创建一个立方体几何体。Create a box geometry.",
  "inputSchema": {
    "type": "object",
    "properties": {
      "width": {
        "type": "number",
        "description": "立方体宽度。Box width.",
        "default": 1
      },
      "height": {
        "type": "number",
        "description": "立方体高度。Box height.",
        "default": 1
      },
      "depth": {
        "type": "number",
        "description": "立方体深度。Box depth.",
        "default": 1
      },
      "widthSegments": {
        "type": "number",
        "description": "宽度分段数。Number of width segments.",
        "default": 1
      },
      "heightSegments": {
        "type": "number",
        "description": "高度分段数。Number of height segments.",
        "default": 1
      },
      "depthSegments": {
        "type": "number",
        "description": "深度分段数。Number of depth segments.",
        "default": 1
      }
    },
    "required": []
  }
}
```

### 示例3：创建网格对象

```json
{
  "name": "three_create_mesh",
  "description": "创建一个网格对象（几何体+材质）。Create a mesh object (geometry + material).",
  "inputSchema": {
    "type": "object",
    "properties": {
      "geometryId": {
        "type": "string",
        "description": "几何体ID。Geometry ID."
      },
      "materialId": {
        "type": "string",
        "description": "材质ID。Material ID."
      },
      "position": {
        "type": "object",
        "description": "对象位置 {x, y, z}。Object position {x, y, z}.",
        "properties": {
          "x": {"type": "number", "default": 0},
          "y": {"type": "number", "default": 0},
          "z": {"type": "number", "default": 0}
        }
      },
      "rotation": {
        "type": "object",
        "description": "对象旋转（弧度）{x, y, z}。Object rotation in radians {x, y, z}.",
        "properties": {
          "x": {"type": "number", "default": 0},
          "y": {"type": "number", "default": 0},
          "z": {"type": "number", "default": 0}
        }
      },
      "scale": {
        "type": "object",
        "description": "对象缩放 {x, y, z}。Object scale {x, y, z}.",
        "properties": {
          "x": {"type": "number", "default": 1},
          "y": {"type": "number", "default": 1},
          "z": {"type": "number", "default": 1}
        }
      }
    },
    "required": ["geometryId", "materialId"]
  }
}
```

---

## 七、下一步行动

1. ✅ 完成功能分析和工具设计
2. ⏳ 创建MCP工具定义文件（tools.json）
3. ⏳ 实现MCP服务器基础框架
4. ⏳ 实现阶段一的核心工具
5. ⏳ 测试和文档编写

---

**文档版本**: v1.0  
**创建日期**: 2025-01-XX

