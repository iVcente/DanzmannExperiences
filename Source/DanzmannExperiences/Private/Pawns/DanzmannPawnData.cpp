// Copyright (C) 2026 Vicente Danzmann. All Rights Reserved.

#include "Pawns/DanzmannPawnData.h"

const FPrimaryAssetType UDanzmannPawnData::PrimaryAssetType(TEXT("PawnData"));

FPrimaryAssetId UDanzmannPawnData::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(PrimaryAssetType, GetFName());
}
