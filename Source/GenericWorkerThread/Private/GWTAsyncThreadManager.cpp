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

#include "GWTAsyncThreadManager.h"

FPSGWTAsyncThread FGWTAsyncThreadWeakInstance::Pin(class FGWTAsyncThreadManager& ThreadManager)
{
    if (! AsyncThreadPtr.IsValid())
    {
        FPSGWTAsyncThread NewAsyncThread(ThreadManager.CreateThread(FMath::Max(RestTime, 0.f), ThreadId));
        AsyncThreadPtr = NewAsyncThread;
        return MoveTemp( NewAsyncThread );
    }

    return AsyncThreadPtr.Pin();
}

FPSGWTAsyncThreadPool FGWTAsyncThreadPoolWeakInstance::Pin(class FGWTAsyncThreadManager& ThreadManager)
{
    if (! AsyncThreadPoolPtr.IsValid())
    {
        FPSGWTAsyncThreadPool NewAsyncThreadPool(ThreadManager.CreateThreadPool(FMath::Max(ThreadCount, 0), ThreadId));
        AsyncThreadPoolPtr = NewAsyncThreadPool;
        return MoveTemp( NewAsyncThreadPool );
    }

    return AsyncThreadPoolPtr.Pin();
}

// Thread Functions

FPSGWTAsyncThread FGWTAsyncThreadManager::CreateThread(float InRestTime, int32& OutInstanceId)
{
    int32 InstanceId = ThreadRegister.UniqueID++;
    FPSGWTAsyncThread AsyncThread( new FGWTAsyncThread(InRestTime) );
    ThreadRegister.InstanceMap.Emplace(InstanceId, AsyncThread);
    OutInstanceId = InstanceId;
    return MoveTemp( AsyncThread );
}

FPSGWTAsyncThread FGWTAsyncThreadManager::CreateThread(float InRestTime)
{
    int32 InstanceId;
    return CreateThread(InRestTime, InstanceId);
}

FPWGWTAsyncThread FGWTAsyncThreadManager::GetThread(int32 InstanceId) const
{
    return ThreadRegister.InstanceMap.Contains(InstanceId)
        ? ThreadRegister.InstanceMap.FindChecked(InstanceId)
        : FPWGWTAsyncThread();
}

// Thread Pool Functions

FPSGWTAsyncThreadPool FGWTAsyncThreadManager::CreateThreadPool(int32 ThreadCount, int32& OutInstanceId)
{
    int32 InstanceId = ThreadPoolRegister.UniqueID++;
    FPSGWTAsyncThreadPool AsyncThreadPool( new FGWTAsyncThreadPool(ThreadCount) );
    ThreadPoolRegister.InstanceMap.Emplace(InstanceId, AsyncThreadPool);
    OutInstanceId = InstanceId;
    return MoveTemp( AsyncThreadPool );
}

FPSGWTAsyncThreadPool FGWTAsyncThreadManager::CreateThreadPool(int32 ThreadCount)
{
    int32 InstanceId;
    return CreateThreadPool(ThreadCount, InstanceId);
}

FPWGWTAsyncThreadPool FGWTAsyncThreadManager::GetThreadPool(int32 InstanceId) const
{
    return ThreadPoolRegister.InstanceMap.Contains(InstanceId)
        ? ThreadPoolRegister.InstanceMap.FindChecked(InstanceId)
        : FPWGWTAsyncThreadPool();
}
