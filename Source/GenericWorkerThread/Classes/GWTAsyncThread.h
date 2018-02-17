// 

#pragma once

#include "Async/Async.h"
#include "HAL/PlatformProcess.h"
#include "HAL/ThreadSafeBool.h"
#include "Containers/Queue.h"
#include "Containers/List.h"
#include "GWTTaskWorker.h"

typedef TSharedPtr<class FGWTAsyncThread> TPSGWTAsyncThread;
typedef TWeakPtr<class FGWTAsyncThread>   TPWGWTAsyncThread;

class FGWTAsyncThread
{

public:

    typedef TFunction<void()> FAsyncCallback;

	FGWTAsyncThread(float InRestTime)
        : RestTime(InRestTime)
        , PauseEvent(nullptr)
        , bIsThreadStopped(false)
        , bIsThreadPaused(false)
    {
        check(RestTime > 0.0001f);
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
        SetPause(false);

        FAsyncCallback AsyncExec( [&, this]() {
            PauseEvent = FGenericPlatformProcess::GetSynchEventFromPool(false);

            Run();

            FGenericPlatformProcess::ReturnSynchEventToPool(PauseEvent);
            PauseEvent = nullptr;
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
        SetPause(false);

        ThreadFuture.Get();
        ThreadFuture = TFuture<void>();

        // Process pending worker entries
        ProcessWorkerEntries();

        // Registers remaining workers for removal
        for (TPWGWTTaskWorker& t : WorkerList)
        {
            RemoveWorkerAsync(t);
        }

        // Finally, proceed to remove all workers
        ProcessWorkerEntries();
    }

    void SetPause(bool state)
    {
        if (bIsThreadPaused != state)
        {
            bIsThreadPaused = state;

            if (! bIsThreadPaused && PauseEvent)
            {
                PauseEvent->Trigger();
            }
        }
    }

	void SetRestTime(float InRestTime)
	{
        check(InRestTime > 0.0001f);
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

	FORCEINLINE bool IsThreadPaused() const
	{
		return bIsThreadPaused;
	}

	// === END Thread Control

	void AddWorker(TPWGWTTaskWorker w)
	{
        if (! IsThreadStarted() || ! IsThreadStopped())
        {
            WorkerEntries.Enqueue(w);
        }
	}

	FORCEINLINE TFuture<void> RemoveWorker(TPWGWTTaskWorker Worker)
	{
        WorkerRemovals.Enqueue(Worker);
        TPSRemovalPromise RemovalPromise( MakeShareable(new TPromise<void>()) );
        WorkerRemovalPromises.Enqueue(RemovalPromise);
        return RemovalPromise->GetFuture();
	}

	FORCEINLINE void RemoveWorkerAsync(TPWGWTTaskWorker Worker)
	{
        WorkerRemovals.Enqueue(Worker);
	}

private:

    typedef TDoubleLinkedList<TPWGWTTaskWorker>       TGWTTaskWorkerList;
    typedef TGWTTaskWorkerList::TDoubleLinkedListNode TGWTTaskWorkerListNode;
    typedef TSharedPtr<TPromise<void>, ESPMode::ThreadSafe> TPSRemovalPromise;

    TFuture<void> ThreadFuture;
	FEvent* PauseEvent = nullptr;

	FThreadSafeBool bIsThreadStopped = false;
	FThreadSafeBool bIsThreadPaused = false;
	float RestTime;

    TGWTTaskWorkerList WorkerList;
	TQueue<TPWGWTTaskWorker> WorkerEntries;
	TQueue<TPWGWTTaskWorker> WorkerRemovals;
    TQueue<TPSRemovalPromise> WorkerRemovalPromises;

    int32 _UniqueWorkerId = 0;

	void Run()
    {
        double currentTime = FPlatformTime::Seconds();
        FPlatformProcess::Sleep(0.03);

        while (! IsThreadStopped())
        {
            double deltaTime = FPlatformTime::Seconds() - currentTime;

            if (IsThreadPaused())
            {
                PauseEvent->Wait();
                continue;
            }

            ProcessWorkerEntries();

            if (WorkerList.Num() > 0)
            {
                TGWTTaskWorkerListNode* Node0( nullptr );
                TGWTTaskWorkerListNode* Node1( WorkerList.GetHead() );
                do
                {
                    Node0 = Node1;
                    Node1 = Node0->GetNextNode();

                    TPSGWTTaskWorker Worker( Node0->GetValue().Pin() );

                    if (Worker.IsValid())
                    {
                        Worker->Tick(deltaTime);
                    }
                    else
                    {
                        WorkerList.RemoveNode(Node0);
                    }
                }
                while ( Node1 );
            }

            currentTime = FPlatformTime::Seconds();
            FPlatformProcess::Sleep(RestTime);
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
            TPWGWTTaskWorker pWorker;
            WorkerEntries.Dequeue(pWorker);

            if (pWorker.IsValid() && ! WorkerList.Contains(pWorker))
            {
                TPSGWTTaskWorker Worker( pWorker.Pin() );
                WorkerList.AddTail(Worker);
                Worker->_WorkerId = _UniqueWorkerId++;
                Worker->Setup();
            }
        }

        while (! WorkerRemovals.IsEmpty())
        {
            TPWGWTTaskWorker pWorker;
            WorkerRemovals.Dequeue(pWorker);

            if (WorkerList.Contains(pWorker))
            {
                WorkerList.RemoveNode(pWorker);

                if (pWorker.IsValid())
                {
                    TPSGWTTaskWorker Worker( pWorker.Pin() );
                    Worker->Shutdown();
                    Worker->_WorkerId = -1;
                }
            }
        }

        while (! WorkerRemovalPromises.IsEmpty())
        {
            TPSRemovalPromise RemovalPromise;
            WorkerRemovalPromises.Dequeue(RemovalPromise);
            RemovalPromise->SetValue();
        }
    }
};
