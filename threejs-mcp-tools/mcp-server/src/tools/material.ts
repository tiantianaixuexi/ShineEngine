/**
 * Material creation tools
 * 材质创建工具
 */

import * as THREE from 'three';
import { store } from '../state/store.js';
import { generateMaterialId } from '../utils/id-manager.js';
import { parseColor, parseMaterialSide } from '../utils/helpers.js';
import type { CallToolResult } from '@modelcontextprotocol/sdk/types.js';

/**
 * 创建基础材质 / Create basic material
 */
export async function handleCreateBasicMaterial(args: any): Promise<CallToolResult> {
  const material = new THREE.MeshBasicMaterial({
    color: parseColor(args.color || '#ffffff'),
    opacity: args.opacity !== undefined ? args.opacity : 1,
    transparent: args.transparent || false,
    side: parseMaterialSide(args.side || 'front'),
    wireframe: args.wireframe || false,
  });

  const materialId = generateMaterialId();
  store.materials.set(materialId, material);

  return {
    content: [{
      type: 'text',
      text: JSON.stringify({ materialId }),
    }],
  };
}

/**
 * 创建标准PBR材质 / Create standard PBR material
 */
export async function handleCreateStandardMaterial(args: any): Promise<CallToolResult> {
  const material = new THREE.MeshStandardMaterial({
    color: parseColor(args.color || '#ffffff'),
    roughness: args.roughness !== undefined ? args.roughness : 1,
    metalness: args.metalness !== undefined ? args.metalness : 0,
    opacity: args.opacity !== undefined ? args.opacity : 1,
    transparent: args.transparent || false,
    side: parseMaterialSide(args.side || 'front'),
  });

  const materialId = generateMaterialId();
  store.materials.set(materialId, material);

  return {
    content: [{
      type: 'text',
      text: JSON.stringify({ materialId }),
    }],
  };
}

/**
 * 创建物理材质 / Create physical material
 */
export async function handleCreatePhysicalMaterial(args: any): Promise<CallToolResult> {
  const material = new THREE.MeshPhysicalMaterial({
    color: parseColor(args.color || '#ffffff'),
    roughness: args.roughness !== undefined ? args.roughness : 1,
    metalness: args.metalness !== undefined ? args.metalness : 0,
    clearcoat: args.clearcoat !== undefined ? args.clearcoat : 0,
    clearcoatRoughness: args.clearcoatRoughness !== undefined ? args.clearcoatRoughness : 0,
    ior: args.ior !== undefined ? args.ior : 1.5,
    transmission: args.transmission !== undefined ? args.transmission : 0,
    opacity: args.opacity !== undefined ? args.opacity : 1,
    transparent: args.transparent || false,
  });

  const materialId = generateMaterialId();
  store.materials.set(materialId, material);

  return {
    content: [{
      type: 'text',
      text: JSON.stringify({ materialId }),
    }],
  };
}

/**
 * 创建Lambert材质 / Create Lambert material
 */
export async function handleCreateLambertMaterial(args: any): Promise<CallToolResult> {
  const material = new THREE.MeshLambertMaterial({
    color: parseColor(args.color || '#ffffff'),
    opacity: args.opacity !== undefined ? args.opacity : 1,
    transparent: args.transparent || false,
  });

  const materialId = generateMaterialId();
  store.materials.set(materialId, material);

  return {
    content: [{
      type: 'text',
      text: JSON.stringify({ materialId }),
    }],
  };
}

/**
 * 创建Phong材质 / Create Phong material
 */
export async function handleCreatePhongMaterial(args: any): Promise<CallToolResult> {
  const material = new THREE.MeshPhongMaterial({
    color: parseColor(args.color || '#ffffff'),
    specular: parseColor(args.specular || '#111111'),
    shininess: args.shininess !== undefined ? args.shininess : 30,
    opacity: args.opacity !== undefined ? args.opacity : 1,
    transparent: args.transparent || false,
  });

  const materialId = generateMaterialId();
  store.materials.set(materialId, material);

  return {
    content: [{
      type: 'text',
      text: JSON.stringify({ materialId }),
    }],
  };
}

/**
 * 创建线材质 / Create line material
 */
export async function handleCreateLineMaterial(args: any): Promise<CallToolResult> {
  const MaterialClass = args.dashed ? THREE.LineDashedMaterial : THREE.LineBasicMaterial;
  const materialOptions: any = {
    color: parseColor(args.color || '#ffffff'),
    linewidth: args.linewidth || 1,
  };

  if (args.dashed) {
    materialOptions.dashSize = args.dashSize || 3;
    materialOptions.gapSize = args.gapSize || 1;
  }

  const material = new MaterialClass(materialOptions);

  const materialId = generateMaterialId();
  store.materials.set(materialId, material);

  return {
    content: [{
      type: 'text',
      text: JSON.stringify({ materialId }),
    }],
  };
}

/**
 * 创建点材质 / Create points material
 */
export async function handleCreatePointsMaterial(args: any): Promise<CallToolResult> {
  const material = new THREE.PointsMaterial({
    color: parseColor(args.color || '#ffffff'),
    size: args.size || 1,
    sizeAttenuation: args.sizeAttenuation !== undefined ? args.sizeAttenuation : true,
    opacity: args.opacity !== undefined ? args.opacity : 1,
    transparent: args.transparent || false,
  });

  const materialId = generateMaterialId();
  store.materials.set(materialId, material);

  return {
    content: [{
      type: 'text',
      text: JSON.stringify({ materialId }),
    }],
  };
}

/**
 * 为材质设置纹理 / Set texture for material
 */
export async function handleSetMaterialTexture(args: any): Promise<CallToolResult> {
  const material = store.getMaterial(args.materialId);
  if (!material) {
    throw new Error(`Material not found: ${args.materialId}`);
  }

  const texture = store.getTexture(args.textureId);
  if (!texture) {
    throw new Error(`Texture not found: ${args.textureId}`);
  }

  // 根据mapType设置不同的纹理属性 / Set different texture properties based on mapType
  switch (args.mapType) {
    case 'map':
      (material as any).map = texture;
      break;
    case 'normalMap':
      (material as any).normalMap = texture;
      break;
    case 'roughnessMap':
      (material as any).roughnessMap = texture;
      break;
    case 'metalnessMap':
      (material as any).metalnessMap = texture;
      break;
    case 'aoMap':
      (material as any).aoMap = texture;
      break;
    case 'emissiveMap':
      (material as any).emissiveMap = texture;
      break;
    case 'bumpMap':
      (material as any).bumpMap = texture;
      break;
    case 'displacementMap':
      (material as any).displacementMap = texture;
      break;
    default:
      (material as any).map = texture;
  }

  material.needsUpdate = true;

  return {
    content: [{
      type: 'text',
      text: JSON.stringify({ success: true }),
    }],
  };
}

