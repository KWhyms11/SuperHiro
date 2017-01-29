// Fill out your copyright notice in the Description page of Project Settings.

#include "SuperHiro.h"
#include "SuperHiroCharacter.h"
#include "SuperHiroHUD.h"
#include "Engine/Canvas.h"

ASuperHiroHUD::ASuperHiroHUD() 
{
	// Set the crosshair texture
	static ConstructorHelpers::FObjectFinder<UTexture2D> CrosshiarTexObj(TEXT("Texture2D'/Game/ThirdPerson/Textures/Crosshair.FirstPersonCrosshair'"));
	SuperHiroCrosshair = CrosshiarTexObj.Object;
}

void ASuperHiroHUD::DrawHUD()
{
	Super::DrawHUD();

	// Draw very simple crosshair

	// find center of the Canvas
	const FVector2D Center(Canvas->ClipX * 0.5f, Canvas->ClipY * 0.5f);

	// offset by half the texture's dimensions so that the center of the texture aligns with the center of the Canvas
	const FVector2D CrosshairDrawPosition((Center.X - (SuperHiroCrosshair->GetSurfaceWidth() * 0.5)),
		(Center.Y - (SuperHiroCrosshair->GetSurfaceHeight() * 0.5f)));

	// draw the crosshair
	FCanvasTileItem TileItem(CrosshairDrawPosition, SuperHiroCrosshair->Resource, FLinearColor::White);
	TileItem.BlendMode = SE_BLEND_Translucent;

	ASuperHiroCharacter* me = Cast<ASuperHiroCharacter>(GetOwningPlayerController()->GetCharacter());

	if (me->bIsAiming)
	{
		Canvas->DrawItem(TileItem);
	}
	
	

}
