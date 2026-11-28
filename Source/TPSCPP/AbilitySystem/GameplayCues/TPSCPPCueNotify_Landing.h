#pragma once

#include "GameplayCueNotify_Static.h"
#include "TPSCPPCueNotify_Landing.generated.h"

UCLASS()
class TPSCPP_API UTPSCPPCueNotify_Landing : public UGameplayCueNotify_Static
{
	GENERATED_BODY()

public:
	virtual void PostInitProperties() override;
	virtual bool OnExecute_Implementation(AActor* Target, const FGameplayCueParameters& Parameters) const override;
};
