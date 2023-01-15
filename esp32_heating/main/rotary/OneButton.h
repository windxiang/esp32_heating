#pragma once

#include "driver/gpio.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

typedef void (*callbackFunction)(void);
typedef void (*parameterizedCallbackFunction)(uint32_t);

#ifdef __cplusplus
}
#endif // __cplusplus

class OneButton {
public:
    OneButton(gpio_num_t pin, const bool activeLow = true);

    void setDebounceTicks(TickType_t ticks);
    void setClickTicks(TickType_t ticks);
    void setPressTicks(TickType_t ticks);

    /**
     * Attach an event to be called when a single click is detected.
     * @param newFunction This function will be called when the event has been detected.
     */
    void attachClick(callbackFunction newFunction);
    void attachClick(parameterizedCallbackFunction newFunction, uint32_t parameter);

    /**
     * Attach an event to be called after a double click is detected.
     * @param newFunction This function will be called when the event has been detected.
     */
    void attachDoubleClick(callbackFunction newFunction);
    void attachDoubleClick(parameterizedCallbackFunction newFunction, uint32_t parameter);

    /**
     * Attach an event to be called after a multi click is detected.
     * @param newFunction This function will be called when the event has been detected.
     */
    void attachMultiClick(callbackFunction newFunction);
    void attachMultiClick(parameterizedCallbackFunction newFunction, uint32_t parameter);

    /**
     * Attach an event to fire when the button is pressed and held down.
     * @param newFunction
     */
    void attachLongPressStart(callbackFunction newFunction);
    void attachLongPressStart(parameterizedCallbackFunction newFunction, uint32_t parameter);

    /**
     * Attach an event to fire as soon as the button is released after a long press.
     * @param newFunction
     */
    void attachLongPressStop(callbackFunction newFunction);
    void attachLongPressStop(parameterizedCallbackFunction newFunction, uint32_t parameter);

    /**
     * Attach an event to fire periodically while the button is held down.
     * @param newFunction
     */
    void attachDuringLongPress(callbackFunction newFunction);
    void attachDuringLongPress(parameterizedCallbackFunction newFunction, uint32_t parameter);

    // ----- State machine functions -----

    /**
     * @brief Call this function every some milliseconds for checking the input
     * level at the initialized digital pin.
     */
    void tick(void);

    /**
     * @brief Call this function every time the input level has changed.
     * Using this function no digital input pin is checked because the current
     * level is given by the parameter.
     */
    void tick(bool level);

    /**
     * Reset the button state machine.
     */
    void reset(void);

    /*
     * return number of clicks in any case: single or multiple clicks
     */
    int getNumberClicks(void);

    /**
     * @return true if we are currently handling button press flow
     * (This allows power sensitive applications to know when it is safe to power down the main CPU)
     */
    bool isIdle() const { return _state == OCS_INIT; }

    /**
     * @return true when a long press is detected
     */
    bool isLongPressed() const { return _state == OCS_PRESS; };

private:
    gpio_num_t _pin; // 硬件GPIO号
    TickType_t _debounceTicks = 30 / portTICK_PERIOD_MS; // 消抖时间
    TickType_t _clickTicks = 100 / portTICK_PERIOD_MS; // 单击检测时间
    TickType_t _pressTicks = 300 / portTICK_PERIOD_MS; // 长按检测时间

    int _buttonPressed;

    // These variables will hold functions acting as event source.
    callbackFunction _clickFunc = NULL;
    parameterizedCallbackFunction _paramClickFunc = NULL;
    uint32_t _clickFuncParam = 0;

    callbackFunction _doubleClickFunc = NULL;
    parameterizedCallbackFunction _paramDoubleClickFunc = NULL;
    uint32_t _doubleClickFuncParam = 0;

    callbackFunction _multiClickFunc = NULL;
    parameterizedCallbackFunction _paramMultiClickFunc = NULL;
    uint32_t _multiClickFuncParam = 0;

    callbackFunction _longPressStartFunc = NULL;
    parameterizedCallbackFunction _paramLongPressStartFunc = NULL;
    uint32_t _longPressStartFuncParam = 0;

    callbackFunction _longPressStopFunc = NULL;
    parameterizedCallbackFunction _paramLongPressStopFunc = NULL;
    uint32_t _longPressStopFuncParam = 0;

    callbackFunction _duringLongPressFunc = NULL;
    parameterizedCallbackFunction _paramDuringLongPressFunc = NULL;
    uint32_t _duringLongPressFuncParam = 0;

    // These variables that hold information across the upcoming tick calls.
    // They are initialized once on program start and are updated every time the
    // tick function is called.

    // define FiniteStateMachine
    enum stateMachine_t : int {
        OCS_INIT = 0, // 等待按键按下
        OCS_DOWN = 1, // 按键按下了
        OCS_UP = 2,
        OCS_COUNT = 3,
        OCS_PRESS = 6,
        OCS_PRESSEND = 7,
        UNKNOWN = 99
    };

    /**
     *  Advance to a new state and save the last one to come back in cas of bouncing detection.
     */
    void _newState(stateMachine_t nextState);

    stateMachine_t _state = OCS_INIT;
    stateMachine_t _lastState = OCS_INIT; // used for debouncing

    TickType_t _startTime; // start of current input change to checking debouncing
    int _nClicks; // count the number of clicks with this variable
    int _maxClicks = 1; // max number (1, 2, multi=3) of clicks of interest by registration of event functions.
};
