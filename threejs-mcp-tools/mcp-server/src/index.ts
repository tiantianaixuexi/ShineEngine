#!/usr/bin/env node

/**
 * Three.js MCP Server
 * Three.js MCP服务器主文件
 */

import { Server } from '@modelcontextprotocol/sdk/server/index.js';
import { StdioServerTransport } from '@modelcontextprotocol/sdk/server/stdio.js';
import {
  CallToolRequestSchema,
  ListToolsRequestSchema,
  type CallToolResult,
} from '@modelcontextprotocol/sdk/types.js';
import { readFileSync } from 'fs';
import { join, dirname } from 'path';
import { fileURLToPath } from 'url';

// Import tool handlers
import * as SceneTools from './tools/scene.js';
import * as GeometryTools from './tools/geometry.js';
import * as MaterialTools from './tools/material.js';
import * as ObjectTools from './tools/object.js';
import * as LightTools from './tools/light.js';
import * as CameraTools from './tools/camera.js';
import * as RendererTools from './tools/renderer.js';
import * as TextureTools from './tools/texture.js';

// Load tools from category files
function loadToolsFromCategories() {
  const categories = [
    'scene',
    'geometry',
    'material',
    'object',
    'light',
    'camera',
    'renderer',
    'texture'
  ];

  const allTools = [];
  const __filename = fileURLToPath(import.meta.url);
  const __dirname = dirname(__filename);

  for (const category of categories) {
    const filePath = join(__dirname, '../../definitions', `${category}.json`);
    try {
      const categoryData = JSON.parse(readFileSync(filePath, 'utf-8'));
      if (categoryData.tools && Array.isArray(categoryData.tools)) {
        // Add category metadata to each tool
        const toolsWithCategory = categoryData.tools.map((tool: any) => ({
          ...tool,
          category: categoryData.category || category,
          categoryName: categoryData.name || category,
        }));
        allTools.push(...toolsWithCategory);
      }
    } catch (error: any) {
      console.error(`Failed to load tools from ${category}.json:`, error.message);
    }
  }

  return { tools: allTools };
}

// Load tools definition
const toolsDefinition = loadToolsFromCategories();

// Create MCP server
const server = new Server(
  {
    name: 'threejs-mcp-server',
    version: '1.0.0',
  },
  {
    capabilities: {
      tools: {},
    },
  }
);

// List tools handler
server.setRequestHandler(ListToolsRequestSchema, async () => {
  return {
    tools: toolsDefinition.tools,
  };
});

