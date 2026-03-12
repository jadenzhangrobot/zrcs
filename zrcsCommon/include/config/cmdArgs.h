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
   JogjAxisId,
   JogjTargetPosition,
};
enum 
{
   ResetAxisId,
};
enum
{
   MoveLX,
   MoveLY,
   MoveLZ,
   MoveLTargetPosition,
};
enum
{
   G01X,
   G01Y,
   G01Z,
   G01A,
   GO1B,
   GO1C,
   GO1F,
};

  enum class TaskScheduling 
  {
    RUN, 
    ERROR_STATE,
    STOP,            // 停止         
    RESET,           //
    START 
  };
