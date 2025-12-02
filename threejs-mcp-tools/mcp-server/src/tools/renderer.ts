/**
 * Renderer creation and manipulation tools
 * 渲染器创建和操作工具
 */

import * as THREE from 'three';
import { store } from '../state/store.js';
import { generateRendererId } from '../utils/id-manager.js';
import { parseColor, canvasToBase64 } from '../utils/helpers.js';
import type { CallToolResult } from '@modelcontextprotocol/sdk/types.js';

/**
 * 创建渲染器 / Create renderer
 */
export async function handleCreateRenderer(args: any): Promise<CallToolResult> {
  const renderer = new THREE.WebGLRenderer({
    antialias: args.antialias !== false,
    alpha: args.alpha || false,
  });

  renderer.setSize(args.width || 800, args.height || 600);
  renderer.setClearColor(parseColor(args.backgroundColor || '#000000'));

  const rendererId = generateRendererId();
  store.renderers.set(rendererId, renderer);

  return {
    content: [{
      type: 'text',
      text: JSON.stringify({ rendererId }),
    }],
  };
}

/**
 * 渲染场景 / Render scene
 */
export async function handleRenderScene(args: any): Promise<CallToolResult> {
  const renderer = store.getRenderer(args.rendererId);
  if (!renderer) {
    throw new Error(`Renderer not found: ${args.rendererId}`);
  }

  const scene = store.getScene(args.sceneId);
  if (!scene) {
    throw new Error(`Scene not found: ${args.sceneId}`);
  }

  const camera = store.getCamera(args.cameraId);
  if (!camera) {
    throw new Error(`Camera not found: ${args.cameraId}`);
  }

  renderer.render(scene, camera);

  return {
    content: [{
      type: 'text',
      text: JSON.stringify({ success: true }),
    }],
  };
}

/**
 * 获取渲染器画布 / Get renderer canvas
 */
export async function handleGetRendererCanvas(args: any): Promise<CallToolResult> {
  const renderer = store.getRenderer(args.rendererId);
  if (!renderer) {
    throw new Error(`Renderer not found: ${args.rendererId}`);
  }

  const imageData = canvasToBase64(renderer);

  return {
    content: [{
      type: 'text',
      text: JSON.stringify({ image: imageData }),
    }],
  };
}

/**
 * 设置渲染器尺寸 / Set renderer size
 */
export async function handleSetRendererSize(args: any): Promise<CallToolResult> {
  const renderer = store.getRenderer(args.rendererId);
  if (!renderer) {
    throw new Error(`Renderer not found: ${args.rendererId}`);
  }

  renderer.setSize(args.width, args.height);

  return {
    content: [{
      type: 'text',
      text: JSON.stringify({ success: true }),
    }],
  };
}

