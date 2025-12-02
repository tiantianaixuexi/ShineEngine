/**
 * ID Manager for generating unique IDs
 * ID管理器：生成唯一标识符
 */

import { v4 as uuidv4 } from 'uuid';

/**
 * 生成场景ID / Generate scene ID
 */
export function generateSceneId(): string {
  return `scene_${uuidv4()}`;
}

/**
 * 生成对象ID / Generate object ID
 */
export function generateObjectId(): string {
  return `object_${uuidv4()}`;
}

/**
 * 生成几何体ID / Generate geometry ID
 */
export function generateGeometryId(): string {
  return `geometry_${uuidv4()}`;
}

/**
 * 生成材质ID / Generate material ID
 */
export function generateMaterialId(): string {
  return `material_${uuidv4()}`;
}

/**
 * 生成光源ID / Generate light ID
 */
export function generateLightId(): string {
  return `light_${uuidv4()}`;
}

/**
 * 生成相机ID / Generate camera ID
 */
export function generateCameraId(): string {
  return `camera_${uuidv4()}`;
}

/**
 * 生成渲染器ID / Generate renderer ID
 */
export function generateRendererId(): string {
  return `renderer_${uuidv4()}`;
}

/**
 * 生成纹理ID / Generate texture ID
 */
export function generateTextureId(): string {
  return `texture_${uuidv4()}`;
}

