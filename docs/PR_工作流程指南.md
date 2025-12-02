# PR 工作流程指南

本文档说明如何在 ShineEngine 项目中创建 Pull Request (PR) 和提交代码的完整流程。

## 目录
- [准备工作](#准备工作)
- [创建功能分支](#创建功能分支)
- [开发与提交](#开发与提交)
- [创建 Pull Request](#创建-pull-request)
- [处理代码审查](#处理代码审查)
- [合并 PR](#合并-pr)

---

## 准备工作

### 1. 确保本地仓库是最新的

```bash
# 切换到主分支
git checkout main

# 拉取最新的代码
git pull origin main
```

### 2. 配置 Git 用户信息（如果还未配置）

```bash
git config --global user.name "你的名字"
git config --global user.email "你的邮箱"
```

---

## 创建功能分支

### 1. 从主分支创建新分支

```bash
# 确保在主分支
git checkout main

# 拉取最新代码
git pull origin main

# 创建并切换到新分支
git checkout -b feature/你的功能名称

# 例如：
# git checkout -b feature/add-obj-loader
```

### 2. 分支命名规范

- **功能分支**: `feature/功能描述`
  - 例如: `feature/add-obj-loader`
- **修复分支**: `fix/问题描述`
  - 例如: `fix/memory-leak`
- **重构分支**: `refactor/重构内容`
  - 例如: `refactor/loader-interface`

---

## 开发与提交

### 1. 进行代码修改

在编辑器中修改代码，实现你的功能。

### 2. 查看修改状态

```bash
# 查看哪些文件被修改了
git status

# 查看具体的修改内容
git diff
```

### 3. 暂存文件

```bash
# 暂存单个文件
git add src/loader/objLoader.cpp

# 暂存多个文件
git add src/loader/objLoader.cpp src/loader/objLoader.h

# 暂存所有修改的文件（谨慎使用）
git add .
```

### 4. 提交更改

```bash
# 提交并添加提交信息
git commit -m "feat: 添加 OBJ 模型加载器

- 实现 OBJ 文件格式解析
- 支持 MTL 材质文件解析
- 提供 MeshData 数据结构"

# 或者使用多行提交信息
git commit -m "feat: 添加 OBJ 模型加载器" -m "
- 实现 OBJ 文件格式解析
- 支持 MTL 材质文件解析
- 提供 MeshData 数据结构
"
```

### 5. 提交信息规范

使用 [Conventional Commits](https://www.conventionalcommits.org/) 规范：

- **feat**: 新功能
  - `feat: 添加 OBJ 模型加载器`
- **fix**: 修复 bug
  - `fix: 修复浮点数解析问题`
- **docs**: 文档更新
  - `docs: 更新 README`
- **style**: 代码格式调整（不影响功能）
  - `style: 格式化代码`
- **refactor**: 重构代码
  - `refactor: 重构加载器接口`
- **test**: 添加测试
  - `test: 添加 OBJ 加载器单元测试`
- **chore**: 构建过程或辅助工具的变动
  - `chore: 更新依赖包`

### 6. 推送分支到远程

```bash
# 首次推送新分支
git push -u origin feature/你的功能名称

# 后续推送（如果分支已存在）
git push origin feature/你的功能名称
```

---

## 创建 Pull Request

### 1. 在 GitHub 上创建 PR

1. 访问项目仓库: `https://github.com/tiantianaixuexi/ShineEngine`
2. 点击 **"Pull requests"** 标签页
3. 点击 **"New pull request"** 按钮
4. 选择分支：
   - **base**: `main` (目标分支)
   - **compare**: `feature/你的功能名称` (你的分支)
5. 填写 PR 信息：
   - **标题**: 清晰描述 PR 的内容
   - **描述**: 详细说明：
     - 这个 PR 做了什么
     - 为什么需要这个改动
     - 如何测试
     - 相关的 Issue（如果有）

### 2. PR 描述模板示例

```markdown
## 描述
添加 OBJ 模型加载器，支持加载和解析 OBJ/MTL 文件格式。

## 变更内容
- 实现 `objLoader` 类，继承自 `IAssetLoader`
- 支持标准 OBJ 文件格式解析（顶点、纹理坐标、法线、面）
- 支持 MTL 材质文件解析
- 支持组和对象，按材质分组提取网格数据
- 提供 `MeshData` 结构，兼容 `gltfLoader`

## 测试
- [ ] 测试加载标准 OBJ 文件
- [ ] 测试加载带 MTL 的 OBJ 文件
- [ ] 测试从内存加载 OBJ 数据
- [ ] 验证提取的网格数据正确性

## 相关 Issue
Closes #123
```

### 3. 使用 Cursor AI 审查（可选）

在 PR 描述中添加 `@cursor review` 可以让 Cursor Bot 自动审查代码：

```markdown
@cursor review
```

---

## 处理代码审查

### 1. 查看审查意见

在 GitHub PR 页面查看审查者的评论和建议。

### 2. 修改代码

根据审查意见修改代码：

```bash
# 修改文件后，暂存并提交
git add src/loader/objLoader.cpp
git commit -m "fix: 修复浮点数解析问题

- 修复符号位置验证
- 修复 illum 命令检查
- 移除重复代码"
```

### 3. 推送更新

```bash
# 推送到远程分支，PR 会自动更新
git push origin feature/你的功能名称
```

### 4. 回复审查意见

在 GitHub PR 页面上：
- 回复审查者的评论
- 标记已解决的评论
- 请求再次审查（如果需要）

---

## 合并 PR

### 1. 确保 PR 已通过审查

- ✅ 所有审查意见已处理
- ✅ CI/CD 检查通过（如果有）
- ✅ 代码审查已批准

### 2. 合并 PR

在 GitHub PR 页面：
1. 点击 **"Merge pull request"** 按钮
2. 选择合并方式：
   - **Create a merge commit**: 保留完整历史记录
   - **Squash and merge**: 将多个提交合并为一个
   - **Rebase and merge**: 线性历史记录
3. 确认合并

### 3. 清理本地分支（可选）

```bash
# 切换回主分支
git checkout main

# 拉取最新的主分支代码
git pull origin main

# 删除本地功能分支
git branch -d feature/你的功能名称

# 删除远程分支（如果已合并）
git push origin --delete feature/你的功能名称
```

---

## 常见问题

### Q: 如何更新已创建的 PR？

A: 只需在本地分支继续提交并推送：

```bash
# 修改代码
# ...

# 提交更改
git add .
git commit -m "fix: 修复某个问题"
git push origin feature/你的功能名称
```

PR 会自动更新，无需重新创建。

### Q: 如何将主分支的最新更改合并到我的分支？

A: 使用 rebase 或 merge：

```bash
# 方法1: 使用 rebase（推荐，保持历史清晰）
git checkout feature/你的功能名称
git fetch origin
git rebase origin/main

# 如果有冲突，解决后：
git add .
git rebase --continue

# 强制推送（因为历史已改变）
git push origin feature/你的功能名称 --force-with-lease

# 方法2: 使用 merge
git checkout feature/你的功能名称
git fetch origin
git merge origin/main

# 解决冲突后
git add .
git commit -m "merge: 合并主分支最新更改"
git push origin feature/你的功能名称
```

### Q: 提交信息写错了怎么办？

A: 使用 `git commit --amend`：

```bash
# 修改最后一次提交信息
git commit --amend -m "新的提交信息"

# 如果已推送，需要强制推送
git push origin feature/你的功能名称 --force-with-lease
```

### Q: 如何撤销最后一次提交但保留修改？

A: 使用 `git reset`：

```bash
# 撤销提交但保留修改
git reset --soft HEAD~1

# 修改代码后重新提交
git add .
git commit -m "新的提交信息"
```

---

## 最佳实践

1. **保持分支小且专注**: 一个 PR 只解决一个问题
2. **频繁提交**: 小的、逻辑清晰的提交更容易审查
3. **编写清晰的提交信息**: 说明"做了什么"和"为什么"
4. **及时响应审查意见**: 保持沟通顺畅
5. **测试你的代码**: 确保功能正常工作
6. **保持主分支干净**: 不要直接在主分支上开发

---

## 参考资源

- [Git 官方文档](https://git-scm.com/doc)
- [GitHub Flow](https://guides.github.com/introduction/flow/)
- [Conventional Commits](https://www.conventionalcommits.org/)
- [Git 工作流程指南](https://www.atlassian.com/git/tutorials/comparing-workflows)

---

**最后更新**: 2025年

