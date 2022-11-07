#include <PIDv1.h>

/*Constructor (...)*********************************************************
 *    The parameters specified here are those for for which we can't set up
 *    reliable defaults, so we need to have the user set them.
 ***************************************************************************/
PID::PID(double* Input, double* Output, double* Setpoint, double Kp, double Ki, double Kd, int POn, int ControllerDirection)
{
    myOutput = Output;
    myInput = Input;
    mySetpoint = Setpoint;
    inAuto = false;

    this->SetOutputLimits(0, 255); // 设置默认的输出限制

    SampleTime = 100 / portTICK_PERIOD_MS; // 默认的采样频率为100ms

    this->SetControllerDirection(ControllerDirection);
    this->SetTunings(Kp, Ki, Kd, POn);

    this->reset();
    lastTime = xTaskGetTickCount();
}

/*Constructor (...)*********************************************************
 *    To allow backwards compatability for v1.1, or for people that just want
 *    to use Proportional on Error without explicitly saying so
 ***************************************************************************/
PID::PID(double* Input, double* Output, double* Setpoint, double Kp, double Ki, double Kd, int ControllerDirection)
    : PID::PID(Input, Output, Setpoint, Kp, Ki, Kd, P_ON_E, ControllerDirection)
{
}

PID::PID(double* Input, double* Output, double* Setpoint, int ControllerDirection)
    : PID::PID(Input, Output, Setpoint, 0, 0, 0, P_ON_E, ControllerDirection)
{
}

/**
 * @brief 复位所有参数
 *
 */
void PID::reset(void)
{
    outputSum = 0;
    lastInput = 0;
    lastTime = xTaskGetTickCount();
}

/* Compute() **********************************************************************
 *     This, as they say, is where the magic happens.  this function should be called
 *   every time "void loop()" executes.  the function will decide for itself whether a new
 *   pid Output needs to be computed.  returns true when the output is computed,
 *   false when nothing has been done.
 **********************************************************************************/
bool PID::Compute()
{
    if (!inAuto)
        return false;

    // 得到2次时间差
    TickType_t now = xTaskGetTickCount();
    TickType_t timeChange = (now - lastTime);

    if (timeChange >= SampleTime) {
        // 当前时间差 大于 采样时间差 开始计算

        /*Compute all the working error variables*/
        double input = *myInput; // PID输入值
        double error = *mySetpoint - input; // 差值 = 目标值 - 输入值
        double dInput = (input - lastInput); // PID输入值 - 之前的输入值

        // 计算I
        outputSum += (ki * error);

        /*Add Proportional on Measurement, if P_ON_M is specified*/
        if (!pOnE)
            outputSum -= kp * dInput;

        // 限制输出幅度
        if (outputSum > outMax)
            outputSum = outMax;
        else if (outputSum < outMin)
            outputSum = outMin;

        /*Add Proportional on Error, if P_ON_E is specified*/
        double output; // 输出结果

        // 计算P
        if (pOnE)
            output = kp * error;
        else
            output = 0;

        // 计算D
        output += outputSum - kd * dInput;

        // 限制输出幅度
        if (output > outMax) {
            output = outMax;
        } else if (output < outMin) {
            output = outMin;
        }

        // 当前输出结果
        *myOutput = output;

        /*Remember some variables for next time*/
        lastInput = input;
        lastTime = now;
        return true;

    } else {
        return false;
    }
}

/**
 * @brief 设置PID参数
 *
 * @param Kp
 * @param Ki
 * @param Kd
 * @param POn
 */
void PID::SetTunings(double Kp, double Ki, double Kd, int POn)
{
    if (Kp < 0 || Ki < 0 || Kd < 0)
        return;

    pOn = POn;
    pOnE = POn == P_ON_E;

    dispKp = Kp;
    dispKi = Ki;
    dispKd = Kd;

    double SampleTimeInSec = ((double)SampleTime) / 1000;
    kp = Kp;
    ki = Ki * SampleTimeInSec;
    kd = Kd / SampleTimeInSec;

    if (controllerDirection == REVERSE) {
        kp = (0 - kp);
        ki = (0 - ki);
        kd = (0 - kd);
    }
}

/* SetTunings(...)*************************************************************
 * Set Tunings using the last-rembered POn setting
 ******************************************************************************/
void PID::SetTunings(double Kp, double Ki, double Kd)
{
    SetTunings(Kp, Ki, Kd, pOn);
}

/* SetSampleTime(...) *********************************************************
 * sets the period, in Milliseconds, at which the calculation is performed
 ******************************************************************************/
void PID::SetSampleTime(int NewSampleTime)
{
    if (NewSampleTime > 0) {
        double ratio = (double)NewSampleTime / (double)SampleTime;
        ki *= ratio;
        kd /= ratio;
        SampleTime = (unsigned long)NewSampleTime;
    }
}

/* SetOutputLimits(...)****************************************************
 *     This function will be used far more often than SetInputLimits.  while
 *  the input to the controller will generally be in the 0-1023 range (which is
 *  the default already,)  the output will be a little different.  maybe they'll
 *  be doing a time window and will need 0-8000 or something.  or maybe they'll
 *  want to clamp it from 0-125.  who knows.  at any rate, that can all be done
 *  here.
 **************************************************************************/
void PID::SetOutputLimits(double Min, double Max)
{
    if (Min >= Max)
        return;

    outMin = Min;
    outMax = Max;

    if (inAuto) {
        if (*myOutput > outMax) {
            *myOutput = outMax;
        } else if (*myOutput < outMin) {
            *myOutput = outMin;
        }

        if (outputSum > outMax) {
            outputSum = outMax;
        } else if (outputSum < outMin) {
            outputSum = outMin;
        }
    }
}

/* SetMode(...)****************************************************************
 * Allows the controller Mode to be set to manual (0) or Automatic (non-zero)
 * when the transition from manual to auto occurs, the controller is
 * automatically initialized
 ******************************************************************************/
void PID::SetMode(int Mode)
{
    bool newAuto = (Mode == AUTOMATIC);
    if (newAuto && !inAuto) { /*we just went from manual to auto*/
        this->Initialize();
    }
    inAuto = newAuto;
}

/* Initialize()****************************************************************
 *	does all the things that need to happen to ensure a bumpless transfer
 *  from manual to automatic mode.
 ******************************************************************************/
void PID::Initialize()
{
    outputSum = *myOutput;
    lastInput = *myInput;
    if (outputSum > outMax)
        outputSum = outMax;
    else if (outputSum < outMin)
        outputSum = outMin;
}

/* SetControllerDirection(...)*************************************************
 * The PID will either be connected to a DIRECT acting process (+Output leads
 * to +Input) or a REVERSE acting process(+Output leads to -Input.)  we need to
 * know which one, because otherwise we may increase the output when we should
 * be decreasing.  This is called from the constructor.
 ******************************************************************************/
void PID::SetControllerDirection(int Direction)
{
    if (inAuto && Direction != controllerDirection) {
        kp = (0 - kp);
        ki = (0 - ki);
        kd = (0 - kd);
    }
    controllerDirection = Direction;
}

double PID::GetKp() { return dispKp; }
double PID::GetKi() { return dispKi; }
double PID::GetKd() { return dispKd; }

int PID::GetMode() { return inAuto ? AUTOMATIC : MANUAL; }
int PID::GetDirection() { return controllerDirection; }
