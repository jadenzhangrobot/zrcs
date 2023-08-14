/*
 * @Author: zhangyongjing
 * @email: 649894200@qq.com
 * @Date: 2023-03-15 11:30:25
 * @LastEditTime: 2023-06-06 09:37:31
 * @Description: 通过c++反射实现通过类名获取类指针
 *
 */
#ifndef CLALLFACTORY_H_
#define CLALLFACTORY_H_
#include "basenode.h"
#include <cstring>
#include <iostream>
#include <map>

#define REGISTER(className)\
std::unique_ptr<className> ptr_##className(new className);\
RegisterAction g_creatorRegister##className(#className,ptr_##className.release())

class classfactory {
private:
  std::map<std::string, zrcs_system::Basenode*> m_classMap;
  classfactory(){};

public:
    zrcs_system::Basenode* getclassbyname(std::string classname) {
    std::map<std::string, zrcs_system::Basenode*>::const_iterator iter;
    iter = m_classMap.find(classname);
    if (iter == m_classMap.end())
      return NULL;
    else
      return iter->second;
  }
  void registClass(std::string name, zrcs_system::Basenode* ptr_class) {
    m_classMap.insert(std::pair<std::string, zrcs_system::Basenode*>(name, ptr_class));
  }
  static classfactory &getInstance() {
    static classfactory cla_fac;
    return cla_fac;
  }

  bool cmd_exist(std::string classname) {

    auto iter = m_classMap.find(classname);
    if (iter != m_classMap.end()) {
      return true;
    } else {
      return false;
    }
  }
};

//注册动作类
class RegisterAction {
public:
  RegisterAction(std::string className, zrcs_system::Basenode* ptrCreateCl) {
    classfactory::getInstance().registClass(className, ptrCreateCl);
  }
};
#endif