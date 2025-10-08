#!/usr/bin/env python3
import os
import sys
import argparse
import json
import hashlib
import re
from pathlib import Path
from typing import Dict, List, Tuple


def parse_design_scale(filename: str) -> float:
    match = re.search(r'@(\d+)x\.(png|jpg|jpeg|bmp)', filename, re.IGNORECASE)
    if match:
        return float(match.group(1))
    return 1.0


def sanitize_identifier(name: str) -> str:
    stem = Path(name).stem
    hash_obj = hashlib.md5(name.encode('utf-8'))
    hash_suffix = hash_obj.hexdigest()[:8]
    sanitized = ''.join(c if c.isalnum() or c == '_' else '_' for c in stem)
    if not sanitized[0].isalpha():
        sanitized = 'res_' + sanitized
    return f"{sanitized}_{hash_suffix}"


def collect_resources(input_dir: Path) -> Dict[str, Tuple[Path, str, float]]:
    resources = {}
    if not input_dir.exists():
        return resources
    
    for file_path in input_dir.rglob('*'):
        if file_path.is_file() and not file_path.name.startswith('.'):
            relative_path = file_path.relative_to(input_dir)
            # 🔧 FIX: Use forward slashes for cross-platform compatibility
            normalized_path = relative_path.as_posix()
            identifier = sanitize_identifier(normalized_path)
            design_scale = parse_design_scale(file_path.name)
            
            counter = 1
            original_identifier = identifier
            while identifier in resources:
                identifier = f"{original_identifier}_{counter}"
                counter += 1
            
            # 🔧 FIX: Store normalized path
            resources[identifier] = (file_path, normalized_path, design_scale)
    
    return resources


def generate_header(resources: Dict[str, Tuple[Path, str, float]], namespace: str, output_path: Path) -> None:
    header_content = f'''#pragma once

#include <cstddef>
#include <string_view>

namespace {namespace} {{

struct ResourceData {{
    const unsigned char* data;
    size_t size;
    std::string_view path;
    float designScale;
}};

'''
    
    for identifier, (_, _, _) in resources.items():
        header_content += f'extern const ResourceData {identifier};\n'
    
    header_content += f'''
const ResourceData* findResource(std::string_view path);
const ResourceData* getAllResources();
size_t getResourceCount();

}} // namespace {namespace}
'''
    
    with open(output_path, 'w', encoding='utf-8') as f:
        f.write(header_content)


def generate_source(resources: Dict[str, Tuple[Path, str, float]], namespace: str,
                   header_name: str, output_path: Path) -> None:
    source_content = f'''#include "{header_name}"
#include <array>
#include <string_view>
#include <algorithm>

namespace {namespace} {{

'''
    
    for identifier, (file_path, relative_path, design_scale) in resources.items():
        with open(file_path, 'rb') as f:
            data = f.read()
        
        hex_data = ', '.join(f'0x{b:02x}' for b in data)
        
        # 🔧 FIX: relative_path already normalized with forward slashes
        source_content += f'''static const unsigned char {identifier}_data[] = {{
    {hex_data}
}};

const ResourceData {identifier} = {{
    {identifier}_data,
    {len(data)},
    "{relative_path}",
    {design_scale}f
}};

'''
    
    source_content += f'''static const std::array<ResourceData, {len(resources)}> all_resources = {{{{
'''
    
    for identifier in resources:
        source_content += f'    {identifier},\n'
    
    source_content += f'''}}}};

const ResourceData* findResource(std::string_view path) {{
    auto it = std::find_if(all_resources.begin(), all_resources.end(),
        [path](const ResourceData& res) {{ return res.path == path; }});
    return (it != all_resources.end()) ? &(*it) : nullptr;
}}

const ResourceData* getAllResources() {{
    return all_resources.data();
}}

size_t getResourceCount() {{
    return all_resources.size();
}}

}} // namespace {namespace}
'''
    
    with open(output_path, 'w', encoding='utf-8') as f:
        f.write(source_content)


def generate_index(resources: Dict[str, Tuple[Path, str, float]], output_path: Path) -> None:
    index_data = {"resources": []}
    
    for identifier, (file_path, relative_path, design_scale) in resources.items():
        stat = file_path.stat()
        index_data["resources"].append({
            "identifier": identifier,
            "path": relative_path,
            "designScale": design_scale,
            "size": stat.st_size,
            "modified": stat.st_mtime
        })
    
    with open(output_path, 'w', encoding='utf-8') as f:
        json.dump(index_data, f, indent=2, ensure_ascii=False)


def main():
    parser = argparse.ArgumentParser(description='Generate embedded resource files')
    parser.add_argument('--input-dir', required=True, help='Input resources directory')
    parser.add_argument('--output-dir', required=True, help='Output directory for generated files')
    parser.add_argument('--namespace', default='Resources', help='C++ namespace for resources')
    parser.add_argument('--header-file', default='embedded_resources.h', help='Header file name')
    parser.add_argument('--source-file', default='embedded_resources.cpp', help='Source file name')
    
    args = parser.parse_args()
    
    input_dir = Path(args.input_dir)
    output_dir = Path(args.output_dir)
    
    output_dir.mkdir(parents=True, exist_ok=True)
    
    resources = collect_resources(input_dir)
    
    if not resources:
        print(f"Warning: No resources found in {input_dir}", file=sys.stderr)
    
    header_path = output_dir / args.header_file
    source_path = output_dir / args.source_file
    index_path = output_dir / "resource_index.json"
    
    generate_header(resources, args.namespace, header_path)
    generate_source(resources, args.namespace, args.header_file, source_path)
    generate_index(resources, index_path)
    
    print(f"Generated {len(resources)} embedded resources")


if __name__ == '__main__':
    main()