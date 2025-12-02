/**
 * Camera creation and manipulation tools
 * 相机创建和操作工具
 */

import * as THREE from 'three';
import { store } from '../state/store.js';
import { generateCameraId } from '../utils/id-manager.js';
import { parsePosition } from '../utils/helpers.js';
import type { CallToolResult } from '@modelcontextprotocol/sdk/types.js';

/**
 * 创建透视相机 / Create perspective camera
 */
export async function handleCreatePerspectiveCamera(args: any): Promise<CallToolResult> {
  const camera = new THREE.PerspectiveCamera(
    args.fov || 50,
    args.aspect || 1,
    args.near || 0.1,
    args.far || 2000
  );

  if (args.position) {
    camera.position.copy(parsePosition(args.position));
  }

  if (args.lookAt) {
    camera.lookAt(parsePosition(args.lookAt));
  }

  const cameraId = generateCameraId();
  store.cameras.set(cameraId, camera);

  return {
    content: [{
      type: 'text',
      text: JSON.stringify({ cameraId }),
    }],
  };
}

/**
 * 创建正交相机 / Create orthographic camera
 */
export async function handleCreateOrthographicCamera(args: any): Promise<CallToolResult> {
  const camera = new THREE.OrthographicCamera(
    args.left || -1,
    args.right || 1,
    args.top || 1,
    args.bottom || -1,
    args.near || 0.1,
    args.far || 2000
  );

  if (args.position) {
    camera.position.copy(parsePosition(args.position));
  }

  if (args.lookAt) {
    camera.lookAt(parsePosition(args.lookAt));
  }

  const cameraId = generateCameraId();
  store.cameras.set(cameraId, camera);

  return {
    content: [{
      type: 'text',
      text: JSON.stringify({ cameraId }),
    }],
  };
}

/**
 * 设置相机位置 / Set camera position
 */
export async function handleSetCameraPosition(args: any): Promise<CallToolResult> {
  const camera = store.getCamera(args.cameraId);
  if (!camera) {
    throw new Error(`Camera not found: ${args.cameraId}`);
  }

  camera.position.copy(parsePosition(args.position));

  return {
    content: [{
      type: 'text',
      text: JSON.stringify({ success: true }),
    }],
  };
}

/**
 * 设置相机朝向 / Set camera look at
 */
export async function handleSetCameraLookAt(args: any): Promise<CallToolResult> {
  const camera = store.getCamera(args.cameraId);
  if (!camera) {
    throw new Error(`Camera not found: ${args.cameraId}`);
  }

  camera.lookAt(parsePosition(args.target));

  return {
    content: [{
      type: 'text',
      text: JSON.stringify({ success: true }),
    }],
  };
}

/**
 * 设置相机视野 / Set camera FOV
 */
export async function handleSetCameraFov(args: any): Promise<CallToolResult> {
  const camera = store.getCamera(args.cameraId);
  if (!camera) {
    throw new Error(`Camera not found: ${args.cameraId}`);
  }

  if (!(camera instanceof THREE.PerspectiveCamera)) {
    throw new Error('FOV can only be set for PerspectiveCamera');
  }

  camera.fov = args.fov;
  camera.updateProjectionMatrix();

  return {
    content: [{
      type: 'text',
      text: JSON.stringify({ success: true }),
    }],
  };
}

