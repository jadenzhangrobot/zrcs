/**
 * @file main.cpp
 * @brief Non-Real-Time process entry point
 * @version 1.0
 * @date 2024
 */

#include <iostream>
#include <thread>
#include <chrono>

int main(int argc, char **argv) 
{
    std::cout << "ZRCS Non-Real-Time Process Started" << std::endl;   
    try 
    {
        // Non-real-time process logic here
        while(true)
        {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    } 
    catch (const std::exception& e) 
    {
        std::cerr << "Exception caught: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
