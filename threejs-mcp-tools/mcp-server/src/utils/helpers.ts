/**
 * Helper functions for Three.js operations
 * Three.js操作辅助函数
 */

import * as THREE from 'three';

/**
 * 解析颜色字符串 / Parse color string
 */
export function parseColor(color: string): THREE.Color {
  return new THREE.Color(color);
}

/**
 * 解析位置对象 / Parse position object
 */
export function parsePosition(position: { x: number; y: number; z: number }): THREE.Vector3 {
  return new THREE.Vector3(position.x, position.y, position.z);
}

/**
 * 解析旋转对象（欧拉角） / Parse rotation object (Euler angles)
 */
export function parseRotation(rotation: { x: number; y: number; z: number }): THREE.Euler {
  return new THREE.Euler(rotation.x, rotation.y, rotation.z);
}

/**
 * 解析缩放对象 / Parse scale object
 */
export function parseScale(scale: { x: number; y: number; z: number }): THREE.Vector3 {
  return new THREE.Vector3(scale.x, scale.y, scale.z);
}

/**
 * 将纹理包裹模式字符串转换为常量 / Convert wrap mode string to constant
 */
export function parseWrapMode(mode: string): number {
  switch (mode) {
    case 'repeat':
      return THREE.RepeatWrapping;
    case 'clamp':
      return THREE.ClampToEdgeWrapping;
    case 'mirror':
      return THREE.MirroredRepeatWrapping;
    default:
      return THREE.ClampToEdgeWrapping;
  }
}

/**
 * 将过滤模式字符串转换为常量 / Convert filter mode string to constant
 */
export function parseMagFilter(filter: string): number {
  switch (filter) {
    case 'nearest':
      return THREE.NearestFilter;
    case 'linear':
      return THREE.LinearFilter;
    default:
      return THREE.LinearFilter;
  }
}

/**
 * 将缩小过滤模式字符串转换为常量 / Convert minification filter mode string to constant
 */
export function parseMinFilter(filter: string): number {
  switch (filter) {
    case 'nearest':
      return THREE.NearestFilter;
    case 'linear':
      return THREE.LinearFilter;
    case 'nearestMipmapNearest':
      return THREE.NearestMipmapNearestFilter;
    case 'nearestMipmapLinear':
      return THREE.NearestMipmapLinearFilter;
    case 'linearMipmapNearest':
      return THREE.LinearMipmapNearestFilter;
    case 'linearMipmapLinear':
      return THREE.LinearMipmapLinearFilter;
    default:
      return THREE.LinearMipmapLinearFilter;
  }
}

/**
 * 将材质面模式字符串转换为常量 / Convert material side string to constant
 */
export function parseMaterialSide(side: string): number {
  switch (side) {
    case 'front':
      return THREE.FrontSide;
    case 'back':
      return THREE.BackSide;
    case 'double':
      return THREE.DoubleSide;
    default:
      return THREE.FrontSide;
  }
}

/**
 * 将纹理格式字符串转换为常量 / Convert texture format string to constant
 */
export function parseTextureFormat(format: string): number {
  switch (format) {
    case 'rgba':
      return THREE.RGBAFormat;
    case 'rgb':
      return THREE.RGBFormat;
    case 'alpha':
      return THREE.AlphaFormat;
    case 'red':
      return THREE.RedFormat;
    case 'rg':
      return THREE.RGFormat;
    default:
      return THREE.RGBAFormat;
  }
}

/**
 * 将渲染器画布转换为Base64图像 / Convert renderer canvas to Base64 image
 */
export function canvasToBase64(renderer: THREE.WebGLRenderer): string {
  const canvas = renderer.domElement;
  return canvas.toDataURL('image/png');
}

