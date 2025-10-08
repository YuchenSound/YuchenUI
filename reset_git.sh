
#!/bin/bash
set -e

echo "=== YuchenUI Git Complete Reset ==="

cd /Users/weiyuchen/Music/VSTDevelop/YuchenUI

# 1. 创建备份
echo "Step 1: Creating backup..."
BACKUP_DIR="/Users/weiyuchen/Music/VSTDevelop/YuchenUI_backup_$(date +%Y%m%d_%H%M%S)"
cp -r . "$BACKUP_DIR"
echo "✅ Backup created at: $BACKUP_DIR"
echo ""

# 2. 保存远程仓库信息
echo "Step 2: Saving repository info..."
if [ -f .git/config ]; then
    REMOTE_URL=$(git config --get remote.origin.url 2>/dev/null || echo "")
    CURRENT_BRANCH=$(git branch --show-current 2>/dev/null || echo "main")
else
    REMOTE_URL=""
    CURRENT_BRANCH="main"
fi
echo "Remote URL: $REMOTE_URL"
echo "Current branch: $CURRENT_BRANCH"
echo ""

# 3. 完全删除 Git
echo "Step 3: Removing all Git data..."
rm -rf .git
rm -rf .gitmodules
rm -rf third_party/freetype/.git
rm -rf third_party/harfbuzz/.git
echo "✅ Git data removed"
echo ""

# 4. 删除并重新克隆 submodules
echo "Step 4: Cleaning third_party..."
rm -rf third_party/freetype
rm -rf third_party/harfbuzz
mkdir -p third_party
echo "✅ third_party cleaned"
echo ""

# 5. 克隆 submodules
echo "Step 5: Cloning FreeType..."
git clone --depth 1 https://github.com/freetype/freetype.git third_party/freetype
echo "✅ FreeType cloned"

echo "Step 6: Cloning HarfBuzz..."
git clone --depth 1 https://github.com/harfbuzz/harfbuzz.git third_party/harfbuzz
echo "✅ HarfBuzz cloned"
echo ""

# 6. 删除 submodule 的 .git（变成普通目录）
echo "Step 7: Converting submodules to regular directories..."
rm -rf third_party/freetype/.git
rm -rf third_party/harfbuzz/.git
# 清理所有嵌套的 .git
find third_party -name ".git" -type d -exec rm -rf {} + 2>/dev/null || true
find third_party -name ".gitmodules" -type f -delete 2>/dev/null || true
echo "✅ Submodules converted to regular directories"
echo ""

# 7. 重新初始化 Git
echo "Step 8: Reinitializing Git..."
git init
git branch -M $CURRENT_BRANCH
echo "✅ Git reinitialized"
echo ""

# 8. 添加远程仓库
if [ -n "$REMOTE_URL" ]; then
    echo "Step 9: Adding remote repository..."
    git remote add origin $REMOTE_URL
    echo "✅ Remote added: $REMOTE_URL"
else
    echo "Step 9: No remote repository found, skipping..."
fi
echo ""

# 9. 创建 .gitignore
echo "Step 10: Creating .gitignore..."
cat > .gitignore << 'EOF'
# Build directories
build/
build_*/
cmake-build-*/
out/

# IDE files
.vscode/
.idea/
.vs/
*.suo
*.user
*.swp
.DS_Store
xcuserdata/
DerivedData/

# Compiled files
*.o
*.obj
*.exe
*.dll
*.dylib
*.so
*.a
*.lib

# Generated files
generated/
*.metallib
*.air
*.cso

# Python
__pycache__/
*.pyc

# OS files
Thumbs.db
Desktop.ini
EOF
echo "✅ .gitignore created"
echo ""

# 10. 创建 .gitattributes
echo "Step 11: Creating .gitattributes..."
cat > .gitattributes << 'EOF'
# Auto detect text files
* text=auto

# Source code
*.cpp text eol=lf
*.h text eol=lf
*.hpp text eol=lf
*.c text eol=lf
*.m text eol=lf
*.mm text eol=lf

# CMake
*.cmake text eol=lf
CMakeLists.txt text eol=lf

# Shaders
*.metal text eol=lf
*.hlsl text eol=lf

# Scripts
*.sh text eol=lf
*.py text eol=lf
*.bat text eol=crlf

# Documentation
*.md text eol=lf
*.txt text eol=lf
LICENSE text eol=lf

# Binary files
*.png binary
*.jpg binary
*.ttf binary
*.otf binary
*.ttc binary
*.a binary
*.lib binary
*.dll binary
*.dylib binary
*.so binary
*.exe binary
*.cso binary
*.metallib binary
EOF
echo "✅ .gitattributes created"
echo ""

# 11. 添加所有文件
echo "Step 12: Adding all files..."
git add -A
echo "✅ Files added"
echo ""

# 12. 查看状态
echo "Step 13: Checking status..."
git status --short
echo ""

# 13. 创建初始提交
echo "Step 14: Creating initial commit..."
git commit -m "Initial commit - Clean YuchenUI project

Cross-platform UI framework supporting macOS and Windows

Features:
- Direct3D 11 renderer for Windows
- Metal renderer for macOS
- Text rendering with HarfBuzz and FreeType
- Component-based UI architecture
- Event system with input management
- Theme support (Pro Tools Classic style)
- Window management

Third-party libraries (included as source):
- FreeType 2 for font rendering
- HarfBuzz for text shaping

Structure:
- source/src/     - Core framework implementation
- source/include/ - Public headers
- resources/      - Fonts and assets
- third_party/    - FreeType and HarfBuzz source
- YuchenUIDemo/   - Demo application"

echo "✅ Initial commit created"
echo ""

# 14. 显示结果
echo "=== Reset Complete ==="
echo ""
echo "Repository status:"
git log --oneline -1
echo ""
echo "Remote repository:"
git remote -v
echo ""
echo "Branch:"
git branch -v
echo ""
echo "Backup location: $BACKUP_DIR"
echo ""
echo "Next steps:"
echo "1. Review with: git status"
echo "2. Force push with: git push -f origin $CURRENT_BRANCH"
echo "" SCRIPT_END

