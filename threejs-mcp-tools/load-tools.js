/**
 * Load and merge all tool definitions from category files
 * 从分类文件中加载并合并所有工具定义
 */

import { readFileSync } from 'fs';
import { join, dirname } from 'path';
import { fileURLToPath } from 'url';

const __filename = fileURLToPath(import.meta.url);
const __dirname = dirname(__filename);

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

export function loadAllTools() {
  const allTools = [];

  for (const category of categories) {
    const filePath = join(__dirname, 'definitions', `${category}.json`);
    try {
      const categoryData = JSON.parse(readFileSync(filePath, 'utf-8'));
      if (categoryData.tools && Array.isArray(categoryData.tools)) {
        // Add category metadata to each tool
        const toolsWithCategory = categoryData.tools.map(tool => ({
          ...tool,
          category: categoryData.category || category,
          categoryName: categoryData.name || category,
        }));
        allTools.push(...toolsWithCategory);
      }
    } catch (error) {
      console.error(`Failed to load tools from ${category}.json:`, error.message);
    }
  }

  return {
    tools: allTools,
  };
}

