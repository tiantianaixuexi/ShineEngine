#!/usr/bin/env node
/**
 * Three.js MCP Lite Server - Code Generator
 * Three.js MCP轻量级服务器 - 代码生成器版本
 */
import { Server } from '@modelcontextprotocol/sdk/server/index.js';
import { StdioServerTransport } from '@modelcontextprotocol/sdk/server/stdio.js';
import { CallToolRequestSchema, ListToolsRequestSchema, } from '@modelcontextprotocol/sdk/types.js';
import { dirname } from 'path';
import { fileURLToPath } from 'url';
// Import tool handlers
import * as GeometryCodeTools from './tools/geometry-code.js';
const __filename = fileURLToPath(import.meta.url);
const __dirname = dirname(__filename);
// Load lite tools definition
const liteToolsDefinition = {
    tools: [
        {
            "name": "three_create_box_geometry",
            "description": "生成立方体几何体的Three.js代码。Generate Three.js code for box geometry.",
            "inputSchema": {
                "type": "object",
                "properties": {
                    "width": {
                        "type": "number",
                        "description": "立方体宽度。Box width.",
                        "default": 1
                    },
                    "height": {
                        "type": "number",
                        "description": "立方体高度。Box height.",
                        "default": 1
                    },
                    "depth": {
                        "type": "number",
                        "description": "立方体深度。Box depth.",
                        "default": 1
                    },
                    "widthSegments": {
                        "type": "number",
                        "description": "宽度分段数。Number of width segments.",
                        "default": 1
                    },
                    "heightSegments": {
                        "type": "number",
                        "description": "高度分段数。Number of height segments.",
                        "default": 1
                    },
                    "depthSegments": {
                        "type": "number",
                        "description": "深度分段数。Number of depth segments.",
                        "default": 1
                    }
                },
                "required": []
            }
        },
        {
            "name": "three_create_sphere_geometry",
            "description": "生成球体几何体的Three.js代码。Generate Three.js code for sphere geometry.",
            "inputSchema": {
                "type": "object",
                "properties": {
                    "radius": {
                        "type": "number",
                        "description": "球体半径。Sphere radius.",
                        "default": 1
                    },
                    "widthSegments": {
                        "type": "number",
                        "description": "水平分段数。Number of horizontal segments.",
                        "default": 32
                    },
                    "heightSegments": {
                        "type": "number",
                        "description": "垂直分段数。Number of vertical segments.",
                        "default": 16
                    }
                },
                "required": []
            }
        },
        {
            "name": "three_create_plane_geometry",
            "description": "生成平面几何体的Three.js代码。Generate Three.js code for plane geometry.",
            "inputSchema": {
                "type": "object",
                "properties": {
                    "width": {
                        "type": "number",
                        "description": "平面宽度。Plane width.",
                        "default": 1
                    },
                    "height": {
                        "type": "number",
                        "description": "平面高度。Plane height.",
                        "default": 1
                    }
                },
                "required": []
            }
        },
        {
            "name": "three_create_cylinder_geometry",
            "description": "生成圆柱体几何体的Three.js代码。Generate Three.js code for cylinder geometry.",
            "inputSchema": {
                "type": "object",
                "properties": {
                    "radiusTop": {
                        "type": "number",
                        "description": "顶部半径。Top radius.",
                        "default": 1
                    },
                    "radiusBottom": {
                        "type": "number",
                        "description": "底部半径。Bottom radius.",
                        "default": 1
                    },
                    "height": {
                        "type": "number",
                        "description": "圆柱体高度。Cylinder height.",
                        "default": 1
                    }
                },
                "required": []
            }
        },
        {
            "name": "three_create_cone_geometry",
            "description": "生成圆锥体几何体的Three.js代码。Generate Three.js code for cone geometry.",
            "inputSchema": {
                "type": "object",
                "properties": {
                    "radius": {
                        "type": "number",
                        "description": "底部半径。Bottom radius.",
                        "default": 1
                    },
                    "height": {
                        "type": "number",
                        "description": "圆锥体高度。Cone height.",
                        "default": 1
                    }
                },
                "required": []
            }
        },
        {
            "name": "three_create_torus_geometry",
            "description": "生成圆环几何体的Three.js代码。Generate Three.js code for torus geometry.",
            "inputSchema": {
                "type": "object",
                "properties": {
                    "radius": {
                        "type": "number",
                        "description": "圆环半径（从中心到管道中心）。Torus radius (from center to tube center).",
                        "default": 1
                    },
                    "tube": {
                        "type": "number",
                        "description": "管道半径。Tube radius.",
                        "default": 0.4
                    }
                },
                "required": []
            }
        },
        {
            "name": "three_generate_scene_code",
            "description": "生成完整的Three.js场景代码片段。Generate complete Three.js scene code snippet.",
            "inputSchema": {
                "type": "object",
                "properties": {
                    "includeSetup": {
                        "type": "boolean",
                        "description": "是否包含场景设置代码。Whether to include scene setup code.",
                        "default": true
                    },
                    "includeRenderer": {
                        "type": "boolean",
                        "description": "是否包含渲染器代码。Whether to include renderer code.",
                        "default": true
                    },
                    "includeAnimation": {
                        "type": "boolean",
                        "description": "是否包含动画循环代码。Whether to include animation loop code.",
                        "default": false
                    }
                },
                "required": []
            }
        }
    ]
};
// Create MCP server
const server = new Server({
    name: 'threejs-mcp-lite',
    version: '1.0.0',
}, {
    capabilities: {
        tools: {},
    },
});
// List tools handler
server.setRequestHandler(ListToolsRequestSchema, async () => {
    return {
        tools: liteToolsDefinition.tools,
    };
});
// Call tool handler
server.setRequestHandler(CallToolRequestSchema, async (request) => {
    const { name, arguments: args } = request.params;
    try {
        switch (name) {
            // Geometry code generation / 几何体代码生成
            case 'three_create_box_geometry':
                return await GeometryCodeTools.handleCreateBoxGeometry(args);
            case 'three_create_sphere_geometry':
                return await GeometryCodeTools.handleCreateSphereGeometry(args);
            case 'three_create_plane_geometry':
                return await GeometryCodeTools.handleCreatePlaneGeometry(args);
            case 'three_create_cylinder_geometry':
                return await GeometryCodeTools.handleCreateCylinderGeometry(args);
            case 'three_create_cone_geometry':
                return await GeometryCodeTools.handleCreateConeGeometry(args);
            case 'three_create_torus_geometry':
                return await GeometryCodeTools.handleCreateTorusGeometry(args);
            // Scene code generation / 场景代码生成
            case 'three_generate_scene_code':
                return await handleGenerateSceneCode(args);
            default:
                throw new Error(`Unknown tool: ${name}`);
        }
    }
    catch (error) {
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
/**
 * Generate complete scene code snippet
 * 生成完整的场景代码片段
 */
async function handleGenerateSceneCode(args) {
    const { includeSetup = true, includeRenderer = true, includeAnimation = false } = args;
    let code = '';
    if (includeSetup) {
        code += `// Scene setup
const scene = new THREE.Scene();
scene.background = new THREE.Color(0x87CEEB); // Sky blue background

// Camera setup
const camera = new THREE.PerspectiveCamera(
  75,
  window.innerWidth / window.innerHeight,
  0.1,
  1000
);
camera.position.set(0, 0, 5);

// Lighting setup
const ambientLight = new THREE.AmbientLight(0xffffff, 0.5);
scene.add(ambientLight);

const directionalLight = new THREE.DirectionalLight(0xffffff, 1);
directionalLight.position.set(10, 10, 5);
scene.add(directionalLight);

`;
    }
    if (includeRenderer) {
        code += `// Renderer setup
const renderer = new THREE.WebGLRenderer({ antialias: true });
renderer.setSize(window.innerWidth, window.innerHeight);
renderer.shadowMap.enabled = true;
renderer.shadowMap.type = THREE.PCFSoftShadowMap;
document.body.appendChild(renderer.domElement);

`;
    }
    if (includeAnimation) {
        code += `// Animation loop
function animate() {
  requestAnimationFrame(animate);

  // Add your animation logic here
  // 在这里添加动画逻辑

  renderer.render(scene, camera);
}
animate();

// Handle window resize
window.addEventListener('resize', () => {
  camera.aspect = window.innerWidth / window.innerHeight;
  camera.updateProjectionMatrix();
  renderer.setSize(window.innerWidth, window.innerHeight);
});

`;
    }
    else {
        code += `// Initial render
renderer.render(scene, camera);

// Handle window resize
window.addEventListener('resize', () => {
  camera.aspect = window.innerWidth / window.innerHeight;
  camera.updateProjectionMatrix();
  renderer.setSize(window.innerWidth, window.innerHeight);
  renderer.render(scene, camera);
});

`;
    }
    code += `// Add objects to scene here
// 在这里添加对象到场景
// scene.add(yourObject);

`;
    return {
        content: [{
                type: 'text',
                text: JSON.stringify({
                    code: code.trim(),
                    description: 'Complete Three.js scene setup code',
                    requires: ['THREE (from CDN or npm)']
                }),
            }],
    };
}
// Start server
async function main() {
    const transport = new StdioServerTransport();
    await server.connect(transport);
    console.error('Three.js MCP Lite Server (Code Generator) started');
}
main().catch((error) => {
    console.error('Fatal error:', error);
    process.exit(1);
});
//# sourceMappingURL=index.js.map