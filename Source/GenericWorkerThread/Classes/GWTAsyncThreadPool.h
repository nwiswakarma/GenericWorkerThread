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
#include "Async.h"
#include "GWTAsyncTypes.h"
#include "GWTAsyncThreadPool.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FGWTAsyncTaskObject_OnTaskDone);

typedef TSharedRef<class FGWTAsyncThreadPool> FPRGWTAsyncThreadPool;
typedef TSharedPtr<class FGWTAsyncThreadPool> FPSGWTAsyncThreadPool;
typedef TWeakPtr<class FGWTAsyncThreadPool>   FPWGWTAsyncThreadPool;

class FGWTAsyncThreadPool
{
    FQueuedThreadPool* const ThreadPool;
    bool bThreadPoolCreated;

public:

    FGWTAsyncThreadPool()
        : ThreadPool(FQueuedThreadPool::Allocate())
        , bThreadPoolCreated(false)
    {
    }

    FGWTAsyncThreadPool(int32 InThreadCount)
        : ThreadPool(FQueuedThreadPool::Allocate())
        , bThreadPoolCreated(false)
    {
        SetThreadInstanceCount(InThreadCount);
    }

    ~FGWTAsyncThreadPool()
    {
        ThreadPool->Destroy();
        delete ThreadPool;
    }

    void SetThreadInstanceCount(int32 InThreadCount)
    {
        if (bThreadPoolCreated)
        {
            ThreadPool->Destroy();
        }

        bThreadPoolCreated = ThreadPool->Create(InThreadCount, 32 * 1024);
    }

    template<typename ResultType>
    TFuture<ResultType> AddQueuedWork(TFunction<ResultType()> Function, TFunction<void()> CompletionCallback = TFunction<void()>())
    {
        if (bThreadPoolCreated)
        {
            TPromise<ResultType> Promise(MoveTemp(CompletionCallback));
            TFuture<ResultType> Future = Promise.GetFuture();

            ThreadPool->AddQueuedWork(new TAsyncQueuedWork<ResultType>(MoveTemp(Function), MoveTemp(Promise)));

            return MoveTemp(Future);
        }

        return TFuture<ResultType>();
    }

    FORCEINLINE TFuture<void> AddQueuedWork(TFunction<void()> Function, TFunction<void()> CompletionCallback = TFunction<void()>())
    {
        return AddQueuedWork<void>(Function, CompletionCallback);
    }

    void AddQueuedEventChain(
        const TArray<FGWTEventTask>& EventTasks,
        FGWTEventFuture* WaitList = nullptr,
        TFunction<void()> CompletionCallback = TFunction<void()>()
        )
    {
        // Wait for events in the wait list
        if (WaitList)
        {
            WaitList->Wait();
        }

        // Empty task, immediate return
        if (EventTasks.Num() <= 0)
        {
            // Call completion callback if required
            if (CompletionCallback)
            {
                CompletionCallback();
            }

            return;
        }

        // Add tasks to thread pool

        // Add queued tasks with completion callback
        if (CompletionCallback)
        {
            TSharedRef<FThreadSafeCounter> TaskCounter( new FThreadSafeCounter(EventTasks.Num()) );
            TFunction<void()> Callback(
                [TaskCounter, CompletionCallback]()
                {
                    int32 CurrentTaskCount = TaskCounter->Decrement();

                    if (CurrentTaskCount == 0)
                    {
                        CompletionCallback();
                    }
                } );

            for (const FGWTEventTask& EventTask : EventTasks)
            {
                EventTask.Key->Future = AddQueuedWork(EventTask.Value, Callback);
            }
        }
        else
        {
            for (const FGWTEventTask& EventTask : EventTasks)
            {
                EventTask.Key->Future = AddQueuedWork(EventTask.Value);
            }
        }
    }
};

typedef TSharedPtr<struct FGWTAsyncTask> FPSGWTAsyncTask;

struct GENERICWORKERTHREAD_API FGWTAsyncTask
{
    FGWTAsyncThreadPool* ThreadPool = nullptr;
    FPSGWTEventFuture Future        = nullptr;
    FGWTEventTaskList TaskList;

    FGWTAsyncTask() = default;

    FGWTAsyncTask(const FPSGWTEventFuture& InFuture, FGWTAsyncThreadPool& InThreadPool)
        : Future(InFuture)
        , ThreadPool(&InThreadPool)
    {
    }

