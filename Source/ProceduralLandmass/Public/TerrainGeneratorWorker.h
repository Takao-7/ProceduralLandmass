// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
#include "CoreMinimal.h"
#include "Runnable.h"
#include "Queue.h"
#include "MeshDataJob.h"
#include "ThreadSafeBool.h"


class FRunnableThread;


/**
 * Worker thread for generating mesh data.
 * This thread will work through the PendingJobs queue until it's empty.
 * When it's empty, it will destroy itself.
 * Will start in pause mode and must be 'activated' manually with @see UnPause().
 */
class PROCEDURALLANDMASS_API FTerrainGeneratorWorker : public FRunnable
{	
public:
	/**
	 * Creates a new terrain generator worker thread and starts it.
	 * @param configuration The terrain configuration to use. We will make a copy of it to be thread-safe.
	 * @param outer The outer object for the terrain configuration's objects (noise generator and float curve).
	 */
	FTerrainGeneratorWorker(const FTerrainConfiguration& configuration, UObject* outer);
	~FTerrainGeneratorWorker();

	/* Job queue for this worker. */
	TQueue<FMeshDataJob, EQueueMode::Spsc> PendingJobs;

	/* Priority job queue for this worker. */
	TQueue<FMeshDataJob, EQueueMode::Spsc> PriorityJobs;

	/* Does the actual work of this thread. This function is static,
	 * so it can be called manually if we are not using multi-threading. */
	void DoWork(FMeshDataJob& currentJob);

	FORCEINLINE void Pause() { bPause = true; }
	FORCEINLINE void UnPause()
	{
		bPause = false;
		Semaphore->Trigger();
	}

	void UpdateConfiguration(const FTerrainConfiguration& newConfig, UObject* outer);

private:
	FThreadSafeBool bPause = false;

	/* Should this thread be killed? */
	FThreadSafeBool bWorkFinished = false;

	/* This will be used to let this thread wait until it is woken up the main thread. */
	FEvent* Semaphore;

	/* The thread we are running on. */
	FRunnableThread* Thread;

	FTerrainConfiguration Configuration;

	FString ThreadName;

	static int32 ThreadCounter;
	static int32 GetNewThreadNumber() { return ThreadCounter++; };

	/* Clears the job queue and deletes the data inside. */
	void ClearJobQueue();


	/////////////////////////////////////////////////////
				/* Runnable interface */
	/////////////////////////////////////////////////////
public:
	//virtual bool Init() override { return true; };
	virtual uint32 Run() override;
	virtual void Stop() override;
	//virtual void Exit() override;
};
