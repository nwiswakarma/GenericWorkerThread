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

#include "Async/Async.h"
#include "HAL/PlatformProcess.h"
#include "HAL/ThreadSafeBool.h"
#include "Containers/Queue.h"
#include "Containers/List.h"
#include "GWTTaskWorker.h"

typedef TSharedPtr<class FGWTAsyncThread> FPSGWTAsyncThread;
typedef TWeakPtr<class FGWTAsyncThread>   FPWGWTAsyncThread;

class FGWTAsyncThread
{

public:

    typedef TFunction<void()> FAsyncCallback;

	FGWTAsyncThread(float InRestTime)
        : RestTime(InRestTime)
        , bIsThreadStopped(false)
    {
    }

	~FGWTAsyncThread()
    {
        StopThread();

        WorkerList.Empty();
        WorkerEntries.Empty();
        WorkerRemovals.Empty();
        WorkerRemovalPromises.Empty();
	}

    void StartThread()
    {
        StartThread( [=](){} );
    }

    void StartThread(FAsyncCallback InAsyncCallback)
    {
        if (IsThreadStarted())
        {
            return;
        }

        bIsThreadStopped = false;

        FAsyncCallback AsyncExec( [&, this]() {
            Run();
        } );

        ThreadFuture = Async<void>(
            EAsyncExecution::Thread,
            AsyncExec,
            InAsyncCallback
        );
    }

    void StopThread()
    {
        if (! IsThreadStarted())
        {
            return;
        }

        check(! IsThreadStopped());

        bIsThreadStopped = true;

        ThreadFuture.Get();
        ThreadFuture = TFuture<void>();

        // Process pending worker entries
        ProcessWorkerEntries();

        // Registers remaining workers for removal
        for (FPWGWTTaskWorker& t : WorkerList)
        {
            RemoveWorkerAsync(t);
        }

        // Finally, proceed to remove all workers
        ProcessWorkerEntries();
    }

	void SetRestTime(float InRestTime)
	{
        RestTime = InRestTime;
	}

	FORCEINLINE float GetRestTime() const
	{
        return RestTime;
	}

	FORCEINLINE bool IsThreadStarted() const
	{
		return ThreadFuture.IsValid();
	}

	FORCEINLINE bool IsThreadStopped() const
	{
		return bIsThreadStopped;
	}

	// === END Thread Control

	void AddWorker(FPWGWTTaskWorker w)
	{
        WorkerEntries.Enqueue(w);
	}

	FORCEINLINE TFuture<void> RemoveWorker(FPWGWTTaskWorker Worker)
	{
        WorkerRemovals.Enqueue(Worker);
        FPSRemovalPromise RemovalPromise( MakeShareable(new TPromise<void>()) );
        WorkerRemovalPromises.Enqueue(RemovalPromise);
        return RemovalPromise->GetFuture();
	}

	FORCEINLINE void RemoveWorkerAsync(FPWGWTTaskWorker Worker)
	{
        WorkerRemovals.Enqueue(Worker);
	}

private:

    typedef TDoubleLinkedList<FPWGWTTaskWorker>       FGWTTaskWorkerList;
    typedef FGWTTaskWorkerList::TDoubleLinkedListNode FGWTTaskWorkerListNode;
    typedef TSharedPtr<TPromise<void>> FPSRemovalPromise;

    TFuture<void> ThreadFuture;
	FThreadSafeBool bIsThreadStopped;
	float RestTime;

    FGWTTaskWorkerList WorkerList;
	TQueue<FPWGWTTaskWorker, EQueueMode::Mpsc> WorkerEntries;
	TQueue<FPWGWTTaskWorker, EQueueMode::Mpsc> WorkerRemovals;
    TQueue<FPSRemovalPromise, EQueueMode::Mpsc> WorkerRemovalPromises;

    int32 _UniqueWorkerId = 0;

	void Run()
    {
        double LastUpdateTime = FPlatformTime::Seconds();

        // Initial sleep
        FPlatformProcess::Sleep(0.03);

        while (! IsThreadStopped())
        {
            ProcessWorkerEntries();

            if (WorkerList.Num() > 0)
            {
                FGWTTaskWorkerListNode* Node0( nullptr );
                FGWTTaskWorkerListNode* Node1( WorkerList.GetHead() );
                do
                {
                    Node0 = Node1;
                    Node1 = Node0->GetNextNode();

                    FPSGWTTaskWorker Worker( Node0->GetValue().Pin() );

                    if (Worker.IsValid())
                    {
                        double DeltaTime = FPlatformTime::Seconds() - LastUpdateTime;
                        Worker->Tick(DeltaTime);
                    }
                    else
                    {
                        WorkerList.RemoveNode(Node0);
                    }
                }
                while (Node1);
            }

            LastUpdateTime = FPlatformTime::Seconds();

            if (RestTime > 0.f)
            {
                FPlatformProcess::Sleep(RestTime);
            }
        }
	}

    void ProcessWorkerEntries()
    {
        if (WorkerEntries.IsEmpty() && WorkerRemovals.IsEmpty())
        {
            return;
        }

        while (! WorkerEntries.IsEmpty())
        {
            FPWGWTTaskWorker pWorker;
            WorkerEntries.Dequeue(pWorker);

            if (pWorker.IsValid() && ! WorkerList.Contains(pWorker))
            {
                FPSGWTTaskWorker Worker( pWorker.Pin() );
                WorkerList.AddTail(Worker);
                Worker->_TaskWorkerId = _UniqueWorkerId++;
                Worker->SetupTaskWorker();
            }
        }

        while (! WorkerRemovals.IsEmpty())
        {
            FPWGWTTaskWorker pWorker;
            WorkerRemovals.Dequeue(pWorker);

            if (WorkerList.Contains(pWorker))
            {
                WorkerList.RemoveNode(pWorker);

                if (pWorker.IsValid())
                {
                    FPSGWTTaskWorker Worker( pWorker.Pin() );
                    Worker->ShutdownTaskWorker();
                    Worker->_TaskWorkerId = -1;
                }
            }
        }

        while (! WorkerRemovalPromises.IsEmpty())
        {
            FPSRemovalPromise RemovalPromise;
            WorkerRemovalPromises.Dequeue(RemovalPromise);
            RemovalPromise->SetValue();
        }
    }
};
