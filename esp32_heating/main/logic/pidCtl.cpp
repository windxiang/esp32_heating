#include "heating.h"
#include "PIDv1.h"

static const char* TAG = "PIDCTL";

struct _pidParm {
    double pidCurTemp; // PID输入值 (当前温度)
    double pidCurOutput; // PID输出值 要输出PWM宽度
    double pidTargetTemp; // PID目标值 (设定温度值)
};

static _pidParm pidParm = {
    pidCurTemp : 0,
    pidCurOutput : 0,
    pidTargetTemp : 0,
};

static PID MyPID(&pidParm.pidCurTemp, &pidParm.pidCurOutput, &pidParm.pidTargetTemp, DIRECT);

/**
 * @brief 初始化PID
 *
 */
void startPID(void)
{
    _HeatingConfig* pCurConfig = getCurrentHeatingConfig();

    MyPID.SetOutputLimits(0, 100); // PID输出限幅
    MyPID.SetMode(AUTOMATIC); // PID控制模式
    MyPID.SetSampleTime(pCurConfig->PIDSample); // 设置采样时间
    MyPID.reset(); // 复位参数
}

/**
 * @brief 得到当前PID输出值
 *
 * @return double
 */
double getPIDOutput(void)
{
    return pidParm.pidCurOutput;
}

/**
 * @brief 处理PID逻辑 和 PWM输出
 *
 * @return uint16_t
 */
static void startPIDLogic(void)
{
    _HeatingConfig* pCurConfig = getCurrentHeatingConfig();

    // 当前温度
    pidParm.pidCurTemp = adcGetHeatingTemp();

    // PID参数设定
    if (pidParm.pidCurTemp < pCurConfig->PIDTemp) {
        // 远PID
        MyPID.SetTunings(pCurConfig->PID[0][0], pCurConfig->PID[0][1], pCurConfig->PID[0][2]);
    } else {
        // 近PID
        MyPID.SetTunings(pCurConfig->PID[1][0], pCurConfig->PID[1][1], pCurConfig->PID[1][2]);
    }

    // 计算输出目标值
    if (TYPE_HEATING_CONSTANT == pCurConfig->type) {
        // 恒温焊台模式
        pidParm.pidTargetTemp = pCurConfig->targetTemp;

    } else if (TYPE_HEATING_VARIABLE == pCurConfig->type) {
        // 回流焊模式
        pidParm.pidTargetTemp = CalculateTemp((xTaskGetTickCount() - getStartOutputTick()) / 1000.0f, pCurConfig->PTemp, NULL);

    } else if (TYPE_T12 == pCurConfig->type) {
        // T12
        pidParm.pidTargetTemp = pCurConfig->targetTemp;
    }

    // PID采样周期
    MyPID.SetSampleTime(pCurConfig->PIDSample);

    // 计算PID输出
    MyPID.Compute();

    // 输出
    if (TYPE_HEATING_CONSTANT == pCurConfig->type || TYPE_HEATING_VARIABLE == pCurConfig->type) {
        pwmOutput(_TYPE_HEAT, pidParm.pidCurOutput);

    } else if (TYPE_T12 == pCurConfig->type) {
        pwmOutput(_TYPE_T12, pidParm.pidCurOutput);
    }

    ESP_LOGI(TAG, "%1.1f/%1.1f 输出:%1.1f PID:%1.1f %1.1f %1.1f\n", pidParm.pidCurTemp, pidParm.pidTargetTemp, pidParm.pidCurOutput, MyPID.GetKp(), MyPID.GetKi(), MyPID.GetKd());
}

/**
 * @brief 温度控制和计算输出 对外接口
 *
 */
void TempCtrlLoop(void)
{
    if (getPIDIsStartOutput()) {
        startPIDLogic();

    } else {
        // PID停止输出
        pwmOutput(_TYPE_HEAT, 0);
        pwmOutput(_TYPE_T12, 0);
    }
}
