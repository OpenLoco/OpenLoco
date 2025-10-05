#!/usr/bin/env python3
"""
Search for all loco_global<{type},{addr}> {name} instances in the codebase.
Sorts them by offset and shows locations where they appear.
"""

import re
import os
import sys
import argparse
from pathlib import Path
from collections import defaultdict
import xml.etree.ElementTree as ET
from xml.dom import minidom

def parse_static_asserts_from_files(root_dir='src'):
    """Parse static_assert statements from all C++ files to extract type sizes."""
    type_sizes = {}

    # Pattern: static_assert(sizeof(Type) == SIZE); where SIZE can be decimal or hex
    pattern = re.compile(r'static_assert\(sizeof\((.*?)\)\s*==\s*(0x[0-9A-Fa-f]+|\d+)\)')

    for root, dirs, files in os.walk(root_dir):
        # Skip build directories
        dirs[:] = [d for d in dirs if d not in {'.git', 'build', 'cmake-build-debug', 'cmake-build-release', 'node_modules', '.venv'}]

        for file in files:
            if file.endswith(('.cpp', '.h', '.hpp')):
                filepath = os.path.join(root, file)
                try:
                    with open(filepath, 'r', encoding='utf-8', errors='ignore') as f:
                        for line in f:
                            match = pattern.search(line)
                            if match:
                                type_name = match.group(1).strip()
                                size_str = match.group(2).strip()
                                # Parse hex or decimal
                                if size_str.startswith('0x'):
                                    size = int(size_str, 16)
                                else:
                                    size = int(size_str)
                                type_sizes[type_name] = size
                except Exception as e:
                    pass  # Silently skip files we can't read

    return type_sizes

# Type size map (in bytes, for x86 32-bit)
# NOTE: Basic types with static_asserts in Hooks.cpp will be loaded dynamically
# Only add types here that are NOT in Hooks.cpp
TYPE_SIZES_HARDCODED = {
    # Pointer types (32-bit x86)
    'char*': 4,
    'std::byte*': 4,
    'TileElement*': 4,
    'const TileElement*': 4,
    'World::TileElement*': 4,
    'World::TreeElement*': 4,
    'World::WallElement*': 4,
    'Town*': 4,
    'Object*': 4,
    'Company*': 4,
    'ScenarioIndexEntry*': 4,
    'ObjectManager::SelectedObjectsFlags*': 4,
    'SelectedObjectsFlags*': 4,
    'LocationOfInterestHashMap*': 4,
    'uint16_t*': 4,
    'Vehicle1*': 4,
    'Vehicle2*': 4,
    'VehicleBogie*': 4,
    'VehicleHead*': 4,
    'Vehicles::VehicleBogie*': 4,
    'Viewport*': 4,
    'const MoveInfo*': 4,


#     LandObjectFlags[getMaxObjects(ObjectType::land)]
# uint16_t[ObjectManager::getMaxObjects(ObjectType::vehicle)]
# uint32_t[enumValue(ExtColour::max)]
# uint32_t[kMaxCargoStats]
# uint8_t[256][4]
# uint8_t[kMapSize]
# uint8_t[widxToTrackTypeTab(widx::tab_track_type_7) + 1]
}

def find_loco_globals(root_dir='.'):
    # Pattern to match loco_global<type, 0xABCDEF> name
    pattern = re.compile(r'loco_global<([^,]+),\s*(0x[0-9A-Fa-f]+)>\s+(\w+)')

    # Dictionary to store: offset -> list of (type, name, file, line)
    globals_by_offset = defaultdict(list)

    # Walk through all files
    for root, dirs, files in os.walk(root_dir):
        # Skip common non-source directories
        dirs[:] = [d for d in dirs if d not in {'.git', 'build', 'cmake-build-debug', 'cmake-build-release', 'node_modules', '.venv'}]

        for file in files:
            # Only process C/C++ source and header files
            if file.endswith(('.cpp', '.h', '.hpp', '.c', '.cc', '.cxx')):
                filepath = os.path.join(root, file)
                try:
                    with open(filepath, 'r', encoding='utf-8', errors='ignore') as f:
                        for line_num, line in enumerate(f, 1):
                            matches = pattern.finditer(line)
                            for match in matches:
                                type_name = match.group(1).strip()
                                offset = match.group(2).strip()
                                var_name = match.group(3).strip()

                                # Convert offset to int for sorting
                                offset_int = int(offset, 16)

                                rel_path = os.path.relpath(filepath, root_dir)
                                globals_by_offset[offset_int].append({
                                    'type': type_name,
                                    'name': var_name,
                                    'file': rel_path,
                                    'line': line_num,
                                    'offset_str': offset
                                })
                except Exception as e:
                    print(f"Error reading {filepath}: {e}")

    return globals_by_offset

