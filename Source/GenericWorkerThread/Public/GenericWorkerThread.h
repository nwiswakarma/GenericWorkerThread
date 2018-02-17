/*
 
 -----
 
*/

#pragma once

#include "CoreMinimal.h"
#include "Stats/Stats.h"
#include "Modules/ModuleInterface.h"

class IGenericWorkerThread : public IModuleInterface
{
public:

	FORCEINLINE static IGenericWorkerThread& Get()
	{
		return FModuleManager::LoadModuleChecked<IGenericWorkerThread>("GenericWorkerThread");
	}

	FORCEINLINE static bool IsAvailable()
	{
        return FModuleManager::Get().IsModuleLoaded("GenericWorkerThread");
	}

    virtual int32 CreateInstance(float InRestTime = .03f) = 0;
    virtual TWeakPtr<class FGWTAsyncThread> GetThread(int32 ThreadId) = 0;
    virtual void AddWorker(int32 ThreadId, TWeakPtr<class IGWTTaskWorker> TaskWorker) = 0;
    virtual TFuture<void> RemoveWorker(int32 ThreadId, TWeakPtr<class IGWTTaskWorker> TaskWorker) = 0;
    virtual void RemoveWorkerAsync(int32 ThreadId, TWeakPtr<class IGWTTaskWorker> TaskWorker) = 0;
    virtual void SetRestTime(int32 ThreadId, float InRestTime) = 0;
    virtual float GetRestTime(int32 ThreadId) = 0;
};
