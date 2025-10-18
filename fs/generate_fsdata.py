# Script to generate fsdata.c for lwIP HTTP server
# fsdata.c format based on lwIP's makefsdata tool
# lwIP: https://savannah.nongnu.org/projects/lwip/

import os

def create_fsdata():
    # Read the HTML file
    with open('index.html', 'rb') as f:
        html_content = f.read()
    
    filename = "/index.html"
    filename_bytes = filename.encode('ascii') + b'\x00'
    
    # Generate fsdata.c
    with open('fsdata.c', 'w') as f:
        f.write('#include "lwip/apps/fs.h"\n')
        f.write('#include "lwip/def.h"\n')
        f.write('#include "lwip/apps/httpd.h"\n\n')
        
        # Write filename data
        f.write('static const unsigned char data_index_html_name[] = {\n')
        f.write('/* /index.html filename */\n')
        for i in range(0, len(filename_bytes), 16):
            chunk = filename_bytes[i:i+16]
            hex_string = ', '.join(f'0x{b:02x}' for b in chunk)
            f.write(f'{hex_string},\n')
        f.write('};\n\n')
        
        # Write HTTP header
        http_header = (
            b'HTTP/1.0 200 OK\r\n'
            b'Server: lwIP/2.1.0\r\n'
            b'Content-Type: text/html\r\n\r\n'
        )
        
        f.write('static const unsigned char data_index_html_header[] = {\n')
        f.write('/* HTTP header */\n')
        for i in range(0, len(http_header), 16):
            chunk = http_header[i:i+16]
            hex_string = ', '.join(f'0x{b:02x}' for b in chunk)
            f.write(f'{hex_string},\n')
        f.write('};\n\n')
        
        # Write the HTML content data
        f.write('static const unsigned char data_index_html_content[] = {\n')
        f.write('/* HTML content */\n')
        for i in range(0, len(html_content), 16):
            chunk = html_content[i:i+16]
            hex_string = ', '.join(f'0x{b:02x}' for b in chunk)
            f.write(f'{hex_string},\n')
        f.write('};\n\n')
        
        # Create file structure
        f.write('const struct fsdata_file file_index_html[] = {{\n')
        f.write('  NULL,\n')
        f.write('  (const char *)data_index_html_name,\n')
        f.write('  (const char *)data_index_html_content,\n')
        f.write('  sizeof(data_index_html_content),\n')
        f.write('  FS_FILE_FLAGS_HEADER_INCLUDED\n')
        f.write('}};\n\n')
        
        f.write('#define FS_ROOT file_index_html\n')
        f.write('#define FS_NUMFILES 1\n')

if __name__ == '__main__':
    create_fsdata()
    print("fsdata.c generated successfully!")