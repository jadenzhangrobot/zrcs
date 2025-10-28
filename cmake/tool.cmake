# 工具和脚本配置模块

# 查找 Python3
find_package(Python3 REQUIRED COMPONENTS Interpreter)

if(Python3_FOUND)
    message(STATUS "Found Python3: ${Python3_EXECUTABLE}")
else()
    message(FATAL_ERROR "Python3 is required but not found")
endif()

# 检查工具脚本是否存在
set(CONFIG_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/tool/config.py")
if(EXISTS ${CONFIG_SCRIPT})
    message(STATUS "Found config script: ${CONFIG_SCRIPT}")
    
    # 添加自定义命令执行 Python 脚本
    add_custom_target(run_python_scripts ALL
        COMMAND ${Python3_EXECUTABLE} ${CONFIG_SCRIPT}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Executing Python configuration script"
        VERBATIM
    )
    
    # 可以添加更多脚本
    # COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tool/another_script.py
    
else()
    message(WARNING "Config script not found: ${CONFIG_SCRIPT}")
    
    # 创建一个空的目标以避免构建错误
    add_custom_target(run_python_scripts ALL
        COMMAND ${CMAKE_COMMAND} -E echo "No Python scripts to run"
        COMMENT "Skipping Python scripts execution"
    )
endif()

# 添加其他工具配置
# 例如：代码生成、文档生成等
