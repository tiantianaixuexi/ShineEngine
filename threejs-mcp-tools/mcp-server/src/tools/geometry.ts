/**
 * Geometry creation tools
 * 几何体创建工具
 */

import * as THREE from 'three';
import { store } from '../state/store.js';
import { generateGeometryId } from '../utils/id-manager.js';
import type { CallToolResult } from '@modelcontextprotocol/sdk/types.js';

/**
 * 创建立方体几何体 / Create box geometry
 */
export async function handleCreateBoxGeometry(args: any): Promise<CallToolResult> {
  const geometry = new THREE.BoxGeometry(
    args.width || 1,
    args.height || 1,
    args.depth || 1,
    args.widthSegments || 1,
    args.heightSegments || 1,
    args.depthSegments || 1
  );

  const geometryId = generateGeometryId();
  store.geometries.set(geometryId, geometry);

  return {
    content: [{
      type: 'text',
      text: JSON.stringify({ geometryId }),
    }],
  };
}

/**
 * 创建球体几何体 / Create sphere geometry
 */
export async function handleCreateSphereGeometry(args: any): Promise<CallToolResult> {
  const geometry = new THREE.SphereGeometry(
    args.radius || 1,
    args.widthSegments || 32,
    args.heightSegments || 16,
    args.phiStart || 0,
    args.phiLength || Math.PI * 2,
    args.thetaStart || 0,
    args.thetaLength || Math.PI
  );

  const geometryId = generateGeometryId();
  store.geometries.set(geometryId, geometry);

  return {
    content: [{
      type: 'text',
      text: JSON.stringify({ geometryId }),
    }],
  };
}

/**
 * 创建平面几何体 / Create plane geometry
 */
export async function handleCreatePlaneGeometry(args: any): Promise<CallToolResult> {
  const geometry = new THREE.PlaneGeometry(
    args.width || 1,
    args.height || 1,
    args.widthSegments || 1,
    args.heightSegments || 1
  );

  const geometryId = generateGeometryId();
  store.geometries.set(geometryId, geometry);

  return {
    content: [{
      type: 'text',
      text: JSON.stringify({ geometryId }),
    }],
  };
}

/**
 * 创建圆柱体几何体 / Create cylinder geometry
 */
