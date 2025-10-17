# macOS PingFang Font Access Guide via System API

2025·Yuchen Wei

## Table of Contents
- [Introduction](#introduction)
- [Why Use API Instead of Hard-Coded Paths](#why-use-api-instead-of-hard-coded-paths)
- [PingFang Font Overview](#pingfang-font-overview)
- [Access Methods](#access-methods)
  - [Method 1: Get Font by Name](#method-1-get-font-by-name)
  - [Method 2: Get Font by Weight](#method-2-get-font-by-weight)
  - [Method 3: Get Font by Family and Traits](#method-3-get-font-by-family-and-traits)
- [Compilation and Execution](#compilation-and-execution)
- [FAQ](#faq)
- [Best Practices](#best-practices)

---

## Introduction

PingFang (苹方) is the Chinese system font introduced by Apple starting from macOS El Capitan (10.11) and iOS 9. Designed jointly by DynaComware and Apple, it replaced the previous STHeiti (Heiti) font, offering better readability and more weight options.

### Font Storage Location Changes

The storage location of PingFang font varies across different macOS versions:

- **macOS Sonoma and earlier**: `/System/Library/Fonts/PingFang.ttc`
- **macOS Sequoia (15.0+)**: `/System/Library/AssetsV2/com_apple_MobileAsset_Font7/.../PingFang.ttc`

The font file uses **TTC (TrueType Collection)** format, containing multiple font families and weights in a single file.

---

## Why Use API Instead of Hard-Coded Paths

###  Wrong Approach: Hard-Coded Path

```objective-c
// Don't do this!
NSString *fontPath = @"/System/Library/Fonts/PingFang.ttc";
```

**Problems:**
1. Path may change across macOS versions
2. Fonts may be moved to dynamic asset system (AssetsV2)
3. Users may customize font locations
4. Code lacks portability

### Correct Approach: Use System API

```objective-c
// Recommended way
NSFont *font = [NSFont fontWithName:@"PingFangSC-Regular" size:13];
CTFontRef ctFont = (__bridge CTFontRef)font;
CFURLRef url = CTFontCopyAttribute(ctFont, kCTFontURLAttribute);
NSString *path = [(__bridge NSURL *)url path];
CFRelease(url);
```

**Advantages:**
- Automatically handles font location changes
- Works across all macOS versions
- Supports font fallback mechanism
- Respects user customizations

---

## PingFang Font Overview

### Font Variants

PingFang includes three regional variants:
- **PingFang SC**: Simplified Chinese
- **PingFang TC**: Traditional Chinese (Taiwan)
- **PingFang HK**: Traditional Chinese (Hong Kong)

### Weight List

| English Name | Chinese Name | PostScript Name | Weight Value |
|-------------|--------------|-----------------|--------------|
| Ultralight | 极细体 | PingFangSC-Ultralight | -0.8 |
| Thin | 纤细体 | PingFangSC-Thin | -0.6 |
| Light | 细体 | PingFangSC-Light | -0.4 |
| Regular | 常规体 | PingFangSC-Regular | 0.0 |
| Medium | 中黑体 | PingFangSC-Medium | 0.23 |
| **Semibold** | **中粗体 (Bold)** | **PingFangSC-Semibold** | **0.3** |

> **Note**: The boldest weight in PingFang is Semibold. There is no Bold or Heavy version.

---

## Access Methods

### Method 1: Get Font by Name

This is the most straightforward method when you know the exact PostScript name.

```objective-c
#import <Cocoa/Cocoa.h>
#import <CoreText/CoreText.h>

int main(int argc, const char * argv[]) {
    @autoreleasepool {
        // Get PingFang Semibold (Bold)
        NSFont *font = [NSFont fontWithName:@"PingFangSC-Semibold" size:13];
        
        if (font) {
            NSLog(@"Font Name: %@", font.fontName);
            NSLog(@"Font Family: %@", font.familyName);
            
            // Get font file path
            CTFontRef ctFont = (__bridge CTFontRef)font;
            CFURLRef url = CTFontCopyAttribute(ctFont, kCTFontURLAttribute);
            
            if (url) {
                NSLog(@"Font Path: %@", [(__bridge NSURL *)url path]);
                CFRelease(url);  // Important: Release Core Foundation object
            }
        } else {
            NSLog(@"Font not found!");
        }
    }
    return 0;
}
```

#### Getting All Weights

```objective-c
NSArray *fontNames = @[
    @"PingFangSC-Ultralight",
    @"PingFangSC-Thin",
    @"PingFangSC-Light",
    @"PingFangSC-Regular",
    @"PingFangSC-Medium",
    @"PingFangSC-Semibold"
];

for (NSString *fontName in fontNames) {
    NSFont *font = [NSFont fontWithName:fontName size:13];
    if (font) {
        NSLog(@"Font: %@", fontName);
        
        CTFontRef ctFont = (__bridge CTFontRef)font;
        CFURLRef url = CTFontCopyAttribute(ctFont, kCTFontURLAttribute);
        if (url) {
            NSLog(@"  Path: %@", [(__bridge NSURL *)url path]);
            CFRelease(url);
        }
    }
}
```

---

### Method 2: Get Font by Weight

Use Font Weight API for dynamic weight selection.

#### Using NSFontDescriptor

```objective-c
#import <Cocoa/Cocoa.h>
#import <CoreText/CoreText.h>

// Create font descriptor with weight trait
NSDictionary *traits = @{
    NSFontWeightTrait: @(NSFontWeightSemibold)
};

NSDictionary *attributes = @{
    NSFontFamilyAttribute: @"PingFang SC",
    NSFontTraitsAttribute: traits
};

NSFontDescriptor *descriptor = [NSFontDescriptor 
    fontDescriptorWithFontAttributes:attributes];
NSFont *font = [NSFont fontWithDescriptor:descriptor size:13];

if (font) {
    NSLog(@"Font Name: %@", font.fontName);
    
    CTFontRef ctFont = (__bridge CTFontRef)font;
    CFURLRef url = CTFontCopyAttribute(ctFont, kCTFontURLAttribute);
    if (url) {
        NSLog(@"Font Path: %@", [(__bridge NSURL *)url path]);
        CFRelease(url);
    }
}
```

#### Using Core Text API

```objective-c
#import <CoreText/CoreText.h>

// Create font descriptor using Core Text
NSDictionary *ctTraits = @{
    (__bridge NSString *)kCTFontWeightTrait: @(0.3)  // Semibold = 0.3
};

NSDictionary *ctAttributes = @{
    (__bridge NSString *)kCTFontFamilyNameAttribute: @"PingFang SC",
    (__bridge NSString *)kCTFontTraitsAttribute: ctTraits
};

CTFontDescriptorRef descriptor = CTFontDescriptorCreateWithAttributes(
    (__bridge CFDictionaryRef)ctAttributes
);

CTFontRef ctFont = CTFontCreateWithFontDescriptor(descriptor, 13.0, NULL);

if (ctFont) {
    // Get PostScript name
    CFStringRef psName = CTFontCopyPostScriptName(ctFont);
    NSLog(@"PostScript Name: %@", (__bridge NSString *)psName);
    CFRelease(psName);
    
    // Get font file path
    CFURLRef url = CTFontCopyAttribute(ctFont, kCTFontURLAttribute);
    if (url) {
        NSLog(@"Font Path: %@", [(__bridge NSURL *)url path]);
        CFRelease(url);
    }
    
    CFRelease(ctFont);
}

CFRelease(descriptor);
```

#### Weight Values Reference

```objective-c
// NSFont weight constants and their numeric values
typedef struct {
    NSString *name;
    CGFloat value;
} WeightMapping;

WeightMapping weights[] = {
    {@"UltraLight", -0.8},
    {@"Thin", -0.6},
    {@"Light", -0.4},
    {@"Regular", 0.0},
    {@"Medium", 0.23},
    {@"Semibold", 0.3},
    {@"Bold", 0.4},
    {@"Heavy", 0.56}
};

// Usage example
for (int i = 0; i < 8; i++) {
    NSDictionary *traits = @{
        (__bridge NSString *)kCTFontWeightTrait: @(weights[i].value)
    };
    
    NSDictionary *attributes = @{
        (__bridge NSString *)kCTFontFamilyNameAttribute: @"PingFang SC",
        (__bridge NSString *)kCTFontTraitsAttribute: traits
    };
    
    CTFontDescriptorRef descriptor = CTFontDescriptorCreateWithAttributes(
        (__bridge CFDictionaryRef)attributes
    );
    CTFontRef font = CTFontCreateWithFontDescriptor(descriptor, 13.0, NULL);
    
    if (font) {
        CFStringRef name = CTFontCopyPostScriptName(font);
        NSLog(@"%@: %@", weights[i].name, (__bridge NSString *)name);
        CFRelease(name);
        CFRelease(font);
    }
    
    CFRelease(descriptor);
}
```

---

### Method 3: Get Font by Family and Traits

Enumerate all members of a font family to find specific fonts.

```objective-c
#import <Cocoa/Cocoa.h>
#import <CoreText/CoreText.h>

int main(int argc, const char * argv[]) {
    @autoreleasepool {
        // Get all members of PingFang SC family
        NSArray *members = [[NSFontManager sharedFontManager] 
            availableMembersOfFontFamily:@"PingFang SC"];
        
        NSLog(@"PingFang SC Family Members:");
        NSLog(@"============================\n");
        
        for (NSArray *member in members) {
            NSString *fontName = member[0];      // PostScript name
            NSString *displayName = member[1];   // Display name
            NSNumber *weight = member[2];        // Weight
            NSNumber *traits = member[3];        // Traits
            
            NSLog(@"PostScript Name: %@", fontName);
            NSLog(@"  Display Name: %@", displayName);
            NSLog(@"  Weight: %@", weight);
            NSLog(@"  Traits: %@", traits);
            
            // Get font file path
            NSFont *font = [NSFont fontWithName:fontName size:13];
            if (font) {
                CTFontRef ctFont = (__bridge CTFontRef)font;
                CFURLRef url = CTFontCopyAttribute(ctFont, kCTFontURLAttribute);
                if (url) {
                    NSLog(@"  Path: %@", [(__bridge NSURL *)url path]);
                    CFRelease(url);
                }
            }
            NSLog(@"");
        }
    }
    return 0;
}
```

---

## Compilation and Execution

### Method 1: Compile Standalone Program

```bash
# Create source file
cat > get_pingfang.m << 'EOF'
#import <Cocoa/Cocoa.h>
#import <CoreText/CoreText.h>

int main(int argc, const char * argv[]) {
    @autoreleasepool {
        NSFont *font = [NSFont fontWithName:@"PingFangSC-Semibold" size:13];
        if (font) {
            NSLog(@"Font Name: %@", font.fontName);
            NSLog(@"Family: %@", font.familyName);
            
            CTFontRef ctFont = (__bridge CTFontRef)font;
            CFURLRef url = CTFontCopyAttribute(ctFont, kCTFontURLAttribute);
            if (url) {
                NSLog(@"Path: %@", [(__bridge NSURL *)url path]);
                CFRelease(url);
            }
        }
    }
    return 0;
}
EOF

# Compile with frameworks
clang -framework Cocoa -framework CoreText get_pingfang.m -o get_pingfang

# Run
./get_pingfang
```

### Method 2: One-Line Command

```bash
# Quick test: Get PingFang Semibold path
clang -framework Cocoa -framework CoreText -x objective-c - -o test << 'EOF' && ./test && rm test
#import <Cocoa/Cocoa.h>
#import <CoreText/CoreText.h>
int main() {
    @autoreleasepool {
        NSFont *f = [NSFont fontWithName:@"PingFangSC-Semibold" size:13];
        if (f) {
            printf("Font: %s\n", [f.fontName UTF8String]);
            CFURLRef u = CTFontCopyAttribute((__bridge CTFontRef)f, kCTFontURLAttribute);
            if (u) {
                printf("Path: %s\n", [[(__bridge NSURL *)u path] UTF8String]);
                CFRelease(u);
            }
        }
    }
    return 0;
}
EOF
```

### Method 3: Using Xcode

1. Create a new **macOS Command Line Tool** project
2. Select **Objective-C** as the language
3. Replace the contents of `main.m` with your code
4. Build and run (`Cmd+R`)

### Compilation Options

```bash
# Basic compilation
clang -framework Cocoa -framework CoreText source.m -o output

# With optimization
clang -O2 -framework Cocoa -framework CoreText source.m -o output

# With debugging symbols
clang -g -framework Cocoa -framework CoreText source.m -o output

# For specific macOS version
clang -mmacosx-version-min=10.15 -framework Cocoa -framework CoreText source.m -o output
```

---

## FAQ

### Q1: Why can't I see PingFang in Font Book?

**A**: PingFang's PostScript name starts with a dot (`.PingFang-SC`), indicating it's a system fallback font. Such fonts are hidden in Font Book but can be used normally via API.

### Q2: How to get Traditional Chinese variants?

**A**: Change the font name to:
- Traditional Chinese (Taiwan): `PingFangTC-Regular`, `PingFangTC-Semibold`, etc.
- Traditional Chinese (Hong Kong): `PingFangHK-Regular`, `PingFangHK-Semibold`, etc.

Example:
```objective-c
// Taiwan Traditional Chinese
NSFont *tcFont = [NSFont fontWithName:@"PingFangTC-Semibold" size:13];

// Hong Kong Traditional Chinese
NSFont *hkFont = [NSFont fontWithName:@"PingFangHK-Semibold" size:13];
```

### Q3: What character sets does PingFang support?

**A**: PingFang includes:
- Complete GB 18030 character set (Simplified)
- Big5 character set (Traditional)
- Basic Latin alphabet and numbers
- Common punctuation marks

### Q4: How to use PingFang in my application?

**A**: Don't specify PingFang directly. Use system font API instead:

```objective-c
// Recommended: Use system font
NSFont *systemFont = [NSFont systemFontOfSize:13];

// System will automatically use:
// - San Francisco for English/numbers
// - PingFang for Chinese characters

// Only if you specifically need PingFang
NSFont *pingfang = [NSFont fontWithName:@"PingFangSC-Regular" size:13];
```

### Q5: What does the AssetsV2 path mean?

**A**: Starting from recent macOS versions, Apple uses a dynamic asset system (MobileAsset) to manage fonts:
- Supports on-demand downloads
- Independent font updates
- Saves disk space
- Unified resource management for iOS and macOS

Example path:
```
/System/Library/AssetsV2/com_apple_MobileAsset_Font7/
  3419f2a427639ad8c8e139149a287865a90fa17e.asset/AssetData/PingFang.ttc
```

This is why you **must use API** to get the path!

### Q6: How to get the system font (San Francisco)?

**A**: Use the system font API:

```objective-c
// Get system font
NSFont *systemFont = [NSFont systemFontOfSize:13];
NSLog(@"System Font: %@", systemFont.fontName);  // .AppleSystemUIFont

// Get file path
CTFontRef ctFont = (__bridge CTFontRef)systemFont;
CFURLRef url = CTFontCopyAttribute(ctFont, kCTFontURLAttribute);
if (url) {
    NSLog(@"Path: %@", [(__bridge NSURL *)url path]);
    // Output: /System/Library/Fonts/SFNS.ttf
    CFRelease(url);
}
```

### Q7: Memory management with Core Foundation?

**A**: Always remember to release Core Foundation objects:

```objective-c
// Correct pattern
CFURLRef url = CTFontCopyAttribute(ctFont, kCTFontURLAttribute);
if (url) {
    // Use the URL
    NSString *path = [(__bridge NSURL *)url path];
    
    // IMPORTANT: Release when done
    CFRelease(url);
}

// For CTFont objects created with Create functions
CTFontRef font = CTFontCreateWithFontDescriptor(descriptor, 13.0, NULL);
if (font) {
    // Use the font
    
    // IMPORTANT: Release when done
    CFRelease(font);
}
```

**Rule of thumb**: If a Core Text function has "Create" or "Copy" in its name, you must `CFRelease` the returned object.

---

## Best Practices

### 1. Always Use API for Font Access

```objective-c
// ✓ Good
NSFont *font = [NSFont fontWithName:@"PingFangSC-Regular" size:13];
CTFontRef ctFont = (__bridge CTFontRef)font;
CFURLRef url = CTFontCopyAttribute(ctFont, kCTFontURLAttribute);

// ✗ Bad
NSString *hardcodedPath = @"/System/Library/Fonts/PingFang.ttc";
```

### 2. Prefer System Font API

```objective-c
// ✓ Best: Let system choose appropriate font
NSFont *systemFont = [NSFont systemFontOfSize:13];

// ✓ Good: Specify PingFang when needed
NSFont *pingfang = [NSFont fontWithName:@"PingFangSC-Regular" size:13];

// ✗ Bad: Hard-code font in UI
NSFont *hardcoded = [NSFont fontWithName:@"PingFangSC-Regular" size:13];
```

### 3. Always Check for nil

```objective-c
NSFont *font = [NSFont fontWithName:@"PingFangSC-Semibold" size:13];
if (font) {
    // Font exists, use it
} else {
    // Font not found, use fallback
    font = [NSFont systemFontOfSize:13];
}
```

### 4. Release Core Foundation Objects

```objective-c
CFURLRef url = CTFontCopyAttribute(ctFont, kCTFontURLAttribute);
if (url) {
    // Use the URL
    NSString *path = [(__bridge NSURL *)url path];
    
    // Always release
    CFRelease(url);
}
```

### 5. Handle Different macOS Versions

```objective-c
// Check if font is available
BOOL isPingFangAvailable(void) {
    NSFont *font = [NSFont fontWithName:@"PingFangSC-Regular" size:13];
    return (font != nil);
}

// Use with fallback
NSFont *getChineseFont(CGFloat size) {
    NSFont *font = [NSFont fontWithName:@"PingFangSC-Regular" size:size];
    if (!font) {
        // Fallback to system font on older macOS
        font = [NSFont systemFontOfSize:size];
    }
    return font;
}
```

### 6. Test Across macOS Versions

Test your code on:
- macOS 10.15 Catalina
- macOS 11 Big Sur
- macOS 12 Monterey
- macOS 13 Ventura
- macOS 14 Sonoma
- macOS 15 Sequoia

### 7. Use Proper Error Handling

```objective-c
NSFont* getPingFangBold(void) {
    NSFont *font = [NSFont fontWithName:@"PingFangSC-Semibold" size:13];
    
    if (!font) {
        NSLog(@"Warning: PingFang Semibold not found, using system font");
        font = [NSFont systemFontOfSize:13 weight:NSFontWeightSemibold];
    }
    
    return font;
}
```

---

## Complete Example Program

Here's a complete, production-ready example:

```objective-c
#import <Cocoa/Cocoa.h>
#import <CoreText/CoreText.h>

@interface FontHelper : NSObject
+ (NSFont *)pingFangFontWithWeight:(NSString *)weight size:(CGFloat)size;
+ (NSString *)fontPathForFont:(NSFont *)font;
+ (void)printFontInfo:(NSFont *)font;
@end

@implementation FontHelper

+ (NSFont *)pingFangFontWithWeight:(NSString *)weight size:(CGFloat)size {
    NSString *fontName = [NSString stringWithFormat:@"PingFangSC-%@", weight];
    NSFont *font = [NSFont fontWithName:fontName size:size];
    
    if (!font) {
        NSLog(@"Warning: Font %@ not found", fontName);
        font = [NSFont systemFontOfSize:size];
    }
    
    return font;
}

+ (NSString *)fontPathForFont:(NSFont *)font {
    if (!font) return nil;
    
    CTFontRef ctFont = (__bridge CTFontRef)font;
    CFURLRef url = CTFontCopyAttribute(ctFont, kCTFontURLAttribute);
    
    NSString *path = nil;
    if (url) {
        path = [(__bridge NSURL *)url path];
        CFRelease(url);
    }
    
    return path;
}

+ (void)printFontInfo:(NSFont *)font {
    if (!font) {
        NSLog(@"Font is nil");
        return;
    }
    
    NSLog(@"Font Name: %@", font.fontName);
    NSLog(@"Display Name: %@", font.displayName);
    NSLog(@"Family: %@", font.familyName);
    NSLog(@"Point Size: %.1f", font.pointSize);
    
    NSString *path = [self fontPathForFont:font];
    if (path) {
        NSLog(@"File Path: %@", path);
    }
}

@end

int main(int argc, const char * argv[]) {
    @autoreleasepool {
        // Get PingFang Bold
        NSFont *boldFont = [FontHelper pingFangFontWithWeight:@"Semibold" size:16];
        [FontHelper printFontInfo:boldFont];
        
        NSLog(@"\n---\n");
        
        // Get all weights
        NSArray *weights = @[@"Ultralight", @"Thin", @"Light", 
                            @"Regular", @"Medium", @"Semibold"];
        for (NSString *weight in weights) {
            NSFont *font = [FontHelper pingFangFontWithWeight:weight size:13];
            NSLog(@"%@: %@", weight, font.fontName);
        }
    }
    return 0;
}
```

Save and compile:
```bash
clang -framework Cocoa -framework CoreText complete_example.m -o font_helper
./font_helper
```

---

## Reference Links

- [Apple Typography Guidelines](https://developer.apple.com/design/human-interface-guidelines/typography)
- [Core Text Programming Guide](https://developer.apple.com/library/archive/documentation/StringsTextFonts/Conceptual/CoreText_Programming/Introduction/Introduction.html)
- [NSFont Class Reference](https://developer.apple.com/documentation/appkit/nsfont)
- [CTFont Reference](https://developer.apple.com/documentation/coretext/ctfont)
- [Font Handling in macOS](https://developer.apple.com/fonts/)

---

**Last Updated**: October 2025

**License**: MIT License

**Feedback**: For issues or suggestions, please refer to Apple Developer Forums or Stack Overflow.
