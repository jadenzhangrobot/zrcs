#include "common/xmlParsing.h"


int main()
{
      
    try {
        XmlParsing xmlParsing("ethercat.xml");
        std::string nodeName = "Axis";
        std::string str=xmlParsing.getRootNode()->children["slave-1"]->attribute["ID"];
         xmlParsing.getRootNode()->children["slave-1"]->attribute["ID"]=std::to_string(100);
      
      
    } catch (const std::runtime_error& e) {
        std::cerr << "Exception caught: " << e.what() << std::endl; 
    }
    return 0;
}