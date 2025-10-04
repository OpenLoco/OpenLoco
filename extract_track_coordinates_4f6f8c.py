#!/usr/bin/env python3
"""
Extract data arrays from loco.exe

Based on CMakeLists.txt segment layout:
- Text segment: VMA 0x401000, file offset = 4096 bytes
- Data segment: VMA 0x4D7000, file offset = 880640 bytes (215 * 4096)

Supported data types:
1. TrackCoordinates structure (8 bytes):
    uint8_t rotationBegin; // 0x00
    uint8_t rotationEnd;   // 0x01
    World::Pos3 pos;       // 0x02 (3 x int16_t = 6 bytes)
        int16_t x;
        int16_t y;
        int16_t z;

2. Height offset pairs (2 bytes):
    int8_t firstTileOffset;  // 0x00
    int8_t lastTileOffset;   // 0x01

3. Raw byte array:
    uint8_t data[N];

4. Int32 array:
    int32_t data[N];

Usage:
    # TrackCoordinates
    python3 extract_track_coordinates_4f6f8c.py loco.exe 0x4F6F8C 80 --type coords
    python3 extract_track_coordinates_4f6f8c.py loco.exe 0x4F7B5C 352 --type coords

    # Height offset pairs (roads)
    python3 extract_track_coordinates_4f6f8c.py loco.exe 0x4F720C 10 --type height_offsets --enum RoadId

    # Height offset pairs (tracks)
    python3 extract_track_coordinates_4f6f8c.py loco.exe 0x4F86B4 44 --type height_offsets --enum TrackId

    # Raw byte array
    python3 extract_track_coordinates_4f6f8c.py loco.exe 0x4F7358 128 --type raw_bytes

    # Int32 array
    python3 extract_track_coordinates_4f6f8c.py loco.exe 0x4FEAB8 32 --type int32_array
"""

import struct
import sys

# Enum value names for different types
ROAD_ID_NAMES = [
    'straight',
    'leftCurveVerySmall',
    'rightCurveVerySmall',
    'leftCurveSmall',
    'rightCurveSmall',
    'straightSlopeUp',
    'straightSlopeDown',
    'straightSteepSlopeUp',
    'straightSteepSlopeDown',
    'turnaround',
]

TRACK_ID_NAMES = [
    'straight',
    'diagonal',
    'leftCurveVerySmall',
    'rightCurveVerySmall',
    'leftCurveSmall',
    'rightCurveSmall',
    'leftCurve',
    'rightCurve',
    'leftCurveLarge',
    'rightCurveLarge',
    'diagonalLeftCurveLarge',
    'diagonalRightCurveLarge',
    'sBendLeft',
    'sBendRight',
    'straightSlopeUp',
    'straightSlopeDown',
    'straightSteepSlopeUp',
    'straightSteepSlopeDown',
    'leftCurveSmallSlopeUp',
    'rightCurveSmallSlopeUp',
    'leftCurveSmallSlopeDown',
    'rightCurveSmallSlopeDown',
    'leftCurveSmallSteepSlopeUp',
    'rightCurveSmallSteepSlopeUp',
    'leftCurveSmallSteepSlopeDown',
    'rightCurveSmallSteepSlopeDown',
    'unkStraight1',
    'unkStraight2',
    'unkLeftCurveVerySmall1',
    'unkLeftCurveVerySmall2',
    'unkRightCurveVerySmall1',
    'unkRightCurveVerySmall2',
    'unkSBendRight',
    'unkSBendLeft',
    'unkStraightSteepSlopeUp1',
    'unkStraightSteepSlopeUp2',
    'unkStraightSteepSlopeDown1',
    'unkStraightSteepSlopeDown2',
    'sBendToDualTrack',
    'sBendToSingleTrack',
    'unkSBendToDualTrack',
    'unkSBendToSingleTrack',
    'turnaround',
    'unkTurnaround',
]

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

