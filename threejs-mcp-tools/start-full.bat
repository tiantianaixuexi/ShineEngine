@echo off
echo Starting Three.js MCP Full Server...
cd mcp-server
npm install @types/three --save-dev
npm run build
npm start
pause
