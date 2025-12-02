/**
 * ID Manager for generating unique IDs
 * ID管理器：生成唯一标识符
 */

import { v4 as uuidv4 } from 'uuid';

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
 * 生成对象ID / Generate object ID
 */
export function generateObjectId(): string {
  return `object_${uuidv4()}`;
}