def extract_height_offsets(exe_path, target_vma, num_entries):
    """Extract height offset pairs from loco.exe."""
    STRUCT_SIZE = 2  # 2 x int8_t

    file_offset = calculate_file_offset(target_vma)

    print(f"Extracting height offset pairs[{num_entries}] from {exe_path}")
    print(f"Virtual address: 0x{target_vma:08X}")
    print(f"File offset: 0x{file_offset:08X} ({file_offset} bytes)")
    print(f"Pair size: {STRUCT_SIZE} bytes")
    print(f"Total data size: {num_entries * STRUCT_SIZE} bytes")
    print()

    with open(exe_path, 'rb') as f:
        f.seek(file_offset)

        entries = []
        for i in range(num_entries):
            # Read 2 bytes for each pair
            data = f.read(STRUCT_SIZE)
            if len(data) != STRUCT_SIZE:
                print(f"Error: Could not read entry {i}")
                break

            # Unpack: 2 x int8_t (signed)
            first_offset, last_offset = struct.unpack('<bb', data)

            entries.append({
                'index': i,
                'first_offset': first_offset,
                'last_offset': last_offset
            })

        return entries

def extract_raw_bytes(exe_path, target_vma, num_bytes):
    """Extract raw byte array from loco.exe."""
    file_offset = calculate_file_offset(target_vma)

    print(f"Extracting raw bytes[{num_bytes}] from {exe_path}")
    print(f"Virtual address: 0x{target_vma:08X}")
    print(f"File offset: 0x{file_offset:08X} ({file_offset} bytes)")
    print(f"Total data size: {num_bytes} bytes")
    print()

    with open(exe_path, 'rb') as f:
        f.seek(file_offset)
        data = f.read(num_bytes)
        if len(data) != num_bytes:
            raise ValueError(f"Could only read {len(data)} bytes of {num_bytes}")
        return list(data)

def extract_int32_array(exe_path, target_vma, num_entries):
    """Extract int32_t array from loco.exe."""
    STRUCT_SIZE = 4  # int32_t is 4 bytes
    total_bytes = num_entries * STRUCT_SIZE

    file_offset = calculate_file_offset(target_vma)

    print(f"Extracting int32_t array[{num_entries}] from {exe_path}")
    print(f"Virtual address: 0x{target_vma:08X}")
    print(f"File offset: 0x{file_offset:08X} ({file_offset} bytes)")
    print(f"Entry size: {STRUCT_SIZE} bytes")
    print(f"Total data size: {total_bytes} bytes")
    print()

    with open(exe_path, 'rb') as f:
        f.seek(file_offset)

        entries = []
        for i in range(num_entries):
            data = f.read(STRUCT_SIZE)
            if len(data) != STRUCT_SIZE:
                raise ValueError(f"Could not read entry {i}")

            # Unpack as little-endian signed int32
            value = struct.unpack('<i', data)[0]
            entries.append(value)

        return entries

def format_coords_output(entries, address, count):
    """Format TrackCoordinates entries as C++ code."""
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

def format_height_offsets_output(entries, address, count, enum_name):
    """Format height offset entries as C++ code with enum indexing."""
    # Get enum names
    if enum_name == 'RoadId':
        enum_names = ROAD_ID_NAMES
        namespace = 'World::Track::RoadId::'
    elif enum_name == 'TrackId':
        enum_names = TRACK_ID_NAMES
        namespace = 'World::Track::TrackId::'
    else:
        enum_names = None
        namespace = ''

    print(f"// 0x{address:X}")
    print(f"// Height offset pairs for decorations (height markers, direction arrows)")
    print(f"// Indexed by {enum_name}")
    print(f"struct DecorationHeightOffsets")
    print(f"{{")
    print(f"    int8_t firstTile;")
    print(f"    int8_t lastTile;")
    print(f"}};")
    print()
    print(f"static constexpr std::array<DecorationHeightOffsets, {count}> kDecorationHeightOffsets = {{{{")

    for entry in entries:
        first = entry['first_offset']
        last = entry['last_offset']
        idx = entry['index']

        if enum_names and idx < len(enum_names):
            comment = f"// {namespace}{enum_names[idx]}"
        else:
            comment = f"// {idx}"

        print(f"    {{ {first:4}, {last:4} }},  {comment}")

    print("}};")
    print()
    print(f"// Helper function to get the appropriate offset")
    print(f"constexpr int8_t getDecorationHeightOffset(bool isFirstTile, {enum_name} id)")
    print(f"{{")
    print(f"    const auto& offsets = kDecorationHeightOffsets[static_cast<uint8_t>(id)];")
    print(f"    return isFirstTile ? offsets.firstTile : offsets.lastTile;")
    print(f"}}")

