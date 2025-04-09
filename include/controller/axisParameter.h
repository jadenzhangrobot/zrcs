#ifndef PARAMETERREAD_H
#define PARAMETERREAD_H
#include <vector>
#include <string>
#include "system/xmlnode/xmlParsing.h"  
using namespace tinyxml2;
namespace ZrcsHardware 
{ 
      typedef  struct {
        std::string name; //轴名称  
        int id; //轴ID
        int slaveId; //从站ID
        double  pitch;   //螺距：每转动一圈的移动距离 
        bool   direction;  // 旋转方向：正向或负向 -->
        double ratio; //传动比    
        double backlash;//反向间隙
        double max_pos = 1.0; //正向软限位
        double min_pos = -1.0;//负向软限位
        double max_vel = 1.0;//最大速度
        double min_vel = -1.0; //最小速度
        double max_acc = 1.0;//最大加速度
        double min_acc = -1.0;//最小加速度             
        double max_pos_following_error = 1.0;//最大位置跟随误差
        double max_vel_following_error = 1.0;//最大速度跟随误差
        double pos_factor = 1.0;//编码器分辨率    
        double pos_offset = 0.0;//位置偏移
        double home_pos = 0.0;//原点位置
        double vel_factor = 1.0;//速度比例
        double target_vel_ = 0;//目标速度
        double target_toq_ = 0;//目标力矩
        double offset_vel_ = 0;//速度偏移
        double offset_toq_ = 0;//力矩偏移
    } AxisPara;
    class ParaConfig:public XmlParsing
    { 
            tinyxml2::XMLDocument axisDoc;
            public:
            std::vector<AxisPara> axisParas;         
       
            ParaConfig(const std::string& xmlFileName):XmlParsing(xmlFileName)
            {
                for (int i=0;tree->root->children.size();i++)
                {
                    if (tree->root->children[i]->data=="Axis")
                    {
                        AxisPara axisPara;
                        axisPara.name = tree->root->children[i]->attribute["name"];
                        axisPara.id = std::stoi(tree->root->children[i]->attribute["id"]);
                        axisPara.slaveId = std::stoi(tree->root->children[i]->attribute["slave"]);
                        axisPara.pitch = std::stod(tree->root->children[i]->attribute["Pitch"]);
                        axisPara.direction = std::stoi(tree->root->children[i]->attribute["Direction"]);
                        axisPara.ratio = std::stod(tree->root->children[i]->attribute["Ratio"]);
                        axisPara.backlash = std::stod(tree->root->children[i]->attribute["Backlash"]);
                        axisPara.max_pos = std::stod(tree->root->children[i]->attribute["max_pos"]);
                        axisPara.min_pos = std::stod(tree->root->children[i]->attribute["min_pos"]);
                        axisPara.max_vel = std::stod(tree->root->children[i]->attribute["max_vel"]);
                        axisPara.min_vel = std::stod(tree->root->children[i]->attribute["min_vel"]);
                        axisPara.max_acc = std::stod(tree->root->children[i]->attribute["max_acc"]);
                        axisPara.min_acc = std::stod(tree->root->children[i]->attribute["min_acc"]);
                        axisPara.max_pos_following_error = std::stod(tree->root->children[i]->attribute["max_pos_following_error"]);
                        axisPara.max_vel_following_error = std::stod(tree->root->children[i]->attribute["max_vel_following_error"]);
                        axisPara.pos_factor = std::stod(tree->root->children[i]->attribute["pos_factor"]);
                        axisPara.pos_offset = std::stod(tree->root->children[i]->attribute["pos_offset"]);
                        axisPara.home_pos = std::stod(tree->root->children[i]->attribute["home_pos"]);
                        axisPara.vel_factor = std::stod(tree->root->children[i]->attribute["vel_factor"]);
                        axisPara.target_vel_ = std::stod(tree->root->children[i]->attribute["target_vel_"]);
                        axisPara.target_toq_ = std::stod(tree->root->children[i]->attribute["target_toq_"]);
                        axisPara.offset_vel_ = std::stod(tree->root->children[i]->attribute["offset_vel_"]);
                        axisPara.offset_toq_ = std::stod(tree->root->children[i]->attribute["offset_toq_"]);
                        axisParas.push_back(axisPara);
                    }       
                }
                                 
            }
            ~ParaConfig()
            {
                for (int i=0;axisParas.size();i++)
                {
                    tree->root->children[i]->attribute["id"]=std::to_string(axisParas[i].id);
                    tree->root->children[i]->attribute["slave"]=std::to_string(axisParas[i].slaveId);
                    tree->root->children[i]->attribute["Pitch"]=std::to_string(axisParas[i].pitch);
                    tree->root->children[i]->attribute["Direction"]=std::to_string(axisParas[i].direction);
                    tree->root->children[i]->attribute["Ratio"]=std::to_string(axisParas[i].ratio);
                    tree->root->children[i]->attribute["Backlash"]=std::to_string(axisParas[i].backlash);
                    tree->root->children[i]->attribute["max_pos"]=std::to_string(axisParas[i].max_pos);
                    tree->root->children[i]->attribute["min_pos"]=std::to_string(axisParas[i].min_pos);
                    tree->root->children[i]->attribute["max_vel"]=std::to_string(axisParas[i].max_vel);
                    tree->root->children[i]->attribute["min_vel"]=std::to_string(axisParas[i].min_vel);
                    tree->root->children[i]->attribute["max_acc"]=std::to_string(axisParas[i].max_acc);
                    tree->root->children[i]->attribute["min_acc"]=std::to_string(axisParas[i].min_acc);
                    tree->root->children[i]->attribute["max_pos_following_error"]=std::to_string(axisParas[i].max_pos_following_error);
                    tree->root->children[i]->attribute["max_vel_following_error"]=std::to_string(axisParas[i].max_vel_following_error);
                    tree->root->children[i]->attribute["pos_factor"]=std::to_string(axisParas[i].pos_factor);
                    tree->root->children[i]->attribute["pos_offset"]=std::to_string(axisParas[i].pos_offset);
                    tree->root->children[i]->attribute["home_pos"]=std::to_string(axisParas[i].home_pos);
                    tree->root->children[i]->attribute["vel_factor"]=std::to_string(axisParas[i].vel_factor);
                    tree->root->children[i]->attribute["target_vel_"]=std::to_string(axisParas[i].target_vel_);
                    tree->root->children[i]->attribute["target_toq_"]=std::to_string(axisParas[i].target_toq_);
                    tree->root->children[i]->attribute["offset_vel_"]=std::to_string(axisParas[i].offset_vel_);
                    tree->root->children[i]->attribute["offset_toq_"]=std::to_string(axisParas[i].offset_toq_);
                    
                }
           }
    };
}
#endif