    FORCEINLINE void Reset()
    {
        Future.Reset();
        TaskList.Empty();
        ThreadPool = nullptr;
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

    FORCEINLINE FGWTEventTaskList& GetTaskList()
    {
        return TaskList;
    }

    FORCEINLINE void AddTask(const TFunction<void()>& TaskCallback)
    {
        if (TaskCallback)
        {
            TaskList.Emplace(
                (TaskList.Num() == 0) ? Future.Get() : Future->AllocateNext(),
                TaskCallback
                );
        }
    }

    void Merge(FGWTAsyncTask& OtherTask, bool bResetOther = false)
    {
        if (IsValid())
        {
            for (FGWTEventTask& Task : OtherTask.GetTaskList())
            {
                AddTask(Task.Value);
            }

            if (bResetOther)
            {
                OtherTask.Reset();
            }
        }
    }

    bool EnqueueTask(TFunction<void()> CompletionCallback = TFunction<void()>())
    {
        if (ThreadPool != nullptr && Future.IsValid())
        {
            ThreadPool->AddQueuedEventChain(TaskList, nullptr, CompletionCallback);
            return true;
        }

        return false;
    }
};

USTRUCT(BlueprintType)
struct GENERICWORKERTHREAD_API FGWTAsyncTaskRef
{
    GENERATED_BODY()

    FPSGWTAsyncThreadPool ThreadPool = nullptr;
    FPSGWTAsyncTask Task             = nullptr;
    TArray<FPSGWTAsyncTask> ChainedTasks;

    FGWTAsyncTaskRef() = default;

    FGWTAsyncTaskRef(const FPSGWTEventFuture& InFuture, FGWTAsyncThreadPool& InThreadPool)
        : Task(new FGWTAsyncTask(InFuture, InThreadPool))
    {
    }

    FGWTAsyncTaskRef(const FPSGWTEventFuture& InFuture, const FPRGWTAsyncThreadPool& InThreadPool)
        : Task(new FGWTAsyncTask(InFuture, *InThreadPool))
        , ThreadPool(InThreadPool)
    {
    }

    FORCEINLINE static void Init(FGWTAsyncTaskRef& TaskRef, const FPSGWTAsyncThreadPool& InThreadPool)
    {
        check(InThreadPool.IsValid());
        TaskRef.Reset();
        TaskRef.ThreadPool = InThreadPool;
        TaskRef.Task = MakeShareable(
            new FGWTAsyncTask(
                MakeShareable(new FGWTEventFuture),
                *TaskRef.ThreadPool
                ) );
    }

    FORCEINLINE static void Init(FGWTAsyncTaskRef& TaskRef, const FPRGWTAsyncThreadPool& InThreadPool)
    {
        TaskRef.Reset();
        TaskRef.ThreadPool = InThreadPool;
        TaskRef.Task = MakeShareable(
            new FGWTAsyncTask(
                MakeShareable(new FGWTEventFuture),
                *TaskRef.ThreadPool
                ) );
    }

    FORCEINLINE void Init(const FPSGWTAsyncThreadPool& InThreadPool)
    {
        check(InThreadPool.IsValid());
        Reset();
        ThreadPool = InThreadPool;
        Task = MakeShareable(
            new FGWTAsyncTask(
                MakeShareable(new FGWTEventFuture),
                *ThreadPool
                ) );
    }

    FORCEINLINE void Reset()
    {
        if (Task.IsValid())
        {
            Task->Reset();
        }

        ChainedTasks.Empty();

        TaskProgress = -1;
    }

    FORCEINLINE bool IsValid() const
    {
        return Task.IsValid() && Task->IsValid();
    }

    FORCEINLINE bool IsIdle() const
    {
        return ! Task.IsValid() || TaskProgress == -1;
    }

    FORCEINLINE bool IsExecuted() const
    {
        return TaskProgress != -1;
    }

    FORCEINLINE bool IsDone() const
    {
        return IsValid() && (TaskProgress == 0);
    }

    FORCEINLINE void Wait()
    {
        if (IsValid())
        {
            Task->Wait();
        }
    }

    FORCEINLINE void AddTask(const TFunction<void()>& TaskCallback)
    {
        if (IsValid() && IsIdle())
        {
            Task->AddTask(TaskCallback);
        }
    }

