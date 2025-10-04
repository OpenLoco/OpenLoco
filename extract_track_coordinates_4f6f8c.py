#!/usr/bin/env python3
"""
Extract TrackCoordinates arrays from loco.exe

Based on CMakeLists.txt segment layout:
- Text segment: VMA 0x401000, file offset = 4096 bytes
- Data segment: VMA 0x4D7000, file offset = 880640 bytes (215 * 4096)

TrackCoordinates structure (8 bytes):
    uint8_t rotationBegin; // 0x00
    uint8_t rotationEnd;   // 0x01
    World::Pos3 pos;       // 0x02 (3 x int16_t = 6 bytes)
        int16_t x;
        int16_t y;
        int16_t z;

Usage:
    python3 extract_track_coordinates_4f6f8c.py loco.exe 0x4F6F8C 80
    python3 extract_track_coordinates_4f6f8c.py loco.exe 0x4F7B5C 352
"""

import struct
import sys

def calculate_file_offset(vma_address):
    """Calculate file offset from virtual memory address."""
    DATA_SEGMENT_VMA = 0x4D7000
    DATA_SEGMENT_FILE_OFFSET = 215 * 4096  # 880640 bytes

    if vma_address < DATA_SEGMENT_VMA:
        raise ValueError(f"Address 0x{vma_address:08X} is not in data segment")

    offset_in_data = vma_address - DATA_SEGMENT_VMA
    return DATA_SEGMENT_FILE_OFFSET + offset_in_data

def extract_track_coordinates(exe_path, target_vma, num_entries):
    """Extract TrackCoordinates array from loco.exe."""
    STRUCT_SIZE = 8  # TrackCoordinates is 8 bytes

    file_offset = calculate_file_offset(target_vma)

    print(f"Extracting TrackCoordinates[{num_entries}] from {exe_path}")
    print(f"Virtual address: 0x{target_vma:08X}")
    print(f"File offset: 0x{file_offset:08X} ({file_offset} bytes)")
    print(f"Structure size: {STRUCT_SIZE} bytes")
    print(f"Total data size: {num_entries * STRUCT_SIZE} bytes")
    print()

    with open(exe_path, 'rb') as f:
        f.seek(file_offset)

        entries = []
        for i in range(num_entries):
            # Read 8 bytes for each TrackCoordinates entry
            data = f.read(STRUCT_SIZE)
            if len(data) != STRUCT_SIZE:
                print(f"Error: Could not read entry {i}")
                break

            # Unpack: 2 bytes (2 x uint8_t), then 6 bytes (3 x int16_t)
            rotation_begin, rotation_end, x, y, z = struct.unpack('<BBhhh', data)

            entries.append({
                'index': i,
                'rotation_begin': rotation_begin,
                'rotation_end': rotation_end,
                'x': x,
                'y': y,
                'z': z
            })

        return entries

def format_cpp_output(entries, address, count):
    """Format entries as C++ code."""
    print(f"// 0x{address:X}")
    print(f"static constexpr std::array<TrackCoordinates, {count}> kTrackCoordinates = {{{{")

    for entry in entries:
        rot_begin = entry['rotation_begin']
        rot_end = entry['rotation_end']
        x = entry['x']
        y = entry['y']
        z = entry['z']
        idx = entry['index']

        print(f"    {{ {rot_begin:3}, {rot_end:3}, {{ {x:4}, {y:4}, {z:4} }} }},  // {idx}")

    print("}};")

def main():
    if len(sys.argv) < 4:
        print("Usage: python3 extract_track_coordinates_4f6f8c.py <exe_path> <address> <count>")
        print("Examples:")
        print("  python3 extract_track_coordinates_4f6f8c.py loco.exe 0x4F6F8C 80")
        print("  python3 extract_track_coordinates_4f6f8c.py loco.exe 0x4F7B5C 352")
        sys.exit(1)

    exe_path = sys.argv[1]
    address_str = sys.argv[2]
    count_str = sys.argv[3]

    # Parse address (handle hex with or without 0x prefix)
    if address_str.startswith('0x') or address_str.startswith('0X'):
        address = int(address_str, 16)
    else:
        address = int(address_str, 16)

    count = int(count_str)

    try:
        entries = extract_track_coordinates(exe_path, address, count)
        print(f"\nSuccessfully extracted {len(entries)} entries\n")
        print("=" * 70)
        format_cpp_output(entries, address, count)

    except FileNotFoundError:
        print(f"Error: Could not find {exe_path}")
        sys.exit(1)
    except Exception as e:
        print(f"Error: {e}")
        sys.exit(1)

if __name__ == '__main__':
    main()

