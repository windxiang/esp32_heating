#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// Constants used in some of the functions below
#define AUTOMATIC 1
#define MANUAL 0
#define DIRECT 0
#define REVERSE 1
#define P_ON_M 0
#define P_ON_E 1

class PID {
public:
    // commonly used functions **************************************************************************
    PID(double*, double*, double*, // * constructor.  links the PID to the Input, Output, and
        double, double, double, int, int); //   Setpoint.  Initial tuning parameters are also set here.
                                           //   (overload for specifying proportional mode)

    PID(double*, double*, double*, // * constructor.  links the PID to the Input, Output, and
        double, double, double, int); //   Setpoint.  Initial tuning parameters are also set here

    PID(double*, double*, double*, int);

    void reset(void); // 复位所有参数

    void SetMode(int Mode); // * sets PID to either Manual (0) or Auto (non-0)

    bool Compute(); // * performs the PID calculation.  it should be
                    //   called every time loop() cycles. ON/OFF and
                    //   calculation frequency can be set using SetMode
                    //   SetSampleTime respectively

    void SetOutputLimits(double, double); // * clamps the output to a specific range. 0-255 by default, but
                                          //   it's likely the user will want to change this depending on
                                          //   the application

    // available but not commonly used functions ********************************************************
    void SetTunings(double, double, // * While most users will set the tunings once in the
        double); //   constructor, this function gives the user the option
                 //   of changing tunings during runtime for Adaptive control
    void SetTunings(double, double, // * overload for specifying proportional mode
        double, int);

    void SetControllerDirection(int); // * Sets the Direction, or "Action" of the controller. DIRECT
                                      //   means the output will increase when error is positive. REVERSE
                                      //   means the opposite.  it's very unlikely that this will be needed
                                      //   once it is set in the constructor.
    void SetSampleTime(int); // * sets the frequency, in Milliseconds, with which
                             //   the PID calculation is performed.  default is 100

    // Display functions ****************************************************************
    double GetKp(); // These functions query the pid for interal values.
    double GetKi(); //  they were created mainly for the pid front-end,
    double GetKd(); // where it's important to know what is actually
    int GetMode(); //  inside the PID.
    int GetDirection(); //

private:
    void Initialize();

    double dispKp; // 原始PID设置参数 不会改变
    double dispKi; // 原始PID设置参数
    double dispKd; // 原始PID设置参数

    double kp; // 计算后的PID参数 根据原始PID参数做计算
    double ki; // 计算后的PID参数
    double kd; // 计算后的PID参数

    int controllerDirection;
    int pOn;

    double* myInput; // PID输入值 (当前传感器值)
    double* myOutput; // PID输出值 (计算结果)
    double* mySetpoint; // PID目标值 (输入值要向目标值靠拢)

    TickType_t lastTime;
    double outputSum, lastInput;

    TickType_t SampleTime; // PID采样时间 小于采样时间不进行计算
    double outMin, outMax;
    bool inAuto, pOnE;
};
