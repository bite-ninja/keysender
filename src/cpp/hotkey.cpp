#include <windows.h>
#include "hotkey.hpp"
#include "keyboard.hpp"
#include <thread>
#include <chrono>
#include <cmath>

Napi::Value Hotkey::buttonIsPressed(const Napi::CallbackInfo &info)
{
    return Napi::Boolean::New(info.Env(), GetKeyState(Keyboard::keysDef.at(std::string(info[0].As<Napi::String>()))) < 0);
}

void Hotkey::messagesGetter(TsfnContext *context)
{
    MSG msg = {0};
    const UINT keyCode = Keyboard::keysDef.at(context->key);
    RegisterHotKey(NULL, 0, NULL, keyCode);
    auto callback = [](Napi::Env env, Napi::Function jsCallback) {
        jsCallback.Call({});
    };
    int8_t prevState = 5;
    int8_t currState;
    while (context->exist)
    {
        if (PeekMessageA(&msg, NULL, 0, 0, PM_REMOVE) && msg.message == WM_HOTKEY && (currState = GetKeyState(keyCode)) != prevState)
        {
            prevState = currState;
            context->tsfn.BlockingCall(callback);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    UnregisterHotKey(NULL, 0);
    context->tsfn.Release();
    delete context;
}

void Hotkey::registerHotkey(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();
    if (info.Length() != 3 || !info[0].IsString() || !info[1].IsString() || !info[2].IsFunction())
    {
        Napi::Error::New(env, "Expected 3 arguments: String || Array, String, Function")
            .ThrowAsJavaScriptException();
        return;
    }
    auto curr = new TsfnContext;
    hotkeysRef.push_back(curr);
    curr->key = std::string(info[0].As<Napi::String>());
    curr->name = info[1].As<Napi::String>();
    curr->tsfn = Napi::ThreadSafeFunction::New(env, info[2].As<Napi::Function>(), "F", 0, 1);
    CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)messagesGetter, curr, NULL, NULL);
}

void Hotkey::unregisterHotkey(const Napi::CallbackInfo &info)
{
    if (info.Length() != 1 || !info[0].IsString())
    {
        Napi::Error::New(info.Env(), "Expected 1 argument: String")
            .ThrowAsJavaScriptException();
        return;
    }
    const std::string name(info[0].As<Napi::String>());
    for (uint8_t i = 0; i < hotkeysRef.size(); i++)
        if (hotkeysRef[i]->name == name)
        {
            hotkeysRef[i]->exist = false;
            hotkeysRef.erase(hotkeysRef.begin() + i);
            i--;
        }
    hotkeysRef.shrink_to_fit();
}

void Hotkey::unregisterAllHotkeys(const Napi::CallbackInfo &info)
{
    for (uint8_t i = 0; i < hotkeysRef.size(); i++)
        hotkeysRef[i]->exist = false;
    hotkeysRef.clear();
    hotkeysRef.shrink_to_fit();
}

Napi::Value Hotkey::findHotkeyName(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();
    if (info.Length() != 1 || !info[0].IsString())
    {
        Napi::Error::New(env, "Expected 1 argument: String")
            .ThrowAsJavaScriptException();
        return env.Undefined();
    }
    const std::string key(info[0].As<Napi::String>());
    for (uint8_t i = 0; i < hotkeysRef.size(); i++)
        if (hotkeysRef[i]->key == key)
            return Napi::String::New(env, hotkeysRef[i]->name);
    return env.Undefined();
}

Napi::FunctionReference Hotkey::constructor;

Napi::Object Hotkey::Init(Napi::Env env, Napi::Object exports)
{
    Napi::HandleScope scope(env);
    Napi::Function func = DefineClass(env, "_GlobalHotkey", {
                                                                StaticMethod("_register", &Hotkey::registerHotkey),
                                                                StaticMethod("unregister", &Hotkey::unregisterHotkey),
                                                                StaticMethod("unregisterAll", &Hotkey::unregisterAllHotkeys),
                                                                StaticMethod("findHotkeyName", &Hotkey::findHotkeyName),
                                                                StaticMethod("_buttonIsPressed", &Hotkey::buttonIsPressed),
                                                            });
    constructor = Napi::Persistent(func);
    constructor.SuppressDestruct();
    exports.Set("_GlobalHotkey", func);
    return exports;
}