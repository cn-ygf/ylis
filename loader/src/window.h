#pragma once
class Window {
public:
    virtual void PostUpdateProgressTask(int value) = 0;
    virtual void PostError() = 0;
    virtual void PostInstallSuccess() = 0;
};