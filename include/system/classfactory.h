
#ifndef CLALLFACTORY_H_
#define CLALLFACTORY_H_
#include <map>
#include <string>
#include <any>
#include "basenodeInterface.h"

typedef zrcsSystem::Basenode* (*CreateNode)(void);
namespace zrcsSystem {
        class classfactory
        {
        private:
            std::map<std::string, std::any> m_classMap;
            classfactory(){};
            classfactory(const classfactory&)=delete;
            classfactory(const classfactory&&)=delete;
            classfactory& operator=(const classfactory&)=delete;
        public:
            std::any getClassByName(std::string classname)
            {
                std::map<std::string,std::any>::const_iterator iter;
                iter = m_classMap.find(classname);
                if (iter == m_classMap.end())
                    return NULL;
                else
                    return iter->second;
            }
            void registClass(std::string name, std::any method)
            {
                m_classMap.insert(std::pair<std::string, std::any>(name, method));
            }
            static classfactory &getInstance()
            {
                static classfactory cla_fac;
                return cla_fac;
            }

            bool cmdExist(const std::string& classname)
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
        class RegisterClass
        { 
            public:            
                RegisterClass(std::string className, std::any ptr)
                {                    
                    zrcsSystem::classfactory::getInstance().registClass(className,ptr);                             
                }
        };
}


 #define REGISTERCMD(className)                     \
 zrcsSystem::Basenode* objectCreator##className() \
    {                                                 \
        zrcsSystem::Basenode* ptr= static_cast<zrcsSystem::Basenode*>(new className());\
        return std::unique_ptr<zrcsSystem::Basenode>(ptr).release();\
    } \
    zrcsSystem::RegisterClass RegisterClass##className(#className,objectCreator##className())



#define REGISTERNODE(className)                     \
zrcsSystem::Basenode* objectCreator##className()\
    {                                                 \
        zrcsSystem::Basenode* ptr= static_cast<zrcsSystem::Basenode*>(new className());\
        return std::unique_ptr<zrcsSystem::Basenode>(ptr).release();\
    } \
    zrcsSystem::RegisterClass RegisterClass##className(#className,(CreateNode)objectCreator##className)



#endif