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
#include <iostream>
#include <map>

#define REGISTER(className)                      \
    className *objectCreator##className()        \
    {                                            \
        std::unique_ptr<className> ptr_className(new className);\
        return ptr_className.release();                    \
    }                                            \
    RegisterAction g_creatorRegister##className( \
        #className, (PTRCreateObject)objectCreator##className)

typedef void *(*PTRCreateObject)(void);



class classfactory
{
private:
    std::map<std::string, PTRCreateObject> m_classMap;
    classfactory(){};

public:
    void *getclassbyname(std::string classname)
    {
        std::map<std::string, PTRCreateObject>::const_iterator iter;
        iter = m_classMap.find(classname);
        if (iter == m_classMap.end())
            return NULL;
        else
            return iter->second();
    }
    void registClass(std::string name, PTRCreateObject method)
    {
        m_classMap.insert(std::pair<std::string, PTRCreateObject>(name, method));
    }
    static classfactory &getInstance()
    {
        static classfactory cla_fac;
        return cla_fac;
    }

     bool cmd_exist(std::string classname)
     {
           
      auto iter = m_classMap.find(classname);
        if (iter != m_classMap.end())
          {
              return true;
          } 
        else
         {
              return false;
         }

     }

};



//注册动作类
class RegisterAction
{
public:
    RegisterAction(std::string className, PTRCreateObject ptrCreateFn)
    {
        classfactory::getInstance().registClass(className, ptrCreateFn);
    }
};
#endif