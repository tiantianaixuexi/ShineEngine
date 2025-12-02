# Three.js MCP工具集使用指南

## 🚀 快速开始

### 步骤1：选择版本

**轻量级版本（推荐新手）**：
```bash
cd mcp-server-lite
npm install
npm run build
npm start
```

**完整版本（专业用户）**：
```bash
cd mcp-server
# 注意：需要先安装 @types/three
npm install @types/three --save-dev
npm run build
npm start
```

### 步骤2：配置MCP客户端

在Claude Desktop配置文件中添加：

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

### 步骤3：开始使用

重启Claude Desktop，现在你可以在对话中使用Three.js工具了！

---

## 🎨 使用示例

### 示例1：创建3D立方体

**输入**：
```
创建一个红色的3D立方体，给我Three.js代码
```

**Claude会调用工具**：
```
three_create_box_geometry → 生成几何体代码
```

**输出**：
```javascript
const geometry = new THREE.BoxGeometry(1, 1, 1, 1, 1, 1);
```

### 示例2：生成完整场景

**输入**：
```
给我一个完整的Three.js场景代码，包括相机、光源和渲染器
```

**Claude会调用工具**：
```
three_generate_scene_code → 生成完整场景代码
```

**输出**：
```javascript
// Scene setup
const scene = new THREE.Scene();
scene.background = new THREE.Color(0x87CEEB);

// Camera setup
const camera = new THREE.PerspectiveCamera(75, window.innerWidth / window.innerHeight, 0.1, 1000);
camera.position.set(0, 0, 5);

// Lighting setup
const ambientLight = new THREE.AmbientLight(0xffffff, 0.5);
scene.add(ambientLight);

const directionalLight = new THREE.DirectionalLight(0xffffff, 1);
directionalLight.position.set(10, 10, 5);
scene.add(directionalLight);

// Renderer setup
const renderer = new THREE.WebGLRenderer({ antialias: true });
renderer.setSize(window.innerWidth, window.innerHeight);
renderer.shadowMap.enabled = true;
renderer.shadowMap.type = THREE.PCFSoftShadowMap;
document.body.appendChild(renderer.domElement);

// Animation loop
function animate() {
  requestAnimationFrame(animate);
  renderer.render(scene, camera);
}
animate();

// Handle window resize
window.addEventListener('resize', () => {
  camera.aspect = window.innerWidth / window.innerHeight;
  camera.updateProjectionMatrix();
  renderer.setSize(window.innerWidth, window.innerHeight);
});
```

---

## 🛠️ 可用工具列表

### 轻量级版本（7个工具）

#### 几何体代码生成
- `three_create_box_geometry` - 立方体
- `three_create_sphere_geometry` - 球体
- `three_create_plane_geometry` - 平面
- `three_create_cylinder_geometry` - 圆柱体
- `three_create_cone_geometry` - 圆锥体
- `three_create_torus_geometry` - 圆环

#### 场景代码生成
- `three_generate_scene_code` - 完整场景代码

### 完整版本（57个工具）

#### 场景管理（6个）
- `three_create_scene` - 创建场景
- `three_add_object_to_scene` - 添加对象
- `three_remove_object_from_scene` - 移除对象
- `three_get_scene_objects` - 获取场景对象
- `three_set_scene_background` - 设置背景
- `three_export_scene_json` - 导出场景

#### 几何体（13个）
- `three_create_box_geometry` - 立方体
- `three_create_sphere_geometry` - 球体
- `three_create_plane_geometry` - 平面
- `three_create_cylinder_geometry` - 圆柱体
- `three_create_cone_geometry` - 圆锥体
- `three_create_torus_geometry` - 圆环
- 更多几何体...

#### 材质（8个）
- `three_create_basic_material` - 基础材质
- `three_create_standard_material` - 标准材质
- `three_create_physical_material` - 物理材质
- 更多材质...

#### 对象操作（9个）
- `three_create_mesh` - 创建网格
- `three_set_object_position` - 设置位置
- `three_set_object_rotation` - 设置旋转
- `three_set_object_scale` - 设置缩放
- `three_get_object_info` - 获取信息

#### 光源（9个）
- `three_create_ambient_light` - 环境光
- `three_create_directional_light` - 方向光
- `three_create_point_light` - 点光源
- `three_create_spot_light` - 聚光灯
- 更多光源...

#### 相机（5个）
- `three_create_perspective_camera` - 透视相机
- `three_create_orthographic_camera` - 正交相机
- 相机控制工具...

