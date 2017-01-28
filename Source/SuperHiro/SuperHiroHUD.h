#pragma once
#include "GameFramework/HUD.h"
#include "SuperHiroHUD.generated.h"

UCLASS()
class ASuperHiroHUD : public AHUD
{
	GENERATED_BODY()
public:
	ASuperHiroHUD();
	virtual void DrawHUD() override;

private:
	class UTexture2D* SuperHiroCrosshair;
};