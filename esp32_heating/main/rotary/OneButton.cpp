#include "OneButton.h"
#include <algorithm>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define LOW 0
#define HIGH 1

/**
 * @brief Construct a new One Button:: One Button object
 *
 * @param pin 检测的按键IO
 * @param activeLow 检测的电平状态
 */
OneButton::OneButton(gpio_num_t pin, const bool activeLow)
{
    _pin = pin;

    if (activeLow) {
        _buttonPressed = LOW;
    } else {
        _buttonPressed = HIGH;
    }

    reset();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 * @brief 设置按键消抖时间
 *
 * @param ticks 30 / portTICK_PERIOD_MS
 */
void OneButton::setDebounceTicks(TickType_t ticks)
{
    _debounceTicks = ticks;
}

/**
 * @brief 设置单击检测时间
 * 建议这个时间要设置比 消抖时间长
 *
 * @param ticks 100 / portTICK_PERIOD_MS
 */
void OneButton::setClickTicks(TickType_t ticks)
{
    _clickTicks = ticks;
}

/**
 * @brief 设置长按检测时间
 *
 * @param ticks 300 / portTICK_PERIOD_MS
 */
void OneButton::setPressTicks(TickType_t ticks)
{
    _pressTicks = ticks;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * @brief 单击回调函数
 *
 * @param newFunction
 */
void OneButton::attachClick(callbackFunction newFunction)
{
    _clickFunc = newFunction;
} // attachClick

// save function for parameterized click event
void OneButton::attachClick(parameterizedCallbackFunction newFunction, uint32_t parameter)
{
    _paramClickFunc = newFunction;
    _clickFuncParam = parameter;
} // attachClick

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * @brief 双击回调函数
 *
 * @param newFunction
 */
void OneButton::attachDoubleClick(callbackFunction newFunction)
{
    _doubleClickFunc = newFunction;
    _maxClicks = std::max(_maxClicks, 2);
}

void OneButton::attachDoubleClick(parameterizedCallbackFunction newFunction, uint32_t parameter)
{
    _paramDoubleClickFunc = newFunction;
    _doubleClickFuncParam = parameter;
    _maxClicks = std::max(_maxClicks, 2);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * @brief 多击回调函数
 *
 * @param newFunction
 */
void OneButton::attachMultiClick(callbackFunction newFunction)
{
    _multiClickFunc = newFunction;
    _maxClicks = std::max(_maxClicks, 100);
}

void OneButton::attachMultiClick(parameterizedCallbackFunction newFunction, uint32_t parameter)
{
    _paramMultiClickFunc = newFunction;
    _multiClickFuncParam = parameter;
    _maxClicks = std::max(_maxClicks, 100);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 * @brief 长按回调函数
 *
 * @param newFunction
 */
void OneButton::attachLongPressStart(callbackFunction newFunction)
{
    _longPressStartFunc = newFunction;
}

void OneButton::attachLongPressStart(parameterizedCallbackFunction newFunction, uint32_t parameter)
{
    _paramLongPressStartFunc = newFunction;
    _longPressStartFuncParam = parameter;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * @brief 长按停止回调函数
 *
 * @param newFunction
 */

void OneButton::attachLongPressStop(callbackFunction newFunction)
{
    _longPressStopFunc = newFunction;
}

void OneButton::attachLongPressStop(parameterizedCallbackFunction newFunction, uint32_t parameter)
{
    _paramLongPressStopFunc = newFunction;
    _longPressStopFuncParam = parameter;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * @brief 多次长按回调函数
 *
 * @param newFunction
 */
void OneButton::attachDuringLongPress(callbackFunction newFunction)
{
    _duringLongPressFunc = newFunction;
}

void OneButton::attachDuringLongPress(parameterizedCallbackFunction newFunction, uint32_t parameter)
{
    _paramDuringLongPressFunc = newFunction;
    _duringLongPressFuncParam = parameter;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void OneButton::reset(void)
{
    _state = OneButton::OCS_INIT;
    _lastState = OneButton::OCS_INIT;
    _nClicks = 0;
    _startTime = 0;
}

int OneButton::getNumberClicks(void)
{
    return _nClicks;
}

/**
 * @brief 设置新的状态 保存旧的状态
 *
 * @param nextState
 */
void OneButton::_newState(stateMachine_t nextState)
{
    _lastState = _state;
    _state = nextState;
}

/**
 * @brief 状态机
 *
 */
void OneButton::tick(void)
{
    if (_pin != GPIO_NUM_NC) {
        tick(gpio_get_level(_pin) == _buttonPressed);
    }
}

void OneButton::tick(bool activeLevel)
{
    TickType_t now = xTaskGetTickCount();
    TickType_t waitTime = (now - _startTime);

    switch (_state) {
    case OneButton::OCS_INIT:
        // 等待按键按下阶段
        if (activeLevel) {
            _newState(OneButton::OCS_DOWN);
            _startTime = now;
            _nClicks = 0;
        }
        break;

    case OneButton::OCS_DOWN:
        // 按下阶段
        if ((!activeLevel) && (waitTime < _debounceTicks)) {
            // 退回上一个阶段
            _newState(_lastState);

        } else if (!activeLevel) {
            // 抬起阶段
            _newState(OneButton::OCS_UP);
            _startTime = now;

        } else if ((activeLevel) && (waitTime > _pressTicks)) {
            // 长按回调函数触发
            if (_longPressStartFunc)
                _longPressStartFunc();

            if (_paramLongPressStartFunc)
                _paramLongPressStartFunc(_longPressStartFuncParam);

            _newState(OneButton::OCS_PRESS);
        }
        break;

    case OneButton::OCS_UP:
        // 按键抬起
        if ((activeLevel) && (waitTime < _debounceTicks)) {
            _newState(_lastState);

        } else if (waitTime >= _debounceTicks) {
            _nClicks++;
            _newState(OneButton::OCS_COUNT);
        }
        break;

    case OneButton::OCS_COUNT:
        if (activeLevel) {
            // button is down again
            _newState(OneButton::OCS_DOWN);
            _startTime = now; // remember starting time

        } else if ((waitTime > _clickTicks) || (_nClicks == _maxClicks)) {
            // now we know how many clicks have been made.

            if (_nClicks == 1) {
                // 单击事件
                if (_clickFunc)
                    _clickFunc();

                if (_paramClickFunc)
                    _paramClickFunc(_clickFuncParam);

            } else if (_nClicks == 2) {
                // 双击事件
                if (_doubleClickFunc)
                    _doubleClickFunc();

                if (_paramDoubleClickFunc)
                    _paramDoubleClickFunc(_doubleClickFuncParam);

            } else {
                // 多击事件
                if (_multiClickFunc)
                    _multiClickFunc();
                if (_paramMultiClickFunc)
                    _paramMultiClickFunc(_multiClickFuncParam);
            }

            reset();
        }
        break;

    case OneButton::OCS_PRESS:
        // waiting for menu pin being release after long press.

        if (!activeLevel) {
            _newState(OneButton::OCS_PRESSEND);
            _startTime = now;

        } else {
            // still the button is pressed
            if (_duringLongPressFunc)
                _duringLongPressFunc();

            if (_paramDuringLongPressFunc)
                _paramDuringLongPressFunc(_duringLongPressFuncParam);
        }
        break;

    case OneButton::OCS_PRESSEND:
        // button was released.

        if ((activeLevel) && (waitTime < _debounceTicks)) {
            // button was released to quickly so I assume some bouncing.
            _newState(_lastState); // go back

        } else if (waitTime >= _debounceTicks) {
            if (_longPressStopFunc)
                _longPressStopFunc();

            if (_paramLongPressStopFunc)
                _paramLongPressStopFunc(_longPressStopFuncParam);
            reset();
        }
        break;

    default:
        _newState(OneButton::OCS_INIT);
        break;
    }
}