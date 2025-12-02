/**
 * State store for Three.js objects
 * 状态存储：管理Three.js对象的状态
 */

import * as THREE from 'three';

export class StateStore {
  // 场景存储 / Scene storage
  public scenes = new Map<string, THREE.Scene>();
  
  // 对象存储 / Object storage
  public objects = new Map<string, THREE.Object3D>();
  
  // 几何体存储 / Geometry storage
  public geometries = new Map<string, THREE.BufferGeometry>();
  
  // 材质存储 / Material storage
  public materials = new Map<string, THREE.Material>();
  
  // 光源存储 / Light storage
  public lights = new Map<string, THREE.Light>();
  
  // 相机存储 / Camera storage
  public cameras = new Map<string, THREE.Camera>();
  
  // 渲染器存储 / Renderer storage
  public renderers = new Map<string, THREE.WebGLRenderer>();
  
  // 纹理存储 / Texture storage
  public textures = new Map<string, THREE.Texture>();

  /**
   * 清理所有资源 / Clear all resources
   */
  public dispose(): void {
    // 释放几何体 / Dispose geometries
    this.geometries.forEach(geometry => geometry.dispose());
    this.geometries.clear();

    // 释放材质 / Dispose materials
    this.materials.forEach(material => {
      if (Array.isArray(material)) {
        material.forEach(m => m.dispose());
      } else {
        material.dispose();
      }
    });
    this.materials.clear();

    // 释放纹理 / Dispose textures
    this.textures.forEach(texture => texture.dispose());
    this.textures.clear();

    // 释放渲染器 / Dispose renderers
    this.renderers.forEach(renderer => {
      renderer.dispose();
      renderer.domElement?.remove();
    });
    this.renderers.clear();

    // 清空其他存储 / Clear other storages
    this.scenes.clear();
    this.objects.clear();
    this.lights.clear();
    this.cameras.clear();
  }

  /**
   * 获取场景 / Get scene
   */
  public getScene(sceneId: string): THREE.Scene | undefined {
    return this.scenes.get(sceneId);
  }

  /**
   * 获取对象 / Get object
   */
  public getObject(objectId: string): THREE.Object3D | undefined {
    return this.objects.get(objectId);
  }

  /**
   * 获取几何体 / Get geometry
   */
  public getGeometry(geometryId: string): THREE.BufferGeometry | undefined {
    return this.geometries.get(geometryId);
  }

  /**
   * 获取材质 / Get material
   */
  public getMaterial(materialId: string): THREE.Material | undefined {
    return this.materials.get(materialId);
  }

  /**
   * 获取光源 / Get light
   */
  public getLight(lightId: string): THREE.Light | undefined {
    return this.lights.get(lightId);
  }

  /**
   * 获取相机 / Get camera
   */
  public getCamera(cameraId: string): THREE.Camera | undefined {
    return this.cameras.get(cameraId);
  }

  /**
   * 获取渲染器 / Get renderer
   */
  public getRenderer(rendererId: string): THREE.WebGLRenderer | undefined {
    return this.renderers.get(rendererId);
  }

  /**
   * 获取纹理 / Get texture
   */
  public getTexture(textureId: string): THREE.Texture | undefined {
    return this.textures.get(textureId);
  }
}

// 全局状态存储实例 / Global state store instance
export const store = new StateStore();

