/*
	
*/

#include "GenericWorkerThread.h"
#include "GWTAsyncThread.h"
#include "GWTTaskWorker.h"

#define LOCTEXT_NAMESPACE "FGenericWorkerThread"

class FGenericWorkerThread : public IGenericWorkerThread
{
    int32 UniqueThreadID = 0;
    TMap<int32, TPSGWTAsyncThread> ThreadInstanceMap;

public:

    // -- BEGIN IModuleInterface

	virtual void StartupModule() override
	{
    }

	virtual void ShutdownModule() override
	{
    }

    // -- END IModuleInterface

    virtual int32 CreateInstance(float InRestTime)
    {
        int32 ThreadId = UniqueThreadID++;
        TPSGWTAsyncThread AsyncThread( new FGWTAsyncThread(InRestTime) );
        ThreadInstanceMap.Add(ThreadId, AsyncThread);
        return ThreadId;
    }

    FORCEINLINE virtual TPWGWTAsyncThread GetThread(int32 ThreadId)
    {
        return ThreadInstanceMap.Contains(ThreadId)
            ? ThreadInstanceMap.FindChecked(ThreadId)
            : TPWGWTAsyncThread();
    }

    virtual void AddWorker(int32 ThreadId, TPWGWTTaskWorker TaskWorker)
    {
        TPSGWTAsyncThread AsyncThread( GetThread(ThreadId).Pin() );
        if (AsyncThread.IsValid())
        {
            AsyncThread->AddWorker(TaskWorker);
        }
    }

    virtual TFuture<void> RemoveWorker(int32 ThreadId, TPWGWTTaskWorker TaskWorker)
    {
        TPSGWTAsyncThread AsyncThread( GetThread(ThreadId).Pin() );
        if (AsyncThread.IsValid())
        {
            return AsyncThread->RemoveWorker(TaskWorker);
        }
        return TFuture<void>();
    }

    virtual void RemoveWorkerAsync(int32 ThreadId, TPWGWTTaskWorker TaskWorker)
    {
        TPSGWTAsyncThread AsyncThread( GetThread(ThreadId).Pin() );
        if (AsyncThread.IsValid())
        {
            AsyncThread->RemoveWorkerAsync(TaskWorker);
        }
    }

    virtual void SetRestTime(int32 ThreadId, float InRestTime)
    {
        TPSGWTAsyncThread AsyncThread( GetThread(ThreadId).Pin() );
        if (AsyncThread.IsValid())
        {
            AsyncThread->SetRestTime(InRestTime);
        }
    }

    FORCEINLINE virtual float GetRestTime(int32 ThreadId)
    {
        TPSGWTAsyncThread AsyncThread( GetThread(ThreadId).Pin() );
        if (AsyncThread.IsValid())
        {
            return AsyncThread->GetRestTime();
        }
        return -1.f;
    }
};

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FGenericWorkerThread, GenericWorkerThread)
