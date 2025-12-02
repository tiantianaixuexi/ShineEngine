# Three.js MCP Lite - 代码生成器版本

轻量级Three.js MCP服务器，不依赖Three.js运行时，而是生成Three.js代码供用户使用。

## 🎯 设计理念

**传统MCP服务器**: 运行Three.js → 生成实际的3D场景 → 返回渲染结果

**Lite版本**: 生成Three.js代码 → 用户获得可执行代码 → 在自己的项目中运行

## ✨ 优势

### ✅ 无依赖运行
- 不需要安装Three.js库
- 服务器体积小，启动快
- 适合资源受限的环境

### ✅ 代码复用性
- 生成的代码可以直接复制到项目中使用
- 用户可以修改和定制生成的代码
- 代码是完整的Three.js实现

### ✅ 学习友好
- AI可以解释生成的代码
- 用户可以看到Three.js的实际用法
- 有助于学习3D编程

## 🚀 使用方法

### 1. 启动服务器
```bash
npm install
npm run build
npm start
```

### 2. 调用工具
```json
{
  "tool": "three_create_box_geometry",
  "arguments": {
    "width": 2,
    "height": 1,
    "depth": 1
  }
}
```

### 3. 获取结果
```json
{
  "geometryId": "geometry_abc123",
  "code": "const geometry = new THREE.BoxGeometry(2, 1, 1, 1, 1, 1);",
  "description": "Box geometry creation code"
}
```

### 4. 在项目中使用
```javascript
// 复制生成的代码到你的Three.js项目中
const geometry = new THREE.BoxGeometry(2, 1, 1, 1, 1, 1);
const material = new THREE.MeshBasicMaterial({ color: 0xff0000 });
const mesh = new THREE.Mesh(geometry, material);
scene.add(mesh);
```

## 📋 支持的工具

### 几何体代码生成
- `three_create_box_geometry` - 立方体
- `three_create_sphere_geometry` - 球体
- `three_create_plane_geometry` - 平面
- `three_create_cylinder_geometry` - 圆柱体
- `three_create_cone_geometry` - 圆锥体
- `three_create_torus_geometry` - 圆环

### 材质代码生成（计划中）
- `three_create_basic_material` - 基础材质
- `three_create_standard_material` - 标准材质

### 对象代码生成（计划中）
- `three_create_mesh` - 网格对象
- `three_create_scene` - 场景创建

## 🔄 与完整版本对比

| 特性 | 完整版本 | Lite版本 |
|------|----------|----------|
| 依赖 | Three.js库 | 无外部依赖 |
| 输出 | 实际渲染结果 | Three.js代码 |
| 功能 | 完全3D操作 | 代码生成 |
| 体积 | ~5MB | ~100KB |
| 性能 | 高（本地渲染） | 极高（纯代码） |
| 学习价值 | 中等 | 高 |

## 🎯 适用场景

### ✅ 推荐使用Lite版本的情况
- 学习Three.js开发
- 快速原型设计
- 代码示例生成
- 教育和教学场景
- 资源受限环境

### ✅ 推荐使用完整版本的情况
- 需要实际渲染结果
- 复杂的3D交互
- 实时3D预览
- 专业3D应用开发

## 📚 示例

### 生成一个完整的3D场景代码

```javascript
// AI通过MCP工具生成的完整代码示例
import * as THREE from 'three';

// 创建场景
const scene = new THREE.Scene();

// 创建几何体
const geometry = new THREE.BoxGeometry(2, 1, 1, 1, 1, 1);

// 创建材质
const material = new THREE.MeshBasicMaterial({
  color: 0xff0000,
  wireframe: false
});

// 创建网格
const mesh = new THREE.Mesh(geometry, material);

// 设置变换
mesh.position.set(0, 0, 0);
mesh.rotation.set(0, 0, 0);
mesh.scale.set(1, 1, 1);

// 添加到场景
scene.add(mesh);

// 创建相机
const camera = new THREE.PerspectiveCamera(75, window.innerWidth / window.innerHeight, 0.1, 1000);
camera.position.set(0, 0, 5);

// 创建渲染器
const renderer = new THREE.WebGLRenderer();
renderer.setSize(window.innerWidth, window.innerHeight);
document.body.appendChild(renderer.domElement);

// 渲染循环
function animate() {
  requestAnimationFrame(animate);
  renderer.render(scene, camera);
}
animate();
```

## 🚀 未来扩展

- 添加更多几何体类型
- 支持材质和光源代码生成
- 生成完整的场景代码片段
- 支持动画代码生成
- 代码优化和格式化

## 📄 许可证

MIT License

