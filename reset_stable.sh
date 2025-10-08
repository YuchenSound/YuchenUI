
#!/bin/bash
set -e

echo "=== YuchenUI Git Reset with Stable Third-Party Libraries ==="
echo ""

cd /Users/weiyuchen/Music/VSTDevelop/YuchenUI

# 1. Backup
echo "Step 1: Creating backup..."
BACKUP_DIR="/Users/weiyuchen/Music/VSTDevelop/YuchenUI_backup_$(date +%Y%m%d_%H%M%S)"
cp -r . "$BACKUP_DIR"
echo "Backup: $BACKUP_DIR"
echo ""

# 2. Save remote info
echo "Step 2: Saving repository info..."
REMOTE_URL=$(git config --get remote.origin.url 2>/dev/null || echo "")
CURRENT_BRANCH=$(git branch --show-current 2>/dev/null || echo "main")
echo "Remote: $REMOTE_URL"
echo "Branch: $CURRENT_BRANCH"
echo ""

# 3. Delete all Git data
echo "Step 3: Removing Git data..."
rm -rf .git .gitmodules
find . -name ".git" -type d -exec rm -rf {} + 2>/dev/null || true
find . -name ".gitmodules" -type f -delete 2>/dev/null || true
echo "Done"
echo ""

# 4. Clean third_party
echo "Step 4: Cleaning third_party..."
rm -rf third_party/freetype
rm -rf third_party/harfbuzz
rm -rf third_party/stb
mkdir -p third_party
echo "Done"
echo ""

# 5. Clone FreeType 2.13.2
echo "Step 5: Cloning FreeType 2.13.2..."
git clone --depth 1 --branch VER-2-13-2 \
    https://github.com/freetype/freetype.git \
    third_party/freetype
rm -rf third_party/freetype/.git
echo "FreeType 2.13.2 ready"
echo ""

# 6. Clone HarfBuzz 8.3.0
echo "Step 6: Cloning HarfBuzz 8.3.0..."
git clone --depth 1 --branch 8.3.0 \
    https://github.com/harfbuzz/harfbuzz.git \
    third_party/harfbuzz
rm -rf third_party/harfbuzz/.git
echo "HarfBuzz 8.3.0 ready"
echo ""

# 7. Add STB (single header library)
echo "Step 7: Adding STB image..."
mkdir -p third_party/stb
curl -L -o third_party/stb/stb_image.h \
    https://raw.githubusercontent.com/nothings/stb/master/stb_image.h
echo "STB added"
echo ""

# 8. Clean all nested .git
echo "Step 8: Cleaning nested .git directories..."
find third_party -name ".git" -type d -exec rm -rf {} + 2>/dev/null || true
find third_party -name ".gitmodules" -delete 2>/dev/null || true
echo "Done"
echo ""

# 9. Reinitialize Git
echo "Step 9: Reinitializing Git..."
git init
git branch -M $CURRENT_BRANCH
if [ -n "$REMOTE_URL" ]; then
    git remote add origin $REMOTE_URL
fi
echo "Git initialized"
echo ""

# 10. Create .gitignore
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
*.xcworkspace/xcuserdata/
*.xcodeproj/xcuserdata/
DerivedData/
*.suo
*.user
*.swp
*.swo
.DS_Store

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
*.spv

# Python
__pycache__/
*.pyc

# OS files
Thumbs.db
Desktop.ini
EOF
echo "Done"
echo ""

# 11. Create .gitattributes
echo "Step 11: Creating .gitattributes..."
cat > .gitattributes << 'EOF'
# Auto detect text files
* text=auto

# Source code - always LF
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

# Docs
*.md text eol=lf
*.txt text eol=lf
LICENSE text eol=lf

# Binary files
*.png binary
*.jpg binary
*.jpeg binary
*.gif binary
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
*.air binary
EOF
echo "Done"
echo ""

# 12. Create third_party README
echo "Step 12: Creating third_party/README.md..."
cat > third_party/README.md << 'EOF'
# Third-Party Libraries

## FreeType 2.13.2
- Repository: https://github.com/freetype/freetype
- License: FreeType License (BSD-style)
- Tag: VER-2-13-2
- Purpose: Font rendering

## HarfBuzz 8.3.0
- Repository: https://github.com/harfbuzz/harfbuzz
- License: MIT License
- Tag: 8.3.0
- Purpose: Text shaping

## STB Image
- Repository: https://github.com/nothings/stb
- License: MIT License or Public Domain
- Purpose: Image loading

## Update Instructions
```bash
# FreeType
rm -rf freetype
git clone --depth 1 --branch VER-2-13-2 https://github.com/freetype/freetype.git
rm -rf freetype/.git

# HarfBuzz
rm -rf harfbuzz
git clone --depth 1 --branch 8.3.0 https://github.com/harfbuzz/harfbuzz.git
rm -rf harfbuzz/.git

# STB
mkdir -p stb
curl -L -o stb/stb_image.h https://raw.githubusercontent.com/nothings/stb/master/stb_image.h
EOF
echo "Done"
echo ""
13. Add all files
echo "Step 13: Adding files to Git..."
git add -A
echo "Done"
echo ""
14. Show status
echo "Step 14: Checking what will be committed..."
git status --short | head -30
echo "..."
echo ""
15. Create commit
echo "Step 15: Creating initial commit..."
git commit -m "Initial commit - YuchenUI with stable dependencies
Cross-platform UI framework for macOS and Windows
Features:

Direct3D 11 renderer (Windows)
Metal renderer (macOS)
Text rendering with HarfBuzz + FreeType
Component-based architecture
Event system
Theme support
Window management

Third-Party Libraries:

FreeType 2.13.2 (stable)
HarfBuzz 8.3.0 (stable)
STB Image (latest)

Project Structure:

source/         Framework implementation
resources/      Fonts and assets
third_party/    FreeType, HarfBuzz, STB
YuchenUIDemo/   Demo application"
echo "Done"
echo ""

16. Show result
echo "=== Reset Complete ==="
echo ""
echo "Commit:"
git log --oneline -1
echo ""
echo "Remote:"
git remote -v
echo ""
echo "Branch:"
git branch -v
echo ""
echo "Backup: $BACKUP_DIR"
echo ""
echo "Files committed: $(git ls-files | wc -l)"
echo ""
17. Ask for push
echo "Ready to force push!"
echo ""
read -p "Force push to GitHub? This will OVERWRITE remote history! (yes/no): " confirm
if [ "$confirm" = "yes" ]; then
echo ""
echo "Force pushing..."
git push -f origin $CURRENT_BRANCH
echo ""
echo "Push complete!"
else
echo ""
echo "Push cancelled. You can manually push with:"
echo "  git push -f origin $CURRENT_BRANCH"
fi
echo ""
echo "=== Done ==="