def get_all_unique_types(globals_dict):
    """Extract all unique type strings from the globals."""
    types = set()
    for entries in globals_dict.values():
        for entry in entries:
            types.add(entry['type'])
    return sorted(types)

def parse_array_type(type_str):
    """Parse array types like 'int8_t[32]' into base type and element count."""
    match = re.match(r'^(.+?)\[(.+?)\]$', type_str)
    if match:
        base_type = match.group(1).strip()
        size_expr = match.group(2).strip()
        # Try to evaluate simple expressions like "2 * 10" or just "32"
        try:
            # Handle simple multiplication
            if '*' in size_expr:
                parts = size_expr.split('*')
                count = 1
                for part in parts:
                    count *= int(part.strip())
            else:
                count = int(size_expr)
            return base_type, count
        except:
            return None, None
    return None, None

def get_type_size(type_str):
    """Get the size of a type in bytes. Returns None if unknown."""
    # Direct lookup
    if type_str in TYPE_SIZES:
        return TYPE_SIZES[type_str]

    # Handle array types
    base_type, count = parse_array_type(type_str)
    if base_type and count:
        base_size = get_type_size(base_type)
        if base_size:
            return base_size * count

    return None

def calculate_file_offset(vma_address):
    """Calculate file offset from virtual memory address (from extract_track_coordinates_4f6f8c.py)."""
    DATA_SEGMENT_VMA = 0x4D7000
    DATA_SEGMENT_FILE_OFFSET = 215 * 4096  # 880640 bytes

    if vma_address < DATA_SEGMENT_VMA:
        # Could be in text segment, but we only care about data segment for zero-fill check
        return None

    offset_in_data = vma_address - DATA_SEGMENT_VMA
    return DATA_SEGMENT_FILE_OFFSET + offset_in_data

def is_zero_filled(exe_path, vma_address, size):
    """Check if a memory region in loco.exe is zero-filled."""
    if not exe_path or not os.path.exists(exe_path):
        return None  # Can't check

    file_offset = calculate_file_offset(vma_address)
    if file_offset is None:
        return None  # Not in data segment

    try:
        with open(exe_path, 'rb') as f:
            f.seek(file_offset)
            data = f.read(size)
            if len(data) != size:
                return None  # Couldn't read full size
            return all(b == 0 for b in data)
    except:
        return None  # Error reading

def check_warnings(globals_dict, sorted_offsets, exe_path=None):
    """Check for warnings: multiple types at same offset, overlapping ranges, non-zero-filled data."""
    warnings = []

    for i, offset in enumerate(sorted_offsets):
        entries = globals_dict[offset]

        # Check for multiple different types at same offset
        unique_types = set(entry['type'] for entry in entries)
        if len(unique_types) > 1:
            warnings.append({
                'type': 'multiple_types',
                'offset': offset,
                'offset_hex': entries[0]['offset_str'],
                'types': list(unique_types)
            })

        # Check for overlapping with next offset
        if i + 1 < len(sorted_offsets):
            next_offset = sorted_offsets[i + 1]
            # Use the first entry's type to calculate size
            type_str = entries[0]['type']
            size = get_type_size(type_str)
            if size:
                end_offset = offset + size
                if end_offset > next_offset:
                    warnings.append({
                        'type': 'overlap',
                        'offset': offset,
                        'offset_hex': entries[0]['offset_str'],
                        'size': size,
                        'end_offset': end_offset,
                        'next_offset': next_offset,
                        'next_offset_hex': globals_dict[next_offset][0]['offset_str'],
                        'var_type': type_str
                    })

        # Check if offset is NOT zero-filled in the exe
        if exe_path:
            type_str = entries[0]['type']
            size = get_type_size(type_str)
            if size:
                zero_filled = is_zero_filled(exe_path, offset, size)
                if zero_filled is False:  # Explicitly False, not None
                    warnings.append({
                        'type': 'not_zero_filled',
                        'offset': offset,
                        'offset_hex': entries[0]['offset_str'],
                        'size': size,
                        'var_type': type_str,
                        'var_name': entries[0]['name']
                    })

    return warnings

