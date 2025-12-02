/**
 * Script to split mcp-tools.json into category-based files
 * 将mcp-tools.json按类别拆分为多个文件的脚本
 */

import { readFileSync, writeFileSync } from 'fs';
import { join, dirname } from 'path';
import { fileURLToPath } from 'url';

const __filename = fileURLToPath(import.meta.url);
const __dirname = dirname(__filename);

// Read the original tools file
const toolsFile = readFileSync(join(__dirname, '../mcp-tools.json'), 'utf-8');
const allTools = JSON.parse(toolsFile);

// Define tool categories
const categories = {
  scene: {
    name: '场景管理工具',
    description: 'Scene Management Tools',
    tools: []
  },
  geometry: {
    name: '几何体工具',
    description: 'Geometry Tools',
    tools: []
  },
  material: {
    name: '材质工具',
    description: 'Material Tools',
    tools: []
  },
  object: {
    name: '对象工具',
    description: 'Object Tools',
    tools: []
  },
  light: {
    name: '光源工具',
    description: 'Light Tools',
    tools: []
  },
  camera: {
    name: '相机工具',
    description: 'Camera Tools',
    tools: []
  },
  renderer: {
    name: '渲染器工具',
    description: 'Renderer Tools',
    tools: []
  },
  texture: {
    name: '纹理工具',
    description: 'Texture Tools',
    tools: []
  }
};

// Categorize tools
allTools.tools.forEach(tool => {
  const name = tool.name;
  if (name.startsWith('three_create_scene') || name.startsWith('three_add_object') || 
      name.startsWith('three_remove_object') || name.startsWith('three_get_scene') || 
      name.startsWith('three_set_scene') || name.startsWith('three_export_scene')) {
    categories.scene.tools.push(tool);
  } else if (name.includes('geometry')) {
    categories.geometry.tools.push(tool);
  } else if (name.includes('material')) {
    categories.material.tools.push(tool);
  } else if (name.includes('light')) {
    categories.light.tools.push(tool);
  } else if (name.includes('camera')) {
    categories.camera.tools.push(tool);
  } else if (name.includes('renderer')) {
    categories.renderer.tools.push(tool);
  } else if (name.includes('texture')) {
    categories.texture.tools.push(tool);
  } else if (name.includes('mesh') || name.includes('line') || name.includes('points') || 
             name.includes('object') && !name.includes('scene')) {
    categories.object.tools.push(tool);
  }
});

// Write category files
Object.keys(categories).forEach(category => {
  const categoryData = {
    category: category,
    name: categories[category].name,
    description: categories[category].description,
    tools: categories[category].tools
  };
  
  const filePath = join(__dirname, `${category}.json`);
  writeFileSync(filePath, JSON.stringify(categoryData, null, 2), 'utf-8');
  console.log(`✓ Created ${category}.json with ${categories[category].tools.length} tools`);
});

console.log('\n✓ All category files created successfully!');

