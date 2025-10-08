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