export async function handleCreateCylinderGeometry(args: any): Promise<CallToolResult> {
  const geometry = new THREE.CylinderGeometry(
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
  store.geometries.set(geometryId, geometry);

  return {
    content: [{
      type: 'text',
      text: JSON.stringify({ geometryId }),
    }],
  };
}

/**
 * 创建圆锥体几何体 / Create cone geometry
 */
export async function handleCreateConeGeometry(args: any): Promise<CallToolResult> {
  const geometry = new THREE.ConeGeometry(
    args.radius || 1,
    args.height || 1,
    args.radialSegments || 32,
    args.heightSegments || 1,
    args.openEnded || false,
    args.thetaStart || 0,
    args.thetaLength || Math.PI * 2
  );

  const geometryId = generateGeometryId();
  store.geometries.set(geometryId, geometry);

  return {
    content: [{
      type: 'text',
      text: JSON.stringify({ geometryId }),
    }],
  };
}

/**
 * 创建圆环几何体 / Create torus geometry
 */
export async function handleCreateTorusGeometry(args: any): Promise<CallToolResult> {
  const geometry = new THREE.TorusGeometry(
    args.radius || 1,
    args.tube || 0.4,
    args.radialSegments || 16,
    args.tubularSegments || 100,
    args.arc || Math.PI * 2
  );

  const geometryId = generateGeometryId();
  store.geometries.set(geometryId, geometry);

  return {
    content: [{
      type: 'text',
      text: JSON.stringify({ geometryId }),
    }],
  };
}

/**
 * 创建圆环结几何体 / Create torus knot geometry
 */
export async function handleCreateTorusKnotGeometry(args: any): Promise<CallToolResult> {
  const geometry = new THREE.TorusKnotGeometry(
    args.radius || 1,
    args.tube || 0.4,
    args.tubularSegments || 64,
    args.radialSegments || 8,
    args.p || 2,
    args.q || 3
  );

  const geometryId = generateGeometryId();
  store.geometries.set(geometryId, geometry);

  return {
    content: [{
      type: 'text',
      text: JSON.stringify({ geometryId }),
    }],
  };
}

/**
 * 创建胶囊体几何体 / Create capsule geometry
 */
export async function handleCreateCapsuleGeometry(args: any): Promise<CallToolResult> {
  const geometry = new THREE.CapsuleGeometry(
    args.radius || 1,
    args.length || 1,
    args.capSubdivisions || 4,
    args.radialSegments || 8
  );

  const geometryId = generateGeometryId();
  store.geometries.set(geometryId, geometry);

  return {
    content: [{
      type: 'text',
      text: JSON.stringify({ geometryId }),
    }],
  };
}

/**
 * 创建圆形几何体 / Create circle geometry
 */
export async function handleCreateCircleGeometry(args: any): Promise<CallToolResult> {
  const geometry = new THREE.CircleGeometry(
    args.radius || 1,
    args.segments || 32,
    args.thetaStart || 0,
    args.thetaLength || Math.PI * 2
  );

  const geometryId = generateGeometryId();
  store.geometries.set(geometryId, geometry);

  return {
    content: [{
      type: 'text',
      text: JSON.stringify({ geometryId }),
    }],
  };
}

/**
 * 创建环形几何体 / Create ring geometry
 */
export async function handleCreateRingGeometry(args: any): Promise<CallToolResult> {
  const geometry = new THREE.RingGeometry(
    args.innerRadius || 0.5,
    args.outerRadius || 1,
    args.thetaSegments || 32,
    args.phiSegments || 1,
    args.thetaStart || 0,
    args.thetaLength || Math.PI * 2
  );

  const geometryId = generateGeometryId();
  store.geometries.set(geometryId, geometry);

  return {
    content: [{
      type: 'text',
      text: JSON.stringify({ geometryId }),
    }],
  };
}

/**
 * 创建四面体几何体 / Create tetrahedron geometry
 */
export async function handleCreateTetrahedronGeometry(args: any): Promise<CallToolResult> {
  const geometry = new THREE.TetrahedronGeometry(
    args.radius || 1,
    args.detail || 0
  );

  const geometryId = generateGeometryId();
  store.geometries.set(geometryId, geometry);

  return {
    content: [{
      type: 'text',
      text: JSON.stringify({ geometryId }),
    }],
  };
}

/**
 * 创建八面体几何体 / Create octahedron geometry
 */
export async function handleCreateOctahedronGeometry(args: any): Promise<CallToolResult> {
  const geometry = new THREE.OctahedronGeometry(
    args.radius || 1,
    args.detail || 0
  );

  const geometryId = generateGeometryId();
  store.geometries.set(geometryId, geometry);

  return {
    content: [{
      type: 'text',
      text: JSON.stringify({ geometryId }),
    }],
  };
}

/**
 * 创建二十面体几何体 / Create icosahedron geometry
 */
export async function handleCreateIcosahedronGeometry(args: any): Promise<CallToolResult> {
  const geometry = new THREE.IcosahedronGeometry(
    args.radius || 1,
    args.detail || 0
  );

  const geometryId = generateGeometryId();
  store.geometries.set(geometryId, geometry);

  return {
    content: [{
      type: 'text',
      text: JSON.stringify({ geometryId }),
    }],
  };
}

/**
 * 创建十二面体几何体 / Create dodecahedron geometry
 */
export async function handleCreateDodecahedronGeometry(args: any): Promise<CallToolResult> {
  const geometry = new THREE.DodecahedronGeometry(
    args.radius || 1,
    args.detail || 0
  );

  const geometryId = generateGeometryId();
  store.geometries.set(geometryId, geometry);

  return {
    content: [{
      type: 'text',
      text: JSON.stringify({ geometryId }),
    }],
  };
}

