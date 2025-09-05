#include "parser.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <map>
#include <string>

using namespace gpr;
using namespace std;

// 结构体用于存储G代码命令的完整信息
struct GCodeCommand {
    char command_type;  // G, M, T, S, F等
    int command_number; // 命令编号
    map<char, double> parameters; // 参数映射 (X->10.5, Y->20.3等)
    string comment;     // 注释内容
    int block_index;    // 所在块的索引
    
    void print() const {
        cout << "块[" << block_index << "]: ";
        if (command_type != '\0') {
            cout << command_type << command_number;
        }
        
        for (const auto& param : parameters) {
            cout << " " << param.first << param.second;
        }
        
        if (!comment.empty()) {
            cout << " ; " << comment;
        }
        cout << endl;
    }
};

// 从gcode_program中提取所有命令和参数
vector<GCodeCommand> extract_all_commands(const gcode_program& program) {
    vector<GCodeCommand> commands;
    
    // 遍历程序中的每个块
    for (int i = 0; i < program.num_blocks(); i++) {
        block b = program.get_block(i);
        
        GCodeCommand cmd;
        cmd.block_index = i;
        cmd.command_type = '\0';
        cmd.command_number = -1;
        
        // 遍历块中的每个chunk
        for (int j = 0; j < b.size(); j++) {
            chunk c = b.get_chunk(j);
            
            if (c.tp() == CHUNK_TYPE_WORD_ADDRESS) {
                char word = c.get_word();
                addr address = c.get_address();
                double value = address.double_value();
                
                // 识别主要命令类型
                if (word == 'G' || word == 'M' || word == 'T') {
                    cmd.command_type = word;
                    cmd.command_number = address.int_value();
                }
                // 存储所有参数
                else {
                    cmd.parameters[word] = value;
                }
            }
            else if (c.tp() == CHUNK_TYPE_COMMENT) {
                cmd.comment = c.get_comment_text();
            }
        }
        
        // 只有当块包含有效内容时才添加到结果中
        if (cmd.command_type != '\0' || !cmd.parameters.empty() || !cmd.comment.empty()) {
            commands.push_back(cmd);
        }
    }
    
    return commands;
}

// 按命令类型过滤
vector<GCodeCommand> filter_by_command(const vector<GCodeCommand>& commands, 
                                      char command_type, int command_number = -1) {
    vector<GCodeCommand> filtered;
    
    for (const auto& cmd : commands) {
        if (cmd.command_type == command_type) {
            if (command_number == -1 || cmd.command_number == command_number) {
                filtered.push_back(cmd);
            }
        }
    }
    
    return filtered;
}

// 统计命令使用频率
map<string, int> count_command_usage(const vector<GCodeCommand>& commands) {
    map<string, int> usage_count;
    
    for (const auto& cmd : commands) {
        if (cmd.command_type != '\0') {
            string key = string(1, cmd.command_type) + to_string(cmd.command_number);
            usage_count[key]++;
        }
    }
    
    return usage_count;
}

// 查找包含特定参数的命令
vector<GCodeCommand> find_commands_with_parameter(const vector<GCodeCommand>& commands, 
                                                char parameter) {
    vector<GCodeCommand> result;
    
    for (const auto& cmd : commands) {
        if (cmd.parameters.find(parameter) != cmd.parameters.end()) {
            result.push_back(cmd);
        }
    }
    
    return result;
}

int main() {
    // 读取G代码文件
    string file_path = "gcode_samples/cura_3D_printer.gcode";
    ifstream file(file_path);
    
    if (!file.is_open()) {
        cerr << "无法打开文件: " << file_path << endl;
        return 1;
    }
    
    string file_contents((istreambuf_iterator<char>(file)),
                        istreambuf_iterator<char>());
    
    // 解析G代码
    gcode_program program = parse_gcode(file_contents);
    
    cout << "=== G代码解析结果 ===" << endl;
    cout << "总块数: " << program.num_blocks() << endl;
    
    // 提取所有命令
    vector<GCodeCommand> all_commands = extract_all_commands(program);
    cout << "有效命令数: " << all_commands.size() << endl << endl;
    
    // 显示前10个命令作为示例
    cout << "=== 前10个命令示例 ===" << endl;
    for (int i = 0; i < min(10, (int)all_commands.size()); i++) {
        all_commands[i].print();
    }
    cout << endl;
    
    // 统计命令使用频率
    cout << "=== 命令使用频率统计 ===" << endl;
    auto usage = count_command_usage(all_commands);
    for (const auto& pair : usage) {
        cout << pair.first << ": " << pair.second << " 次" << endl;
    }
    cout << endl;
    
    // 查找特定命令
    cout << "=== G1命令示例 (前5个) ===" << endl;
    auto g1_commands = filter_by_command(all_commands, 'G', 1);
    for (int i = 0; i < min(5, (int)g1_commands.size()); i++) {
        g1_commands[i].print();
    }
    cout << endl;
    
    cout << "=== M命令示例 (前5个) ===" << endl;
    auto m_commands = filter_by_command(all_commands, 'M');
    for (int i = 0; i < min(5, (int)m_commands.size()); i++) {
        m_commands[i].print();
    }
    cout << endl;
    
    // 查找包含特定参数的命令
    cout << "=== 包含E参数的命令 (前5个) ===" << endl;
    auto e_commands = find_commands_with_parameter(all_commands, 'E');
    for (int i = 0; i < min(5, (int)e_commands.size()); i++) {
        e_commands[i].print();
    }
    cout << endl;
    
    // 演示如何访问特定命令的参数
    cout << "=== 参数访问示例 ===" << endl;
    if (!g1_commands.empty()) {
        const auto& cmd = g1_commands[0];
        cout << "第一个G1命令的参数:" << endl;
        
        if (cmd.parameters.find('X') != cmd.parameters.end()) {
            cout << "  X坐标: " << cmd.parameters.at('X') << endl;
        }
        if (cmd.parameters.find('Y') != cmd.parameters.end()) {
            cout << "  Y坐标: " << cmd.parameters.at('Y') << endl;
        }
        if (cmd.parameters.find('Z') != cmd.parameters.end()) {
            cout << "  Z坐标: " << cmd.parameters.at('Z') << endl;
        }
        if (cmd.parameters.find('F') != cmd.parameters.end()) {
            cout << "  进给率: " << cmd.parameters.at('F') << endl;
        }
    }
    
    return 0;
}