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
#include "Future.h"
#include "GWTAsyncTypes.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FGWTPromiseObject_OnPromiseDone);

template<typename ResultType>
using TGWTAsyncFuture  = TFuture<ResultType>;

template<typename ResultType>
struct TGWTAsyncFutureContainer
{
    typedef TGWTAsyncFuture<ResultType> FFutureType;
    FFutureType* const Future;

    TGWTAsyncFutureContainer()
        : Future(new FFutureType())
    {
    }

    TGWTAsyncFutureContainer(FFutureType&& InFuture)
        : Future(new FFutureType())
    {
        *Future = MoveTemp(InFuture);
    }

    ~TGWTAsyncFutureContainer()
    {
        delete Future;
    }
};

template<typename ResultType>
struct TGWTAsyncFutureList
{
    typedef TGWTAsyncFuture<ResultType> FFutureType;

    FFutureType Future;
    TGWTAsyncFutureList<ResultType>* Next;

    TGWTAsyncFutureList()
        : Next(nullptr)
    {
    }

    ~TGWTAsyncFutureList()
    {
        Reset();
    }

    FORCEINLINE bool IsValid() const
    {
        return Future.IsValid();
    }

    FORCEINLINE bool IsReady() const
    {
        return Future.IsReady();
    }

    FORCEINLINE bool IsDone() const
    {
        if (! IsValid() || IsReady())
        {
            return Next ? Next->IsDone() : true;
        }

        return false;
    }

    void Reset()
    {
        // Wait for current task future to complete
        Wait();

        // Delete next instance in the list
        if (Next)
        {
            delete Next;
            Next = nullptr;
        }

        // Invalidate future object
        if (Future.IsValid())
        {
            Future = FFutureType();
        }
    }

    TGWTAsyncFutureList<ResultType>* AllocateNext()
    {
        if (! Next)
        {
            Next = new TGWTAsyncFutureList<ResultType>();
        }

        return Next;
    }

    void Wait()
    {
        if (Future.IsValid() && ! Future.IsReady())
        {
            Future.Get();
        }

        if (Next)
        {
            Next->Wait();
        }
    }
};

typedef TGWTAsyncFutureList<void>                                    FGWTEventFuture;
typedef TKeyValuePair<TGWTAsyncFutureList<void>*, TFunction<void()>> FGWTEventTask;

typedef TArray<FGWTEventTask>       FGWTEventTaskList;
typedef TSharedPtr<FGWTEventFuture> FPSGWTEventFuture;

USTRUCT(BlueprintType)
struct GENERICWORKERTHREAD_API FGWTTaskEventRef
{
    GENERATED_BODY()

    FPSGWTEventFuture Future = nullptr;

    FGWTTaskEventRef() = default;

    FGWTTaskEventRef(const FPSGWTEventFuture& InFuture)
        : Future(InFuture)
    {
    }

    FORCEINLINE void Reset()
    {
        Future.Reset();
    }

    FORCEINLINE bool IsValid() const
    {
        return Future.IsValid();
    }

    FORCEINLINE bool IsDone() const
    {
        return Future.IsValid() ? Future->IsDone() : true;
    }

    FORCEINLINE void Wait()
    {
        if (IsValid())
        {
            Future->Wait();
            Future.Reset();
        }
    }
};

UCLASS(BlueprintType)
class GENERICWORKERTHREAD_API UGWTTaskEventObject : public UObject
{
	GENERATED_BODY()

public:

	UPROPERTY(BlueprintReadWrite)
    FGWTTaskEventRef EventRef;

    UFUNCTION(BlueprintCallable)
    void ResetEvent()
    {
        EventRef.Reset();
    }

    UFUNCTION(BlueprintCallable)
    bool IsEventValid() const
    {
        return EventRef.IsValid();
    }

    UFUNCTION(BlueprintCallable)
    void WaitEvent()
    {
        EventRef.Wait();
    }

    UFUNCTION(BlueprintCallable)
    bool IsEventDone() const
    {
        return EventRef.IsDone();
    }
};

UCLASS(BlueprintType)
class GENERICWORKERTHREAD_API UGWTPromiseObject : public UObject
{
	GENERATED_BODY()

public:

    typedef TFuture<void>        FFuture;
    typedef TFunction<void()>    FFutureCallback;
    typedef TPromise<void>       FPromise;
    typedef TSharedPtr<FPromise> FPSPromise;

	UPROPERTY(BlueprintAssignable)
    FGWTPromiseObject_OnPromiseDone OnPromiseDone;

    UFUNCTION(BlueprintCallable)
    bool IsFutureValid() const
    {
        return Future.IsValid();
    }

    UFUNCTION(BlueprintCallable)
    bool IsFutureReady() const
    {
        return Future.IsReady();
    }

    UFUNCTION(BlueprintCallable)
    void Wait()
    {
        if (IsFutureValid() && ! IsFutureReady())
        {
            Future.Wait();
        }
    }

    FORCEINLINE bool IsIdle() const
    {
        return !Future.IsValid() || Future.IsReady();
    }

    FPSPromise InitPromise(FFutureCallback CompletionCallback = FFutureCallback())
    {
        check(IsIdle());

        FutureCallback = MoveTemp(CompletionCallback);

        Promise = MakeShareable(
            new FPromise(
                [&]()
                {
                    ExecuteFutureCallback();
                    ResetPromise();
                } ) );

        Future = Promise->GetFuture();

        return Promise;
    }

    void SetPromise()
    {
        check(Promise.IsValid());
        check(!Future.IsReady());

        Promise->SetValue();
    }

    FORCEINLINE FPSPromise GetPromisePtr() const
    {
        return Promise;
    }

private:

	FPSPromise Promise;
    FFuture Future;
    FFutureCallback FutureCallback;

    void ResetPromise()
    {
        Promise.Reset();
    }

    void ExecuteFutureCallback()
    {
        if (FutureCallback)
        {
            FutureCallback();
        }

        OnPromiseDone.Broadcast();
    }
};
