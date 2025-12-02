/**
 * Geometry code generation tools
 * 几何体代码生成工具（不依赖Three.js运行时）
 */

import { generateGeometryId } from '../utils/id-manager.js';
import type { CallToolResult } from '@modelcontextprotocol/sdk/types.js';

// Three.js geometry code templates
const GEOMETRY_TEMPLATES = {
  box: (width: number, height: number, depth: number, widthSegments: number, heightSegments: number, depthSegments: number) => `
const geometry = new THREE.BoxGeometry(
  ${width}, ${height}, ${depth},
  ${widthSegments}, ${heightSegments}, ${depthSegments}
);`,

  sphere: (radius: number, widthSegments: number, heightSegments: number, phiStart: number, phiLength: number, thetaStart: number, thetaLength: number) => `
const geometry = new THREE.SphereGeometry(
  ${radius}, ${widthSegments}, ${heightSegments},
  ${phiStart}, ${phiLength}, ${thetaStart}, ${thetaLength}
);`,

  plane: (width: number, height: number, widthSegments: number, heightSegments: number) => `
const geometry = new THREE.PlaneGeometry(
  ${width}, ${height}, ${widthSegments}, ${heightSegments}
);`,

  cylinder: (radiusTop: number, radiusBottom: number, height: number, radialSegments: number, heightSegments: number, openEnded: boolean, thetaStart: number, thetaLength: number) => `
const geometry = new THREE.CylinderGeometry(
  ${radiusTop}, ${radiusBottom}, ${height}, ${radialSegments}, ${heightSegments},
  ${openEnded}, ${thetaStart}, ${thetaLength}
);`,

  torus: (radius: number, tube: number, radialSegments: number, tubularSegments: number, arc: number) => `
const geometry = new THREE.TorusGeometry(
  ${radius}, ${tube}, ${radialSegments}, ${tubularSegments}, ${arc}
);`,

  cone: (radius: number, height: number, radialSegments: number, heightSegments: number, openEnded: boolean, thetaStart: number, thetaLength: number) => `
const geometry = new THREE.ConeGeometry(
  ${radius}, ${height}, ${radialSegments}, ${heightSegments},
  ${openEnded}, ${thetaStart}, ${thetaLength}
);`
};

/**
 * Generate box geometry code
 */
export async function handleCreateBoxGeometry(args: any): Promise<CallToolResult> {
  const code = GEOMETRY_TEMPLATES.box(
    args.width || 1,
    args.height || 1,
    args.depth || 1,
    args.widthSegments || 1,
    args.heightSegments || 1,
    args.depthSegments || 1
  );

  const geometryId = generateGeometryId();

  return {
    content: [{
      type: 'text',
      text: JSON.stringify({
        geometryId,
        code: code.trim(),
        description: 'Box geometry creation code'
      }),
    }],
  };
}

/**
 * Generate sphere geometry code
 */
export async function handleCreateSphereGeometry(args: any): Promise<CallToolResult> {
  const code = GEOMETRY_TEMPLATES.sphere(
    args.radius || 1,
    args.widthSegments || 32,
    args.heightSegments || 16,
    args.phiStart || 0,
    args.phiLength || Math.PI * 2,
    args.thetaStart || 0,
    args.thetaLength || Math.PI
  );

  const geometryId = generateGeometryId();

  return {
    content: [{
      type: 'text',
      text: JSON.stringify({
        geometryId,
        code: code.trim(),
        description: 'Sphere geometry creation code'
      }),
    }],
  };
}

/**
 * Generate plane geometry code
 */
export async function handleCreatePlaneGeometry(args: any): Promise<CallToolResult> {
  const code = GEOMETRY_TEMPLATES.plane(
    args.width || 1,
    args.height || 1,
    args.widthSegments || 1,
    args.heightSegments || 1
  );

  const geometryId = generateGeometryId();

  return {
    content: [{
      type: 'text',
      text: JSON.stringify({
        geometryId,
        code: code.trim(),
        description: 'Plane geometry creation code'
      }),
    }],
  };
}

/**
 * Generate cylinder geometry code
 */
export async function handleCreateCylinderGeometry(args: any): Promise<CallToolResult> {
  const code = GEOMETRY_TEMPLATES.cylinder(
    args.radiusTop || 1,
    args.radiusBottom || 1,
    args.height || 1,
    args.radialSegments || 32,
    args.heightSegments || 1,
    args.openEnded || false,
    args.thetaStart || 0,
    args.thetaLength || Math.PI * 2
  );

  const geometryId = generateGeometryId();

  return {
    content: [{
      type: 'text',
      text: JSON.stringify({
        geometryId,
        code: code.trim(),
        description: 'Cylinder geometry creation code'
      }),
    }],
  };
}

/**
 * Generate cone geometry code
 */
export async function handleCreateConeGeometry(args: any): Promise<CallToolResult> {
  const code = GEOMETRY_TEMPLATES.cone(
    args.radius || 1,
    args.height || 1,
    args.radialSegments || 32,
    args.heightSegments || 1,
    args.openEnded || false,
    args.thetaStart || 0,
    args.thetaLength || Math.PI * 2
  );

  const geometryId = generateGeometryId();

  return {
    content: [{
      type: 'text',
      text: JSON.stringify({
        geometryId,
        code: code.trim(),
        description: 'Cone geometry creation code'
      }),
    }],
  };
}

/**
 * Generate torus geometry code
 */
export async function handleCreateTorusGeometry(args: any): Promise<CallToolResult> {
  const code = GEOMETRY_TEMPLATES.torus(
    args.radius || 1,
    args.tube || 0.4,
    args.radialSegments || 16,
    args.tubularSegments || 100,
    args.arc || Math.PI * 2
  );

  const geometryId = generateGeometryId();

  return {
    content: [{
      type: 'text',
      text: JSON.stringify({
        geometryId,
        code: code.trim(),
        description: 'Torus geometry creation code'
      }),
    }],
  };
}

