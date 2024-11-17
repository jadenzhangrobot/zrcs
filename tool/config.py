# 要生成的.h文件的文件名
import xml.etree.ElementTree as slaveXml
filename = "RegisterConfig.h"
# 打开文件进行写入
with open(filename, 'w') as file:
    #将文件清空
    file.write('')
    # 写入一些声明或定义
    file.write("#ifndef EXAMPLE_H\n")
    file.write("#define EXAMPLE_H\n\n")
    file.write("// 一些声明或定义内容\n")
    file.write("int add(int a, int b);\n")
    file.write("#endif // EXAMPLE_H\n")
 
print(f"'{filename}' has been generated.")