    void AddTaskChain(FGWTAsyncTaskRef& OtherTaskRef, bool bResetOther = false)
    {
        if (! IsIdle() || ! OtherTaskRef.IsValid())
        {
            return;
        }

        if (IsValid())
        {
            ChainedTasks.Emplace(Task);
        }

        Task = OtherTaskRef.Task;

        if (bResetOther)
        {
            OtherTaskRef.Reset();
        }
    }

    void AddTaskChain(const TFunction<void()>& TaskCallback)
    {
        if (! IsIdle() || ! IsValid() || ! TaskCallback)
        {
            return;
        }

        // Task list is empty, add task to the list without creating new task object
        if (Task->TaskList.Num() == 0)
        {
            AddTask(TaskCallback);
        }
        // Create new task object and chain the task callback
        else
        {
            ChainedTasks.Emplace(Task);

            Task = MakeShareable(new FGWTAsyncTask(
                MakeShareable(new FGWTEventFuture),
                *ThreadPool
                ) );
            Task->AddTask(TaskCallback);
        }
    }

    FORCEINLINE void Merge(FGWTAsyncTaskRef& OtherTaskRef, bool bResetOther = false)
    {
        if (IsValid() && OtherTaskRef.IsValid() && IsIdle())
        {
            Task->Merge(*OtherTaskRef.Task, bResetOther);
        }
    }

    bool EnqueueTask()
    {
        // Task is invalid or is currently in progress, abort
        if (! IsValid() || ! IsIdle())
        {
            return false;
        }

        bool bResult = false;

        if (ChainedTasks.Num() > 0)
        {
            TSharedPtr<TFunction<void()>> Callback0(nullptr);
            TSharedPtr<TFunction<void()>> Callback1(nullptr);

            {
                FPSGWTAsyncTask& ChainedTask(Task);
                Callback1 = MakeShareable(new TFunction<void()>(
                    [ChainedTask, this]()
                    {
                        check(ChainedTask.IsValid());
                        ChainedTask->EnqueueTask([this](){ TaskProgress = 0; });
                    } ) );
            }

            for (int32 i=(ChainedTasks.Num()-1); i>=1; --i)
            {
                FPSGWTAsyncTask& ChainedTask(ChainedTasks[i]);
                Callback0 = Callback1;
                Callback1 = MakeShareable(new TFunction<void()>(
                    [Callback0, ChainedTask]()
                    {
                        check(ChainedTask.IsValid());
                        ChainedTask->EnqueueTask(*Callback0);
                    } ) );
            }

            check(ChainedTasks[0].IsValid());

            TaskProgress = 1;
            bResult = ChainedTasks[0]->EnqueueTask(*Callback1);
        }
        else
        {
            TaskProgress = 1;
            bResult = Task->EnqueueTask([this](){ TaskProgress = 0; });
        }

        return bResult;
    }

private:
    int32 TaskProgress = -1;
};

UCLASS(BlueprintType)
class GENERICWORKERTHREAD_API UGWTAsyncTaskObject : public UObject
{
	GENERATED_BODY()

public:

	UPROPERTY(BlueprintReadWrite)
    FGWTAsyncTaskRef TaskRef;

	UPROPERTY(BlueprintAssignable)
    FGWTAsyncTaskObject_OnTaskDone OnTaskDone;

    UFUNCTION(BlueprintCallable)
    void ResetTask()
    {
        TaskRef.Reset();
    }

    UFUNCTION(BlueprintCallable)
    bool IsTaskValid() const
    {
        return TaskRef.IsValid();
    }

    UFUNCTION(BlueprintCallable)
    bool IsTaskIdle() const
    {
        return TaskRef.IsIdle();
    }

    UFUNCTION(BlueprintCallable)
    bool IsTaskExecuted() const
    {
        return TaskRef.IsExecuted();
    }

    UFUNCTION(BlueprintCallable)
    bool IsTaskDone() const
    {
        return TaskRef.IsDone();
    }

    UFUNCTION(BlueprintCallable)
    void WaitTask()
    {
        TaskRef.Wait();
    }

    UFUNCTION(BlueprintCallable)
    void AddTaskChain(UPARAM(ref) FGWTAsyncTaskRef& OtherTaskRef, bool bResetOther = false)
    {
        TaskRef.AddTaskChain(OtherTaskRef, bResetOther);
    }

    UFUNCTION(BlueprintCallable)
    bool ExecuteTask()
    {
        return TaskRef.EnqueueTask();
    }
};
