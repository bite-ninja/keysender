#pragma once
#ifndef TOUCH_H
#define TOUCH_H

#include "includes.hpp"

class Touch
{
public:
  Napi::Value toggleTap(const Napi::CallbackInfo &info);

private:
  BOOL sendTouchInput(Napi::Env *env, const POINTER_TOUCH_INFO *contact);
};

#endif