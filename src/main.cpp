/**
 * @copyrightCopyright(c)2024Glroadcorporation
 * @filename:main.cpp
 * @brief:
 * zhangyongjing@oetsky.com
 * @createdate:2024-01-08
 */
#include "command/Cmdhead.h"
#include "controller/rtos/xenomai.h"
#include "system/centre.h"
#include <unistd.h>
#include <open62541/server.h>
int main(int argc, char **argv) {


  zrcs_system::centre &ct = zrcs_system::centre::getInstance();
  ct.registerController<1, controller::EthercatMotor,controller::EthercatTransceive, controller::xenomai>();
  ct.init();
   UA_Server *server = UA_Server_new();

    /* Add a variable node to the server */
     char  locale[]="en-US";
     char  NodeName[]="the answer";
    /* 1) Define the variable attributes */
    UA_VariableAttributes attr = UA_VariableAttributes_default;
    attr.displayName = UA_LOCALIZEDTEXT(locale, NodeName);
    UA_Int32 myInteger = 42;
    UA_Variant_setScalar(&attr.value, &myInteger, &UA_TYPES[UA_TYPES_INT32]);

    /* 2) Define where the node shall be added with which browsename */
    UA_NodeId newNodeId = UA_NODEID_STRING(1, NodeName);
    UA_NodeId parentNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
    UA_NodeId parentReferenceNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES);
    UA_NodeId variableType = UA_NODEID_NULL; /* take the default variable type */
    UA_QualifiedName browseName = UA_QUALIFIEDNAME(1, NodeName);

    /* 3) Add the node */
    UA_Server_addVariableNode(server, newNodeId, parentNodeId,
                              parentReferenceNodeId, browseName,
                              variableType, attr, NULL, NULL);

    /* Run the server (until ctrl-c interrupt) */
    UA_StatusCode status = UA_Server_runUntilInterrupt(server);

    /* Clean up */
    UA_Server_delete(server);
    return status == UA_STATUSCODE_GOOD ? EXIT_SUCCESS : EXIT_FAILURE;

  while(1)
  {
    sleep(1);
  }
  return 0;
}
