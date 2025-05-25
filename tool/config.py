import xml.etree.ElementTree as ET
import os

def get_c_type(bit_len):
    type_map = {
        '32': 'int32',
        '16': 'int16',
        '8': 'int8'
    }
    return type_map.get(bit_len, 'int32')

def parse_ethercat_xml(xml_file):
    tree = ET.parse(xml_file)
    root = tree.getroot()
    all_entries = []
    
    for slave in root.findall('slave'):
        slave_id = slave.get('ID')
        entries_in_slave = []
        
        for sm in slave.findall('syncManager'):
            sm_idx = sm.get('idx')
            # 获取输入和输出的PDO条目
            if sm_idx in ['2', '3']:  # 同时处理输入输出PDO
                for pdo in sm.findall('pdo'):
                    for entry in pdo.findall('pdoEntry'):
                        entry_info = {
                            'name': entry.get('name'),
                            'slave_id': slave_id,
                            'bitLen': entry.get('bitLen'),
                            'sm_idx': sm_idx,  # 添加同步管理器索引信息
                            'idx': entry.get('idx'),
                            'subIdx': entry.get('subIdx')
                        }
                        entries_in_slave.append(entry_info)
        
        # 按照同步管理器索引排序，确保输出PDO在前
        entries_in_slave.sort(key=lambda x: (x['sm_idx'], x['name']))
        all_entries.extend(entries_in_slave)
    
    return all_entries

def generate_header(entries, header_file):
    with open(header_file, 'w') as f:
        f.write('#ifndef ETHERCAT_ENTRIES_H\n')
        f.write('#define ETHERCAT_ENTRIES_H\n\n')
        
        current_slave = None
        index_sm2 = 0 
        index_sm3=0
        for entry in entries:
            if current_slave != entry['slave_id']:
                current_slave = entry['slave_id']
                f.write(f'\n// Slave {current_slave}\n')
                index_sm2 = 0
                index_sm3 = 0
            if entry['sm_idx'] == '2':
                if index_sm2 == 0:
                    f.write(f'\n   //outputs\n')
                c_type = get_c_type(entry['bitLen'])
                macro_name = f"{entry['name']}_{current_slave}"
                f.write(f'    #define {macro_name} {current_slave} {index_sm2} {c_type}\n')
                index_sm2 += 1
            if entry['sm_idx'] == '3': 
                if index_sm3 == 0:
                    f.write(f'\n   //inputs\n')
                c_type = get_c_type(entry['bitLen'])
                macro_name = f"{entry['name']}_{current_slave}"
                f.write(f'    #define {macro_name} {current_slave} {index_sm3} {c_type}\n')
                index_sm3 += 1
            
        f.write('\n#endif // ETHERCAT_ENTRIES_H\n')

if __name__ == "__main__":
    # Get script directory and project root
    project_root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    
    # Define input and output paths
    xml_file = os.path.join(project_root, 'config', 'ethercat.xml')
    build_dir = os.path.join(project_root, 'build')
    header_file = os.path.join(build_dir, 'ethercat_pdo.h')
    
    # Create build directory if it doesn't exist
    os.makedirs(build_dir, exist_ok=True)
    
    entries = parse_ethercat_xml(xml_file)
    generate_header(entries, header_file)