def output_xml(globals_dict, sorted_offsets):
    """Output results in XML format."""
    root = ET.Element('loco_globals')

    total_instances = sum(len(v) for v in globals_dict.values())
    root.set('total_instances', str(total_instances))
    root.set('unique_offsets', str(len(sorted_offsets)))

    for offset in sorted_offsets:
        entries = globals_dict[offset]
        offset_str = entries[0]['offset_str']

        offset_elem = ET.SubElement(root, 'offset')
        offset_elem.set('hex', offset_str)
        offset_elem.set('decimal', str(offset))

        for entry in entries:
            instance = ET.SubElement(offset_elem, 'instance')
            instance.set('type', entry['type'])
            instance.set('name', entry['name'])
            instance.set('file', entry['file'])
            instance.set('line', str(entry['line']))

    # Pretty print XML
    xml_str = minidom.parseString(ET.tostring(root)).toprettyxml(indent='  ')
    print(xml_str)

def output_text(globals_dict, sorted_offsets):
    """Output results in human-readable text format."""
    print(f"Found {sum(len(v) for v in globals_dict.values())} loco_global instances at {len(sorted_offsets)} unique offsets\n")
    print("=" * 100)

    for offset in sorted_offsets:
        entries = globals_dict[offset]
        offset_str = entries[0]['offset_str']

        print(f"\nOffset: {offset_str} ({offset})")
        print("-" * 100)

        for entry in entries:
            print(f"  {entry['type']:<30} {entry['name']:<40} @ {entry['file']}:{entry['line']}")

    print("\n" + "=" * 100)
    print(f"\nTotal: {sum(len(v) for v in globals_dict.values())} instances")

def main():
    parser = argparse.ArgumentParser(
        description='Search for loco_global instances in the codebase'
    )
    parser.add_argument(
        '--format',
        choices=['text', 'xml'],
        default='text',
        help='Output format (default: text)'
    )
    parser.add_argument(
        '--exe',
        type=str,
        help='Path to loco.exe for zero-fill checking'
    )

    args = parser.parse_args()

    # Load type sizes from all static_asserts in the codebase
    parsed_types = parse_static_asserts_from_files()

    # Merge with hardcoded types, checking for duplicates
    global TYPE_SIZES
    TYPE_SIZES = {}

    # Check for duplicates between parsed and hardcoded map
    duplicates = set(parsed_types.keys()) & set(TYPE_SIZES_HARDCODED.keys())
    if duplicates:
        print("ERROR: Duplicate types found in both codebase static_asserts and hardcoded map:", file=sys.stderr)
        for dup in sorted(duplicates):
            print(f"  {dup}: parsed={parsed_types[dup]}, hardcoded={TYPE_SIZES_HARDCODED[dup]}", file=sys.stderr)
        sys.exit(1)

    # Merge the two sources
    TYPE_SIZES.update(parsed_types)
    TYPE_SIZES.update(TYPE_SIZES_HARDCODED)

    if args.format == 'text':
        print("Searching for loco_global instances...\n")

    globals_dict = find_loco_globals()

    if not globals_dict:
        if args.format == 'text':
            print("No loco_global instances found.")
        else:
            root = ET.Element('loco_globals')
            root.set('total_instances', '0')
            root.set('unique_offsets', '0')
            print(minidom.parseString(ET.tostring(root)).toprettyxml(indent='  '))
        return

    # Sort by offset
    sorted_offsets = sorted(globals_dict.keys())

    # Check for unknown types
    unique_types = get_all_unique_types(globals_dict)
    unknown_types = []
    for type_str in unique_types:
        if get_type_size(type_str) is None:
            unknown_types.append(type_str)

    if unknown_types:
        print("ERROR: Unknown types found:", file=sys.stderr)
        for t in unknown_types:
            print(f"  {t}", file=sys.stderr)
        sys.exit(1)

    # ANSI color codes
    RED = '\033[91m'
    YELLOW = '\033[93m'
    RESET = '\033[0m'

    # Check for warnings
    warnings = check_warnings(globals_dict, sorted_offsets, args.exe)
    if warnings:
        print("WARNINGS:", file=sys.stderr)
        for warning in warnings:
            if warning['type'] == 'multiple_types':
                print(f"  Offset {warning['offset_hex']} has multiple types: {', '.join(warning['types'])}", file=sys.stderr)
            elif warning['type'] == 'overlap':
                print(f"{YELLOW}  Offset {warning['offset_hex']} (type {warning['var_type']}, size {warning['size']}) overlaps next offset {warning['next_offset_hex']} (ends at {warning['end_offset']}){RESET}", file=sys.stderr)
            elif warning['type'] == 'not_zero_filled':
                print(f"{RED}  Offset {warning['offset_hex']} ({warning['var_name']}: {warning['var_type']}, size {warning['size']}) is NOT zero-filled in exe{RESET}", file=sys.stderr)
        print(file=sys.stderr)

    if args.format == 'xml':
        output_xml(globals_dict, sorted_offsets)
    else:
        output_text(globals_dict, sorted_offsets)

if __name__ == '__main__':
    main()
