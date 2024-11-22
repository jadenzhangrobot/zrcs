#ifndef OPCUASERVER
#define OPCUASERVER
#include "system/centre.h"
#include <open62541/server.h>
#include <open62541/server_pubsub.h>
#include <iostream>
#include <open62541/client_config_default.h>
#include <open62541/plugin/log_stdout.h>
#include <string>
namespace zrcsServer {
  typedef UA_StatusCode (*UA_MethodCallback)(UA_Server*, const UA_NodeId*, void*, const UA_NodeId*, void*, const UA_NodeId*, void*, size_t, const UA_Variant*, size_t, UA_Variant*);
class OpcuaServer
{
    UA_Server *server=nullptr;
    UA_Boolean running ;
   // static UA_NodeId ZrcsCmdNodeId ;
    public:
    OpcuaServer()
    {
        running=true;
    }
 void OpcuaInit()
 {
    
     server = UA_Server_new();

     // 创建一个对象节点
    //char NodeName[]="ZrcsCmdNodeId";
    // ZrcsCmdNodeId = UA_NODEID_STRING(1, "ZrcsCmdNodeId");
    UA_ObjectAttributes stuAttr = UA_ObjectAttributes_default;
    UA_Server_addObjectNode(server, UA_NODEID_STRING(1, "ZrcsCmdNodeId"),
                            UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                            UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
                            UA_QUALIFIEDNAME(1, "ZrcsCmdNodeId"), UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE),
                            stuAttr, NULL, NULL);  
    UA_VariableAttributes nameAttr = UA_VariableAttributes_default;
    UA_String studentName = UA_STRING("Xiao Ming");
    UA_Variant_setScalar(&nameAttr.value, &studentName, &UA_TYPES[UA_TYPES_STRING]);
    nameAttr.displayName = UA_LOCALIZEDTEXT("en-US", "Name");
    UA_Server_addVariableNode(server, UA_NODEID_NULL, UA_NODEID_STRING(1, "ZrcsCmdNodeId"),
                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                              UA_QUALIFIEDNAME(1, "StudentName"),
                              UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE), nameAttr, NULL, NULL);
   addHelloWorldMethod(server);


 }
 void updateCurrentTime(UA_Server * server, UA_NodeId timeNodeId)
{
	UA_DateTime now = UA_DateTime_now();
	UA_Variant value;
	UA_Variant_setScalar(&value, &now, &UA_TYPES[UA_TYPES_DATETIME]);
	UA_Server_writeValue(server, timeNodeId, value);
}



 void addHelloWorldMethod(UA_Server *server) {
    char language[]="en-US";
    UA_Argument inputArgument;
    UA_Argument_init(&inputArgument);
    char InputArgumentDescription[]="AString";
    inputArgument.description = UA_LOCALIZEDTEXT(language, InputArgumentDescription);
    char InputArgumentName[]="MyInput";
    inputArgument.name = UA_STRING(InputArgumentName);
    inputArgument.dataType = UA_TYPES[UA_TYPES_STRING].typeId;
    inputArgument.valueRank = UA_VALUERANK_SCALAR;

    UA_Argument outputArgument;
    UA_Argument_init(&outputArgument);

    char OutputArgumentDescription[]="AString";
    outputArgument.description = UA_LOCALIZEDTEXT(language,OutputArgumentDescription);
     char OutputArgumentName[]="MyOutput";
    outputArgument.name = UA_STRING(OutputArgumentName);
    outputArgument.dataType = UA_TYPES[UA_TYPES_STRING].typeId;
    outputArgument.valueRank = UA_VALUERANK_SCALAR;

    UA_MethodAttributes ZrcscmdAttr = UA_MethodAttributes_default;
      
    ZrcscmdAttr.description = UA_LOCALIZEDTEXT(language,"Say Hello World`");
    ZrcscmdAttr.displayName = UA_LOCALIZEDTEXT(language,"SendZrcsCmd");
    ZrcscmdAttr.executable = true;
    ZrcscmdAttr.userExecutable = true;
     
    
     UA_Server_addMethodNode(server, UA_NODEID_NUMERIC(1,62541),
                            UA_NODEID_STRING(1, "ZrcsCmdNodeId"),
                            UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                            UA_QUALIFIEDNAME(1, "ZrcsCmd"),
                            ZrcscmdAttr, SendCmdToQueue,
                            1, &inputArgument, 1, &outputArgument, NULL, NULL);
}



 static UA_StatusCode SendCmdToQueue(UA_Server *server,
                         			const UA_NodeId *sessionId, void *sessionHandle,
                         			const UA_NodeId *methodId, void *methodContext,
                         			const UA_NodeId *objectId, void *objectContext,
                         			size_t inputSize, const UA_Variant *input,
                         			size_t outputSize, UA_Variant *output,OpcuaServer* ops) 
                     {
                       
                            UA_String *inputStr = (UA_String*)input->data;
                           if (inputStr->length>0)
                            {                                  
                                  std::string cmd(reinterpret_cast<char*>(inputStr->data),inputStr->length);
                                  
                                   ops->Ct->cmd_queue.push(cmd);
                                   return UA_STATUSCODE_GOOD;   
                                                                                  
                            }
                           else {

                                  return UA_STATUSCODE_BAD;
                            }
                                                                 
                     }

  void OpcuaRun()
   {
     // UA_Server_runUntilInterrupt(server);
      UA_Server_run(server, &running);
   }
    /* Clean up */
    ~OpcuaServer()
    {
           running=false;
           UA_Server_delete(server);

    }
    
   


};
}
#endif