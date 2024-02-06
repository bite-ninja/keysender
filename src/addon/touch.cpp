#include "touch.hpp"

#include "helper.hpp"

#define STATUS_ACCESS_DENIED ((NTSTATUS)0xC0000022L)

BOOL Touch::sendTouchInput(Napi::Env *env, const POINTER_TOUCH_INFO *touchInfo)
{
    if (!::InjectTouchInput(1, touchInfo))
    {
        const auto error = GetLastError();
        switch (error)
        {
        case ERROR_INVALID_PARAMETER:
        {
            Napi::TypeError::New(*env, "InjectTouchInput() => ERROR_INVALID_PARAMETER").ThrowAsJavaScriptException();
            break;
        }
        case STATUS_ACCESS_DENIED:
        {
            Napi::TypeError::New(*env, "InjectTouchInput() => STATUS_ACCESS_DENIED").ThrowAsJavaScriptException();
            break;
        }
        case ERROR_TIMEOUT:
        {
            Napi::TypeError::New(*env, "InjectTouchInput() => ERROR_TIMEOUT").ThrowAsJavaScriptException();
            break;
        }
        case ERROR_NOT_READY:
        {
            Napi::TypeError::New(*env, "InjectTouchInput() => ERROR_NOT_READY").ThrowAsJavaScriptException();
            break;
        }
        }
        return false;
    }
    return true;
}

Napi::Value Touch::toggleTap(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();
    std::cout << "Expected 3 arguments (x: number, y: number, touchDown: boolean). Received " + std::to_string(info.Length()) + " arguments. Details:" << std::endl;

    if (info.Length() != 3)
    {
        std::cout << " Incorrect number of arguments." << std::endl;
    }

    for (size_t i = 0; i < info.Length(); ++i)
    {
        std::cout << " Arg " + std::to_string(i + 1) + ":" << std::endl;
        if (info[i].IsNumber())
        {
            double numberValue = info[i].As<Napi::Number>().DoubleValue();
            std::cout << " Number(" + std::to_string(numberValue) + ")" << std::endl;
        }
        else if (info[i].IsBoolean())
        {
            bool boolValue = info[i].As<Napi::Boolean>().Value();
            std::cout << " Boolean(" + std::string(boolValue ? "true" : "false") + ")" << std::endl;
        }
        else
        {
            // Simplified type reporting
            std::cout << " of incompatible type." << std::endl;
        }
    }

    // Check if the arguments match the expected types.
    if (!info[0].IsNumber())
    {
        std::cout << " - First argument should be a number." << std::endl;
    }
    if (!info[1].IsNumber())
    {
        std::cout << " - Second argument should be a number." << std::endl;
    }
    if (!info[2].IsBoolean())
    {
        std::cout << " - Third argument should be a boolean." << std::endl;
    }

    if (info.Length() != 3 || !info[0].IsNumber() || !info[1].IsNumber() || !info[2].IsBoolean())
    {
        Napi::TypeError::New(env, "Invalid bla arguments. Expected (x: number, y: number, touchDown: boolean)").ThrowAsJavaScriptException();
        return Napi::Boolean::New(env, false);
    }

    int x = info[0].As<Napi::Number>().Int32Value();
    int y = info[1].As<Napi::Number>().Int32Value();
    bool touchDown = info[2].As<Napi::Boolean>().Value();

    POINTER_TOUCH_INFO touchInfo;
    memset(&touchInfo, 0, sizeof(POINTER_TOUCH_INFO));

    // POINTER_INFO part
    touchInfo.pointerInfo.pointerType = PT_TOUCH;
    touchInfo.pointerInfo.pointerId = 0; // You may need to manage the pointer IDs if you have multiple touches.
    touchInfo.pointerInfo.ptPixelLocation.x = x;
    touchInfo.pointerInfo.ptPixelLocation.y = y;

    // POINTER_TOUCH_INFO part
    touchInfo.touchFlags = TOUCH_FLAG_NONE; // The default value.
    touchInfo.touchMask = TOUCH_MASK_NONE;  // Default: None of the optional fields are valid.

    // Set touch contact state
    BOOL result = true;

    // INRANGE | UPDATE	            Touch hover starts or moves
    // INRANGE | INCONTACT | DOWN	Touch contact down
    // INRANGE | INCONTACT | UPDATE	Touch contact moves
    // INRANGE | UP	                Touch contact up and transition to hover
    // UPDATE	                    Touch hover ends
    // UP	                        Touch ends
    if (touchDown)
    {
        std::cout << "Injecting Tap Start POINTER_FLAG_INRANGE | POINTER_FLAG_UPDATE" << std::endl;
        touchInfo.pointerInfo.pointerFlags = POINTER_FLAG_INRANGE | POINTER_FLAG_UPDATE;
        result = result && sendTouchInput(&env, &touchInfo);
        std::cout << "Injecting Tap Down.  POINTER_FLAG_INRANGE | POINTER_FLAG_INCONTACT | POINTER_FLAG_DOWN" << std::endl;
        touchInfo.pointerInfo.pointerFlags = POINTER_FLAG_INRANGE | POINTER_FLAG_INCONTACT | POINTER_FLAG_DOWN;
        result = result && sendTouchInput(&env, &touchInfo);
    }
    else
    {
        std::cout << "Injecting Tap up. POINTER_FLAG_INRANGE | POINTER_FLAG_UP" << std::endl;
        touchInfo.pointerInfo.pointerFlags = POINTER_FLAG_INRANGE | POINTER_FLAG_UP;
        result = result && sendTouchInput(&env, &touchInfo);
        std::cout << "Injecting Tap end. POINTER_FLAG_UPDATE" << std::endl;
        touchInfo.pointerInfo.pointerFlags = POINTER_FLAG_UPDATE;
        result = result && sendTouchInput(&env, &touchInfo);
    }

    // Simulate a touch input event
    std::cout << "Inject completed." << std::endl;
    return Napi::Boolean::New(env, result != 0);
}
