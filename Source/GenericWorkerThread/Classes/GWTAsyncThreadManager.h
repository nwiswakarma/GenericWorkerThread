////////////////////////////////////////////////////////////////////////////////
//
// MIT License
// 
// Copyright (c) 2018-2019 Nuraga Wiswakarma
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//
////////////////////////////////////////////////////////////////////////////////
// 

#pragma once

#include "CoreMinimal.h"
#include "GWTAsyncThread.h"
#include "GWTAsyncThreadPool.h"
#include "GWTTaskWorker.h"

struct GENERICWORKERTHREAD_API FGWTAsyncThreadWeakInstance
{
    FPWGWTAsyncThread AsyncThreadPtr;
    int32 ThreadId;
    float RestTime = 0.f;

    FGWTAsyncThreadWeakInstance() = default;

    FGWTAsyncThreadWeakInstance(float InRestTime)
        : RestTime(InRestTime)
    {
    }

    FPSGWTAsyncThread Pin(class FGWTAsyncThreadManager& ThreadManager);
    bool IsValid() const;
};

struct GENERICWORKERTHREAD_API FGWTAsyncThreadPoolWeakInstance
{
    FPWGWTAsyncThreadPool AsyncThreadPoolPtr;
    int32 ThreadId;
    int32 ThreadCount = 1;

    FGWTAsyncThreadPoolWeakInstance() = default;

    FGWTAsyncThreadPoolWeakInstance(int32 InThreadCount)
        : ThreadCount(InThreadCount)
    {
    }

    FPSGWTAsyncThreadPool Pin(class FGWTAsyncThreadManager& ThreadManager);
    bool IsValid() const;
};

class GENERICWORKERTHREAD_API FGWTAsyncThreadManager
{
    struct FThreadRegister
    {
        TMap<int32, FPWGWTAsyncThread> InstanceMap;
        int32 UniqueID = 0;
    };

    struct FThreadPoolRegister
    {
        TMap<int32, FPWGWTAsyncThreadPool> InstanceMap;
        int32 UniqueID = 0;
    };

    FThreadRegister ThreadRegister;
    FThreadPoolRegister ThreadPoolRegister;

public:

    // Thread Functions

    FPSGWTAsyncThread CreateThread(float InRestTime, int32& OutInstanceId);
    FPSGWTAsyncThread CreateThread(float InRestTime);
    FPWGWTAsyncThread GetThread(int32 InstanceId) const;

    FORCEINLINE bool HasThread(int32 InstanceId) const
    {
        return ThreadRegister.InstanceMap.Contains(InstanceId);
    }

    // Thread Pool Functions

    FPSGWTAsyncThreadPool CreateThreadPool(int32 ThreadCount, int32& OutInstanceId);
    FPSGWTAsyncThreadPool CreateThreadPool(int32 ThreadCount);
    FPWGWTAsyncThreadPool GetThreadPool(int32 InstanceId) const;

    FORCEINLINE bool HasThreadPool(int32 InstanceId) const
    {
        return ThreadPoolRegister.InstanceMap.Contains(InstanceId);
    }
};
