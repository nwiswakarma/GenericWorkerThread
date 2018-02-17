// 

#pragma once

#include "Templates/SharedPointer.h"

typedef TSharedPtr<class IGWTTaskWorker> TPSGWTTaskWorker;
typedef TWeakPtr<class IGWTTaskWorker>   TPWGWTTaskWorker;

class IGWTTaskWorker
{
    friend class FGWTAsyncThread;
    int32 _WorkerId = -1;

    virtual bool operator==(const IGWTTaskWorker& rhs) const
    {
        return _WorkerId == rhs._WorkerId;
    }

public:

    virtual void Setup()
    {
    }

    virtual void Shutdown()
    {
    }

    virtual void Tick(float DeltaTime) = 0;

};