def format_raw_bytes_output(bytes_data, address, count):
    """Format raw bytes as C++ code."""
    print(f"// 0x{address:X}")
    print(f"// Lane occupation mask lookup table")
    print(f"// Indexed by (trackAndDirection._data >> 2)")
    print(f"// Upper nibble (>> 4) contains 2-bit lane occupation mask")
    print(f"static constexpr std::array<uint8_t, {count}> kRoadOccupationMasks = {{{{")

    # Format 16 bytes per line for readability
    for i in range(0, len(bytes_data), 16):
        chunk = bytes_data[i:i+16]
        hex_values = ', '.join(f'0x{b:02X}' for b in chunk)
        print(f"    {hex_values},  // 0x{i:02X}-0x{i+len(chunk)-1:02X}")

    print("}};")

def format_int32_array_output(int32_data, address, count):
    """Format int32 array as C++ code."""
    print(f"// 0x{address:X}")
    print(f"// Sound volume levels indexed by SoundId")
    print(f"static constexpr std::array<int32_t, {count}> kSoundVolumeTable = {{ {{")
    print(f"    // clang-format off")

    # Format 8 values per line for readability
    for i in range(0, len(int32_data), 8):
        chunk = int32_data[i:i+8]
        values = ', '.join(f'{v:4}' for v in chunk)
        print(f"    {values},")

    print(f"    // clang-format on")
    print(f"}} }};")

def main():
    if len(sys.argv) < 4:
        print("Usage: python3 extract_track_coordinates_4f6f8c.py <exe_path> <address> <count> [--type coords|height_offsets|raw_bytes|int32_array] [--enum RoadId|TrackId]")
        print("Examples:")
        print("  python3 extract_track_coordinates_4f6f8c.py loco.exe 0x4F6F8C 80 --type coords")
        print("  python3 extract_track_coordinates_4f6f8c.py loco.exe 0x4F7B5C 352 --type coords")
        print("  python3 extract_track_coordinates_4f6f8c.py loco.exe 0x4F720C 10 --type height_offsets --enum RoadId")
        print("  python3 extract_track_coordinates_4f6f8c.py loco.exe 0x4F86B4 44 --type height_offsets --enum TrackId")
        print("  python3 extract_track_coordinates_4f6f8c.py loco.exe 0x4F7358 128 --type raw_bytes")
        print("  python3 extract_track_coordinates_4f6f8c.py loco.exe 0x4FEAB8 32 --type int32_array")
        sys.exit(1)

    exe_path = sys.argv[1]
    address_str = sys.argv[2]
    count_str = sys.argv[3]

    # Parse optional arguments
    data_type = 'coords'  # default
    enum_name = 'RoadId'  # default

    if '--type' in sys.argv:
        type_idx = sys.argv.index('--type')
        if type_idx + 1 < len(sys.argv):
            data_type = sys.argv[type_idx + 1]

    if '--enum' in sys.argv:
        enum_idx = sys.argv.index('--enum')
        if enum_idx + 1 < len(sys.argv):
            enum_name = sys.argv[enum_idx + 1]

    # Parse address (handle hex with or without 0x prefix)
    if address_str.startswith('0x') or address_str.startswith('0X'):
        address = int(address_str, 16)
    else:
        address = int(address_str, 16)

    count = int(count_str)

    try:
        if data_type == 'coords':
            entries = extract_track_coordinates(exe_path, address, count)
            print(f"\nSuccessfully extracted {len(entries)} entries\n")
            print("=" * 70)
            format_coords_output(entries, address, count)
        elif data_type == 'height_offsets':
            entries = extract_height_offsets(exe_path, address, count)
            print(f"\nSuccessfully extracted {len(entries)} entries\n")
            print("=" * 70)
            format_height_offsets_output(entries, address, count, enum_name)
        elif data_type == 'raw_bytes':
            bytes_data = extract_raw_bytes(exe_path, address, count)
            print(f"\nSuccessfully extracted {len(bytes_data)} bytes\n")
            print("=" * 70)
            format_raw_bytes_output(bytes_data, address, count)
        elif data_type == 'int32_array':
            int32_data = extract_int32_array(exe_path, address, count)
            print(f"\nSuccessfully extracted {len(int32_data)} int32 values\n")
            print("=" * 70)
            format_int32_array_output(int32_data, address, count)
        else:
            print(f"Error: Unknown data type '{data_type}'")
            sys.exit(1)

    except FileNotFoundError:
        print(f"Error: Could not find {exe_path}")
        sys.exit(1)
    except Exception as e:
        print(f"Error: {e}")
        sys.exit(1)

if __name__ == '__main__':
    main()

