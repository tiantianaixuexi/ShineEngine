/**
 * Texture creation and manipulation tools
 * 纹理创建和操作工具
 */

import * as THREE from 'three';
import { store } from '../state/store.js';
import { generateTextureId } from '../utils/id-manager.js';
import { parseWrapMode, parseMagFilter, parseMinFilter, parseTextureFormat } from '../utils/helpers.js';
import type { CallToolResult } from '@modelcontextprotocol/sdk/types.js';

/**
 * 从URL加载纹理 / Load texture from URL
 */
export async function handleLoadTexture(args: any): Promise<CallToolResult> {
  return new Promise((resolve, reject) => {
    const loader = new THREE.TextureLoader();
    
    loader.load(
      args.url,
      (texture) => {
        // 设置包裹模式 / Set wrap modes
        texture.wrapS = parseWrapMode(args.wrapS || 'clamp');
        texture.wrapT = parseWrapMode(args.wrapT || 'clamp');
        
        // 设置过滤模式 / Set filter modes
        texture.magFilter = parseMagFilter(args.magFilter || 'linear');
        texture.minFilter = parseMinFilter(args.minFilter || 'linearMipmapLinear');
        
        const textureId = generateTextureId();
        store.textures.set(textureId, texture);

        resolve({
          content: [{
            type: 'text',
            text: JSON.stringify({ textureId }),
          }],
        });
      },
      undefined,
      (error) => {
        reject(new Error(`Failed to load texture: ${error.message}`));
      }
    );
  });
}

/**
 * 创建数据纹理 / Create data texture
 */
export async function handleCreateDataTexture(args: any): Promise<CallToolResult> {
  const { data, width, height, format } = args;

  if (!Array.isArray(data) || data.length !== width * height * 4) {
    throw new Error(`Invalid data array. Expected ${width * height * 4} elements for RGBA format.`);
  }

  const typedArray = new Uint8Array(data);
  const texture = new THREE.DataTexture(
    typedArray,
    width,
    height,
    parseTextureFormat(format || 'rgba')
  );

  texture.needsUpdate = true;

  const textureId = generateTextureId();
  store.textures.set(textureId, texture);

  return {
    content: [{
      type: 'text',
      text: JSON.stringify({ textureId }),
    }],
  };
}

