/**
 * Object creation and manipulation tools
 * 对象创建和操作工具
 */

import * as THREE from 'three';
import { store } from '../state/store.js';
import { generateObjectId } from '../utils/id-manager.js';
import { parsePosition, parseRotation, parseScale } from '../utils/helpers.js';
import type { CallToolResult } from '@modelcontextprotocol/sdk/types.js';

/**
 * 创建网格对象 / Create mesh object
 */
export async function handleCreateMesh(args: any): Promise<CallToolResult> {
  const geometry = store.getGeometry(args.geometryId);
  if (!geometry) {
    throw new Error(`Geometry not found: ${args.geometryId}`);
  }

  const material = store.getMaterial(args.materialId);
  if (!material) {
    throw new Error(`Material not found: ${args.materialId}`);
  }

  const mesh = new THREE.Mesh(geometry, material);

  // 设置名称 / Set name
  if (args.name) {
    mesh.name = args.name;
  }

  // 设置位置 / Set position
  if (args.position) {
    mesh.position.copy(parsePosition(args.position));
  }

  // 设置旋转 / Set rotation
  if (args.rotation) {
    mesh.rotation.copy(parseRotation(args.rotation));
  }

  // 设置缩放 / Set scale
  if (args.scale) {
    mesh.scale.copy(parseScale(args.scale));
  }

  // 设置可见性 / Set visibility
  if (args.visible !== undefined) {
    mesh.visible = args.visible;
  }

  const objectId = generateObjectId();
  store.objects.set(objectId, mesh);

  return {
    content: [{
      type: 'text',
      text: JSON.stringify({ objectId }),
    }],
  };
}

/**
 * 创建线对象 / Create line object
 */
export async function handleCreateLine(args: any): Promise<CallToolResult> {
  const geometry = store.getGeometry(args.geometryId);
  if (!geometry) {
    throw new Error(`Geometry not found: ${args.geometryId}`);
  }

  const material = store.getMaterial(args.materialId);
  if (!material) {
    throw new Error(`Material not found: ${args.materialId}`);
  }

  const line = new THREE.Line(geometry, material as THREE.LineBasicMaterial | THREE.LineDashedMaterial);

  if (args.position) {
    line.position.copy(parsePosition(args.position));
  }

  const objectId = generateObjectId();
  store.objects.set(objectId, line);

  return {
    content: [{
      type: 'text',
      text: JSON.stringify({ objectId }),
    }],
  };
}

/**
 * 创建点对象 / Create points object
 */
export async function handleCreatePoints(args: any): Promise<CallToolResult> {
  const geometry = store.getGeometry(args.geometryId);
  if (!geometry) {
    throw new Error(`Geometry not found: ${args.geometryId}`);
  }

  const material = store.getMaterial(args.materialId);
  if (!material) {
    throw new Error(`Material not found: ${args.materialId}`);
  }

  const points = new THREE.Points(geometry, material as THREE.PointsMaterial);

  if (args.position) {
    points.position.copy(parsePosition(args.position));
  }

  const objectId = generateObjectId();
  store.objects.set(objectId, points);

  return {
    content: [{
      type: 'text',
      text: JSON.stringify({ objectId }),
    }],
  };
}

/**
 * 设置对象位置 / Set object position
 */
export async function handleSetObjectPosition(args: any): Promise<CallToolResult> {
  const object = store.getObject(args.objectId);
  if (!object) {
    throw new Error(`Object not found: ${args.objectId}`);
  }

  object.position.copy(parsePosition(args.position));

  return {
    content: [{
      type: 'text',
      text: JSON.stringify({ success: true }),
    }],
  };
}

/**
 * 设置对象旋转 / Set object rotation
 */
export async function handleSetObjectRotation(args: any): Promise<CallToolResult> {
  const object = store.getObject(args.objectId);
  if (!object) {
    throw new Error(`Object not found: ${args.objectId}`);
  }

  object.rotation.copy(parseRotation(args.rotation));

  return {
    content: [{
      type: 'text',
      text: JSON.stringify({ success: true }),
    }],
  };
}

/**
 * 设置对象缩放 / Set object scale
 */
export async function handleSetObjectScale(args: any): Promise<CallToolResult> {
  const object = store.getObject(args.objectId);
  if (!object) {
    throw new Error(`Object not found: ${args.objectId}`);
  }

  object.scale.copy(parseScale(args.scale));

  return {
    content: [{
      type: 'text',
      text: JSON.stringify({ success: true }),
    }],
  };
}

/**
 * 获取对象信息 / Get object info
 */
export async function handleGetObjectInfo(args: any): Promise<CallToolResult> {
  const object = store.getObject(args.objectId);
  if (!object) {
    throw new Error(`Object not found: ${args.objectId}`);
  }

  const info = {
    objectId: args.objectId,
    type: object.type,
    name: object.name,
    position: {
      x: object.position.x,
      y: object.position.y,
      z: object.position.z,
    },
    rotation: {
      x: object.rotation.x,
      y: object.rotation.y,
      z: object.rotation.z,
    },
    scale: {
      x: object.scale.x,
      y: object.scale.y,
      z: object.scale.z,
    },
    visible: object.visible,
  };

  return {
    content: [{
      type: 'text',
      text: JSON.stringify(info),
    }],
  };
}

/**
 * 克隆对象 / Clone object
 */
export async function handleCloneObject(args: any): Promise<CallToolResult> {
  const object = store.getObject(args.objectId);
  if (!object) {
    throw new Error(`Object not found: ${args.objectId}`);
  }

  const cloned = object.clone(args.recursive !== false);

  const objectId = generateObjectId();
  store.objects.set(objectId, cloned);

  return {
    content: [{
      type: 'text',
      text: JSON.stringify({ objectId }),
    }],
  };
}

/**
 * 释放对象资源 / Dispose object resources
 */
export async function handleDisposeObject(args: any): Promise<CallToolResult> {
  const object = store.getObject(args.objectId);
  if (!object) {
    throw new Error(`Object not found: ${args.objectId}`);
  }

  // 如果是网格对象，释放几何体和材质 / If mesh object, dispose geometry and material
  if (object instanceof THREE.Mesh) {
    if (object.geometry) {
      object.geometry.dispose();
    }
    if (object.material) {
      if (Array.isArray(object.material)) {
        object.material.forEach(m => m.dispose());
      } else {
        object.material.dispose();
      }
    }
  }

  // 从场景中移除 / Remove from scene
  if (object.parent) {
    object.parent.remove(object);
  }

  // 从存储中移除 / Remove from store
  store.objects.delete(args.objectId);

  return {
    content: [{
      type: 'text',
      text: JSON.stringify({ success: true }),
    }],
  };
}

