/**
 * Light creation and manipulation tools
 * 光源创建和操作工具
 */

import * as THREE from 'three';
import { store } from '../state/store.js';
import { generateLightId } from '../utils/id-manager.js';
import { parseColor, parsePosition } from '../utils/helpers.js';
import type { CallToolResult } from '@modelcontextprotocol/sdk/types.js';

/**
 * 创建环境光 / Create ambient light
 */
export async function handleCreateAmbientLight(args: any): Promise<CallToolResult> {
  const light = new THREE.AmbientLight(
    parseColor(args.color || '#ffffff'),
    args.intensity || 1
  );

  const lightId = generateLightId();
  store.lights.set(lightId, light);

  return {
    content: [{
      type: 'text',
      text: JSON.stringify({ lightId }),
    }],
  };
}

/**
 * 创建方向光 / Create directional light
 */
export async function handleCreateDirectionalLight(args: any): Promise<CallToolResult> {
  const light = new THREE.DirectionalLight(
    parseColor(args.color || '#ffffff'),
    args.intensity || 1
  );

  if (args.position) {
    light.position.copy(parsePosition(args.position));
  }

  if (args.target) {
    light.target.position.copy(parsePosition(args.target));
  }

  if (args.castShadow) {
    light.castShadow = true;
  }

  const lightId = generateLightId();
  store.lights.set(lightId, light);

  return {
    content: [{
      type: 'text',
      text: JSON.stringify({ lightId }),
    }],
  };
}

/**
 * 创建点光源 / Create point light
 */
export async function handleCreatePointLight(args: any): Promise<CallToolResult> {
  const light = new THREE.PointLight(
    parseColor(args.color || '#ffffff'),
    args.intensity || 1,
    args.distance || 0,
    args.decay || 2
  );

  if (args.position) {
    light.position.copy(parsePosition(args.position));
  }

  if (args.castShadow) {
    light.castShadow = true;
  }

  const lightId = generateLightId();
  store.lights.set(lightId, light);

  return {
    content: [{
      type: 'text',
      text: JSON.stringify({ lightId }),
    }],
  };
}

/**
 * 创建聚光灯 / Create spot light
 */
export async function handleCreateSpotLight(args: any): Promise<CallToolResult> {
  const light = new THREE.SpotLight(
    parseColor(args.color || '#ffffff'),
    args.intensity || 1,
    args.distance || 0,
    args.angle || Math.PI / 6,
    args.penumbra || 0,
    args.decay || 2
  );

  if (args.position) {
    light.position.copy(parsePosition(args.position));
  }

  if (args.target) {
    light.target.position.copy(parsePosition(args.target));
  }

  if (args.castShadow) {
    light.castShadow = true;
  }

  const lightId = generateLightId();
  store.lights.set(lightId, light);

  return {
    content: [{
      type: 'text',
      text: JSON.stringify({ lightId }),
    }],
  };
}

/**
 * 创建半球光 / Create hemisphere light
 */
export async function handleCreateHemisphereLight(args: any): Promise<CallToolResult> {
  const light = new THREE.HemisphereLight(
    parseColor(args.skyColor || '#ffffff'),
    parseColor(args.groundColor || '#ffffff'),
    args.intensity || 1
  );

  if (args.position) {
    light.position.copy(parsePosition(args.position));
  }

  const lightId = generateLightId();
  store.lights.set(lightId, light);

  return {
    content: [{
      type: 'text',
      text: JSON.stringify({ lightId }),
    }],
  };
}

/**
 * 创建矩形区域光 / Create rect area light
 */
export async function handleCreateRectAreaLight(args: any): Promise<CallToolResult> {
  const light = new THREE.RectAreaLight(
    parseColor(args.color || '#ffffff'),
    args.intensity || 1,
    args.width || 10,
    args.height || 10
  );

  if (args.position) {
    light.position.copy(parsePosition(args.position));
  }

  const lightId = generateLightId();
  store.lights.set(lightId, light);

  return {
    content: [{
      type: 'text',
      text: JSON.stringify({ lightId }),
    }],
  };
}

/**
 * 设置光源颜色 / Set light color
 */
export async function handleSetLightColor(args: any): Promise<CallToolResult> {
  const light = store.getLight(args.lightId);
  if (!light) {
    throw new Error(`Light not found: ${args.lightId}`);
  }

  light.color.copy(parseColor(args.color));

  return {
    content: [{
      type: 'text',
      text: JSON.stringify({ success: true }),
    }],
  };
}

/**
 * 设置光源强度 / Set light intensity
 */
export async function handleSetLightIntensity(args: any): Promise<CallToolResult> {
  const light = store.getLight(args.lightId);
  if (!light) {
    throw new Error(`Light not found: ${args.lightId}`);
  }

  light.intensity = args.intensity;

  return {
    content: [{
      type: 'text',
      text: JSON.stringify({ success: true }),
    }],
  };
}

/**
 * 设置光源位置 / Set light position
 */
export async function handleSetLightPosition(args: any): Promise<CallToolResult> {
  const light = store.getLight(args.lightId);
  if (!light) {
    throw new Error(`Light not found: ${args.lightId}`);
  }

  light.position.copy(parsePosition(args.position));

  return {
    content: [{
      type: 'text',
      text: JSON.stringify({ success: true }),
    }],
  };
}