// Call tool handler
server.setRequestHandler(CallToolRequestSchema, async (request) => {
  const { name, arguments: args } = request.params;

  try {
    switch (name) {
      // Scene tools / 场景工具
      case 'three_create_scene':
        return await SceneTools.handleCreateScene(args);
      case 'three_add_object_to_scene':
        return await SceneTools.handleAddObjectToScene(args);
      case 'three_remove_object_from_scene':
        return await SceneTools.handleRemoveObjectFromScene(args);
      case 'three_get_scene_objects':
        return await SceneTools.handleGetSceneObjects(args);
      case 'three_set_scene_background':
        return await SceneTools.handleSetSceneBackground(args);
      case 'three_export_scene_json':
        return await SceneTools.handleExportSceneJson(args);

      // Geometry tools / 几何体工具
      case 'three_create_box_geometry':
        return await GeometryTools.handleCreateBoxGeometry(args);
      case 'three_create_sphere_geometry':
        return await GeometryTools.handleCreateSphereGeometry(args);
      case 'three_create_plane_geometry':
        return await GeometryTools.handleCreatePlaneGeometry(args);
      case 'three_create_cylinder_geometry':
        return await GeometryTools.handleCreateCylinderGeometry(args);
      case 'three_create_cone_geometry':
        return await GeometryTools.handleCreateConeGeometry(args);
      case 'three_create_torus_geometry':
        return await GeometryTools.handleCreateTorusGeometry(args);
      case 'three_create_torus_knot_geometry':
        return await GeometryTools.handleCreateTorusKnotGeometry(args);
      case 'three_create_capsule_geometry':
        return await GeometryTools.handleCreateCapsuleGeometry(args);
      case 'three_create_circle_geometry':
        return await GeometryTools.handleCreateCircleGeometry(args);
      case 'three_create_ring_geometry':
        return await GeometryTools.handleCreateRingGeometry(args);
      case 'three_create_tetrahedron_geometry':
        return await GeometryTools.handleCreateTetrahedronGeometry(args);
      case 'three_create_octahedron_geometry':
        return await GeometryTools.handleCreateOctahedronGeometry(args);
      case 'three_create_icosahedron_geometry':
        return await GeometryTools.handleCreateIcosahedronGeometry(args);
      case 'three_create_dodecahedron_geometry':
        return await GeometryTools.handleCreateDodecahedronGeometry(args);

      // Material tools / 材质工具
      case 'three_create_basic_material':
        return await MaterialTools.handleCreateBasicMaterial(args);
      case 'three_create_standard_material':
        return await MaterialTools.handleCreateStandardMaterial(args);
      case 'three_create_physical_material':
        return await MaterialTools.handleCreatePhysicalMaterial(args);
      case 'three_create_lambert_material':
        return await MaterialTools.handleCreateLambertMaterial(args);
      case 'three_create_phong_material':
        return await MaterialTools.handleCreatePhongMaterial(args);
      case 'three_create_line_material':
        return await MaterialTools.handleCreateLineMaterial(args);
      case 'three_create_points_material':
        return await MaterialTools.handleCreatePointsMaterial(args);
      case 'three_set_material_texture':
        return await MaterialTools.handleSetMaterialTexture(args);

      // Object tools / 对象工具
      case 'three_create_mesh':
        return await ObjectTools.handleCreateMesh(args);
      case 'three_create_line':
        return await ObjectTools.handleCreateLine(args);
      case 'three_create_points':
        return await ObjectTools.handleCreatePoints(args);
      case 'three_set_object_position':
        return await ObjectTools.handleSetObjectPosition(args);
      case 'three_set_object_rotation':
        return await ObjectTools.handleSetObjectRotation(args);
      case 'three_set_object_scale':
        return await ObjectTools.handleSetObjectScale(args);
      case 'three_get_object_info':
        return await ObjectTools.handleGetObjectInfo(args);
      case 'three_clone_object':
        return await ObjectTools.handleCloneObject(args);
      case 'three_dispose_object':
        return await ObjectTools.handleDisposeObject(args);

      // Light tools / 光源工具
      case 'three_create_ambient_light':
        return await LightTools.handleCreateAmbientLight(args);
      case 'three_create_directional_light':
        return await LightTools.handleCreateDirectionalLight(args);
      case 'three_create_point_light':
        return await LightTools.handleCreatePointLight(args);
      case 'three_create_spot_light':
        return await LightTools.handleCreateSpotLight(args);
      case 'three_create_hemisphere_light':
        return await LightTools.handleCreateHemisphereLight(args);
      case 'three_create_rect_area_light':
        return await LightTools.handleCreateRectAreaLight(args);
      case 'three_set_light_color':
        return await LightTools.handleSetLightColor(args);
      case 'three_set_light_intensity':
        return await LightTools.handleSetLightIntensity(args);
      case 'three_set_light_position':
        return await LightTools.handleSetLightPosition(args);

      // Camera tools / 相机工具
      case 'three_create_perspective_camera':
        return await CameraTools.handleCreatePerspectiveCamera(args);
      case 'three_create_orthographic_camera':
        return await CameraTools.handleCreateOrthographicCamera(args);
      case 'three_set_camera_position':
        return await CameraTools.handleSetCameraPosition(args);
      case 'three_set_camera_look_at':
        return await CameraTools.handleSetCameraLookAt(args);
      case 'three_set_camera_fov':
        return await CameraTools.handleSetCameraFov(args);

      // Renderer tools / 渲染器工具
      case 'three_create_renderer':
        return await RendererTools.handleCreateRenderer(args);
      case 'three_render_scene':
        return await RendererTools.handleRenderScene(args);
      case 'three_get_renderer_canvas':
        return await RendererTools.handleGetRendererCanvas(args);
      case 'three_set_renderer_size':
        return await RendererTools.handleSetRendererSize(args);

      // Texture tools / 纹理工具
      case 'three_load_texture':
        return await TextureTools.handleLoadTexture(args);
      case 'three_create_data_texture':
        return await TextureTools.handleCreateDataTexture(args);

      default:
        throw new Error(`Unknown tool: ${name}`);
    }
  } catch (error: any) {
    return {
      content: [{
        type: 'text',
        text: JSON.stringify({
          error: error.message || 'Unknown error',
          stack: error.stack,
        }),
      }],
      isError: true,
    };
  }
});

// Start server
async function main() {
  const transport = new StdioServerTransport();
  await server.connect(transport);
  console.error('Three.js MCP Server started');
}

main().catch((error) => {
  console.error('Fatal error:', error);
  process.exit(1);
});

