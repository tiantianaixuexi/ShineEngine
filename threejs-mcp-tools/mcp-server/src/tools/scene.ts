/**
 * Scene management tools
 * 场景管理工具
 */

import * as THREE from 'three';
import { store } from '../state/store.js';
import { generateSceneId } from '../utils/id-manager.js';
import { parseColor } from '../utils/helpers.js';
import type { CallToolResult } from '@modelcontextprotocol/sdk/types.js';

/**
 * 创建场景 / Create scene
 */
export async function handleCreateScene(args: any): Promise<CallToolResult> {
  const scene = new THREE.Scene();

  // 设置背景 / Set background
  if (args.background) {
    scene.background = parseColor(args.background);
  }

  // 设置雾效 / Set fog
  if (args.fog) {
    if (args.fog.type === 'linear') {
      scene.fog = new THREE.Fog(
        parseColor(args.fog.color),
        args.fog.near || 1,
        args.fog.far || 1000
      );
    } else if (args.fog.type === 'exponential') {
      scene.fog = new THREE.FogExp2(
        parseColor(args.fog.color),
        args.fog.far || 0.00025
      );
    }
  }

  const sceneId = generateSceneId();
  store.scenes.set(sceneId, scene);

  return {
    content: [{
      type: 'text',
      text: JSON.stringify({ sceneId }),
    }],
  };
}

/**
 * 添加对象到场景 / Add object to scene
 */
export async function handleAddObjectToScene(args: any): Promise<CallToolResult> {
  const scene = store.getScene(args.sceneId);
  if (!scene) {
    throw new Error(`Scene not found: ${args.sceneId}`);
  }

  const object = store.getObject(args.objectId);
  if (!object) {
    throw new Error(`Object not found: ${args.objectId}`);
  }

  scene.add(object);

  return {
    content: [{
      type: 'text',
      text: JSON.stringify({ success: true }),
    }],
  };
}

/**
 * 从场景移除对象 / Remove object from scene
 */
export async function handleRemoveObjectFromScene(args: any): Promise<CallToolResult> {
  const scene = store.getScene(args.sceneId);
  if (!scene) {
    throw new Error(`Scene not found: ${args.sceneId}`);
  }

  const object = store.getObject(args.objectId);
  if (!object) {
    throw new Error(`Object not found: ${args.objectId}`);
  }

  scene.remove(object);

  return {
    content: [{
      type: 'text',
      text: JSON.stringify({ success: true }),
    }],
  };
}

/**
 * 获取场景中的所有对象 / Get all objects in scene
 */
export async function handleGetSceneObjects(args: any): Promise<CallToolResult> {
  const scene = store.getScene(args.sceneId);
  if (!scene) {
    throw new Error(`Scene not found: ${args.sceneId}`);
  }

  const objects: any[] = [];
  scene.traverse((object) => {
    if (object !== scene) {
      // 查找对象ID / Find object ID
      for (const [id, obj] of store.objects.entries()) {
        if (obj === object) {
          objects.push({
            objectId: id,
            type: object.type,
            name: object.name,
            position: {
              x: object.position.x,
              y: object.position.y,
              z: object.position.z,
            },
          });
          break;
        }
      }
    }
  });

  return {
    content: [{
      type: 'text',
      text: JSON.stringify({ objects }),
    }],
  };
}

/**
 * 设置场景背景 / Set scene background
 */
export async function handleSetSceneBackground(args: any): Promise<CallToolResult> {
  const scene = store.getScene(args.sceneId);
  if (!scene) {
    throw new Error(`Scene not found: ${args.sceneId}`);
  }

  if (args.background.startsWith('texture_')) {
    // 使用纹理 / Use texture
    const texture = store.getTexture(args.background);
    if (!texture) {
      throw new Error(`Texture not found: ${args.background}`);
    }
    scene.background = texture;
  } else {
    // 使用颜色 / Use color
    scene.background = parseColor(args.background);
  }

  return {
    content: [{
      type: 'text',
      text: JSON.stringify({ success: true }),
    }],
  };
}

/**
 * 导出场景为JSON / Export scene to JSON
 */
export async function handleExportSceneJson(args: any): Promise<CallToolResult> {
  const scene = store.getScene(args.sceneId);
  if (!scene) {
    throw new Error(`Scene not found: ${args.sceneId}`);
  }

  const json = scene.toJSON();
  
  return {
    content: [{
      type: 'text',
      text: JSON.stringify(json, null, 2),
    }],
  };
}

