import sys
import os
import struct

def parse_ar_header(data):
    if len(data) != 60:
        return None
    name = data[0:16].decode('ascii').strip()
    # date = data[16:28]
    # uid = data[28:34]
    # gid = data[34:40]
    # mode = data[40:48]
    size_str = data[48:58].decode('ascii').strip()
    fmag = data[58:60]
    
    if fmag != b'`\n':
        raise ValueError("Invalid header magic")
    
    return name, int(size_str)

def analyze_lib(lib_path):
    print(f"Analyzing {lib_path}...")
    file_size = os.path.getsize(lib_path)
    print(f"Total file size: {file_size} bytes ({file_size/1024:.2f} KB)")

    with open(lib_path, 'rb') as f:
        signature = f.read(8)
        if signature != b'!<arch>\n':
            print("Error: Not a valid .lib/.ar file")
            return

        long_names = None
        members = []
        
        while True:
            header_data = f.read(60)
            if not header_data:
                break
            if len(header_data) < 60:
                print("Warning: Truncated header at end of file")
                break
                
            name, size = parse_ar_header(header_data)
            
            # Position of the data
            data_pos = f.tell()
            
            real_name = name
            
            # Handle special members
            if name == '/':
                real_name = "<Symbol Table>"
            elif name == '//':
                real_name = "<String Table>"
                # Read the string table
                long_names_data = f.read(size)
                long_names = long_names_data
                # Seek back because we consumed the data to read names? 
                # No, we consumed it, that's correct for the file pointer.
                # But we need to handle the padding for the loop.
                if size % 2 != 0:
                    f.seek(1, 1)
                continue
            elif name.startswith('/'):
                try:
                    offset = int(name[1:])
                    if long_names:
                        # Find the null terminator starting from offset
                        end = long_names.find(b'\0', offset)
                        if end != -1:
                            real_name = long_names[offset:end].decode('utf-8', errors='replace')
                        else:
                            real_name = long_names[offset:].decode('utf-8', errors='replace')
                except ValueError:
                    pass

            members.append({
                'name': real_name,
                'size': size,
                'offset': data_pos
            })
            
            # Skip data
            f.seek(size, 1)
            
            # Pad to even byte
            if size % 2 != 0:
                f.seek(1, 1)

    # Sort by size descending
    members.sort(key=lambda x: x['size'], reverse=True)
    
    print("\n--- Member Breakdown (Top 20) ---")
    print(f"{'Size (Bytes)':<12} | {'Size (KB)':<10} | {'Name'}")
    print("-" * 50)
    
    total_member_size = 0
    for m in members:
        total_member_size += m['size']
        
    for i, m in enumerate(members[:20]):
        print(f"{m['size']:<12} | {m['size']/1024:<10.2f} | {m['name']}")
        
    print("-" * 50)
    print(f"Total size of members: {total_member_size} bytes")
    print(f"Overhead/Padding: {file_size - total_member_size} bytes")

if __name__ == "__main__":
    target_lib = r"e:\c++\ShineEngine\exe\base64d.lib"
    if len(sys.argv) > 1:
        target_lib = sys.argv[1]
    
    if not os.path.exists(target_lib):
        print(f"File not found: {target_lib}")
    else:
        analyze_lib(target_lib)
