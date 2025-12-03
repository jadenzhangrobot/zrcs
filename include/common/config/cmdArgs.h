#pragma once
enum  
{
   EnableAxisId,
};
enum 
{
   DisableAxisId,
};
enum 
{
   JogabsjAxisId,
   JogabsjTargetPosition,
};
enum 
{
   ResetAxisId,
};


  enum class TaskScheduling {
    RUN, 
    ERROR,
    STOP,            // 停止         
    RESET,           //
    START 
  };
