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

#include "GenericWorkerThread.h"
#include "GWTAsyncThreadManager.h"
#include "GWTTaskWorker.h"
#include "GWTTickManager.h"

#define LOCTEXT_NAMESPACE "FGenericWorkerThread"

class FGenericWorkerThread : public IGenericWorkerThread
{
    FGWTAsyncThreadManager AsyncThreadManager;
    FGWTTickManager        TickManager;

public:

    // -- BEGIN IModuleInterface

	virtual void StartupModule() override
	{
    }

	virtual void ShutdownModule() override
	{
    }

    // -- END IModuleInterface

    virtual FGWTAsyncThreadManager& GetAsyncThreadManager()
    {
        return AsyncThreadManager;
    }

    virtual const FGWTAsyncThreadManager& GetAsyncThreadManager() const
    {
        return AsyncThreadManager;
    }

    virtual FGWTTickManager& GetTickManager()
    {
        return TickManager;
    }

    virtual const FGWTTickManager& GetTickManager() const
    {
        return TickManager;
    }
};

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FGenericWorkerThread, GenericWorkerThread)
