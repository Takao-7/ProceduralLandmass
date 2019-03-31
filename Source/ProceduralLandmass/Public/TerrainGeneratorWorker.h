// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
#include "CoreMinimal.h"
#include "Runnable.h"
#include "Array2D.h"


class UCurveFloat;


/**
 * Class for generating mesh data.
 */
class PROCEDURALLANDMASS_API FTerrainGeneratorWorker : public FRunnable
{	
protected:
	

public:
	FTerrainGeneratorWorker();
	~FTerrainGeneratorWorker() {};


	/////////////////////////////////////////////////////
				/* Runnable interface */
	/////////////////////////////////////////////////////
public:
	virtual bool Init() override;
	virtual uint32 Run() override;
	virtual void Stop() override;
	virtual void Exit() override;

};
