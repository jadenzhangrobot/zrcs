#ifndef OPCUASERVER
#define OPCUASERVER
#include <cstddef>
#include <open62541/server.h>
#include <open62541/server_pubsub.h>
#include <iostream>
#include <open62541/client_config_default.h>
#include <open62541/plugin/log_stdout.h>
class OpcuaServer
{
    UA_Server *server=nullptr;
    UA_Boolean running ;
   // static UA_NodeId ZrcsCmdNodeId ;
    public:
    OpcuaServer()
    {
     running=true;
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
static UA_StatusCode
helloWorldMethodCallback(UA_Server *server,
                         const UA_NodeId *sessionId, void *sessionHandle,
                         const UA_NodeId *methodId, void *methodContext,
                         const UA_NodeId *objectId, void *objectContext,
                         size_t inputSize, const UA_Variant *input,
                         size_t outputSize, UA_Variant *output) {
    UA_String *inputStr = (UA_String*)input->data;
    UA_String tmp = UA_STRING_ALLOC("Hello ");
    if(inputStr->length > 0) {
        tmp.data = (UA_Byte *)UA_realloc(tmp.data, tmp.length + inputStr->length);
        memcpy(&tmp.data[tmp.length], inputStr->data, inputStr->length);
        tmp.length += inputStr->length;
    }
    UA_Variant_setScalarCopy(output, &tmp, &UA_TYPES[UA_TYPES_STRING]);
    UA_String_clear(&tmp);
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "Hello World was called");
    return UA_STATUSCODE_GOOD;
}

static void addHelloWorldMethod(UA_Server *server) {
    UA_Argument inputArgument;
    UA_Argument_init(&inputArgument);
    inputArgument.description = UA_LOCALIZEDTEXT("en-US", "A String");
    inputArgument.name = UA_STRING("MyInput");
    inputArgument.dataType = UA_TYPES[UA_TYPES_STRING].typeId;
    inputArgument.valueRank = UA_VALUERANK_SCALAR;

    UA_Argument outputArgument;
    UA_Argument_init(&outputArgument);
    outputArgument.description = UA_LOCALIZEDTEXT("en-US", "A String");
    outputArgument.name = UA_STRING("MyOutput");
    outputArgument.dataType = UA_TYPES[UA_TYPES_STRING].typeId;
    outputArgument.valueRank = UA_VALUERANK_SCALAR;

    UA_MethodAttributes helloAttr = UA_MethodAttributes_default;
    helloAttr.description = UA_LOCALIZEDTEXT("en-US","Say `Hello World`");
    helloAttr.displayName = UA_LOCALIZEDTEXT("en-US","Hello World");
    helloAttr.executable = true;
    helloAttr.userExecutable = true;
    UA_Server_addMethodNode(server, UA_NODEID_NUMERIC(1,62541),
                            UA_NODEID_STRING(1, "ZrcsCmdNodeId"),
                            UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                            UA_QUALIFIEDNAME(1, "hello world"),
                            helloAttr, &addOneMethodCallback,
                            1, &inputArgument, 1, &outputArgument, NULL, NULL);
}




   static  UA_StatusCode addOneMethodCallback (UA_Server *server,
                         			const UA_NodeId *sessionId, void *sessionHandle,
                         			const UA_NodeId *methodId, void *methodContext,
                         			const UA_NodeId *objectId, void *objectContext,
                         			size_t inputSize, const UA_Variant *input,
                         			size_t outputSize, UA_Variant *output) 
                    {

                        std::cout <<"--------------------"<<std::endl;
                        return UA_STATUSCODE_GOOD;
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
#endif