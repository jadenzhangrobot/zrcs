#include "xml/xmlParsing.h"
#include "config/parameter.h"
#include <filesystem>


int main()
{
    try {
        XmlParsing xmlParsing("axisConfig.xml");
        std::string nodeName = "Axis";
        std::string str=xmlParsing.getRootNode()->children["axis-1"]->attribute["ID"];
         xmlParsing.getRootNode()->children["slave-1"]->attribute["ID"]=std::to_string(100);   
    } catch (const std::runtime_error& e) {
        std::cerr << "Exception caught: " << e.what() << std::endl; 
    }

    // try {
    //     XmlParsing xmlParsing("axisConfig.xml");
    //     //std::string nodeName = "Axis";
    //     std::string str=xmlParsing.getRootNode()->children["axis-1"]->attribute["ID"];
    //     //  xmlParsing.getRootNode()->children["slave-1"]->attribute["ID"]=std::to_string(100);
      
      
    // } catch (const std::runtime_error& e) {
    //     std::cerr << "Exception caught: " << e.what() << std::endl; 
    // }


    return 0;
}