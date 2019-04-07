// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
#include "CoreMinimal.h"
#include "Runnable.h"
#include "Queue.h"
#include "MeshDataJob.h"
#include "ThreadSafeBool.h"


class FRunnableThread;


/**
 * Class for generating mesh data.
 * This thread will work through the PendingJobs queue until it's empty.
 * When it's empty, it will put itself to sleep until woken up.
 */
class PROCEDURALLANDMASS_API FTerrainGeneratorWorker : public FRunnable
{	
public:
	/* Job queue for this worker. */
	TQueue<FMeshDataJob, EQueueMode::Spsc> PendingJobs;

	/* Should this thread be killed? */
	FThreadSafeBool bKill = false;

	/* Put this thread to sleep at the end of it's current job. */
	void Pause() { bPause = true; };
	void UnPause();

	void ClearJobQueue();

	FTerrainGeneratorWorker();
	~FTerrainGeneratorWorker();

private:
	/* Should this thread pause until woken up? (@see Semaphore). */
	FThreadSafeBool bPause = false;

	/* Critical section for locking variables (thread safety). */
	FCriticalSection CriticalSection;

	/* This will be used to let this thread wait until it is woken up the main thread. */
	FEvent* Semaphore;

	/* The thread we are running on. */
	FRunnableThread* Thread;

	static int32 ThreadCounter;

	static int32 GetNewThreadNumber() { return ++ThreadCounter; };


	/////////////////////////////////////////////////////
				/* Runnable interface */
	/////////////////////////////////////////////////////
public:
	virtual bool Init() override { return true; };
	virtual uint32 Run() override;
	virtual void Stop() override { Pause(); };
	virtual void Exit() override {};
};