#### 渲染器（4个）
- `three_create_renderer` - 创建渲染器
- `three_render_scene` - 渲染场景
- `three_get_renderer_canvas` - 获取图像
- `three_set_renderer_size` - 设置尺寸

#### 纹理（2个）
- `three_load_texture` - 加载纹理
- `three_create_data_texture` - 创建数据纹理

---

## 💻 在项目中使用生成的代码

### 基本HTML模板

```html
<!DOCTYPE html>
<html>
<head>
    <title>Three.js Scene</title>
    <style>
        body { margin: 0; }
        canvas { display: block; }
    </style>
</head>
<body>
    <script type="importmap">
    {
        "imports": {
            "three": "https://unpkg.com/three@0.181.0/build/three.module.js"
        }
    }
    </script>
    <script type="module">
        // 在这里粘贴AI生成的代码

        // 示例：添加一个红色立方体
        const geometry = new THREE.BoxGeometry(1, 1, 1, 1, 1, 1);
        const material = new THREE.MeshBasicMaterial({ color: 0xff0000 });
        const cube = new THREE.Mesh(geometry, material);
        scene.add(cube);

        // 开始渲染
        renderer.render(scene, camera);
    </script>
</body>
</html>
```

### 使用CDN

```html
<script src="https://unpkg.com/three@0.181.0/build/three.min.js"></script>
<script>
    // 在这里使用生成的代码（非模块版本）
    const scene = new THREE.Scene();
    // ...
</script>
```

### 使用npm

```bash
npm install three
```

```javascript
import * as THREE from 'three';

// 在这里使用生成的代码
const scene = new THREE.Scene();
// ...
```

---

## 🔧 故障排除

### 问题1：工具不可用
**解决方案**：
1. 确认服务器正在运行：检查终端输出
2. 确认MCP配置正确：重启Claude Desktop
3. 检查路径是否正确：使用绝对路径

### 问题2：代码不工作
**解决方案**：
1. 检查Three.js版本：确保与生成的代码兼容
2. 检查WebGL支持：某些环境可能不支持
3. 查看浏览器控制台错误信息

### 问题3：构建失败
**解决方案**：
```bash
# 清理缓存
rm -rf node_modules package-lock.json
npm install

# 完整版本需要类型定义
npm install @types/three --save-dev
```

---

## 📚 学习资源

### 官方文档
- [Three.js官方文档](https://threejs.org/docs/)
- [Three.js示例](https://threejs.org/examples/)

### 学习路径
1. **基础概念**：场景(scene)、相机(camera)、渲染器(renderer)
2. **几何体**：BoxGeometry、SphereGeometry等
3. **材质**：MeshBasicMaterial、MeshStandardMaterial等
4. **光源**：DirectionalLight、PointLight等
5. **动画**：requestAnimationFrame、变换操作

### 示例项目
- 查看 `examples/` 目录中的示例
- 从简单到复杂逐步学习

---

## 🎯 最佳实践

### 1. 从简单开始
```javascript
// 先从基础形状开始
const geometry = new THREE.BoxGeometry(1, 1, 1);
const material = new THREE.MeshBasicMaterial({ color: 0xff0000 });
const cube = new THREE.Mesh(geometry, material);
scene.add(cube);
renderer.render(scene, camera);
```

### 2. 添加光源
```javascript
// 添加光源让材质可见
const light = new THREE.DirectionalLight(0xffffff, 1);
light.position.set(10, 10, 5);
scene.add(light);
```

### 3. 动画循环
```javascript
function animate() {
    requestAnimationFrame(animate);
    cube.rotation.x += 0.01;
    cube.rotation.y += 0.01;
    renderer.render(scene, camera);
}
animate();
```

### 4. 响应式设计
```javascript
window.addEventListener('resize', () => {
    camera.aspect = window.innerWidth / window.innerHeight;
    camera.updateProjectionMatrix();
    renderer.setSize(window.innerWidth, window.innerHeight);
});
```

---

## 🚀 进阶使用

### 与其他工具结合
- 使用DALL-E生成纹理
- 使用代码解释器运行Three.js代码
- 结合其他MCP工具创建完整应用

### 自定义扩展
- 添加新的几何体类型
- 创建自定义材质
- 实现特殊效果

---

## 📞 获取帮助

如果遇到问题：

1. 检查 [MCP_README.md](./MCP_README.md) 详细文档
2. 查看 [MCP_LITE_GUIDE.md](./MCP_LITE_GUIDE.md) 轻量级版本指南
3. 检查Claude Desktop的MCP服务器日志
4. 查看浏览器开发者工具控制台

**祝你享受Three.js的3D世界！🎨✨**

