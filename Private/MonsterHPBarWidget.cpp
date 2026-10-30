#include "MonsterHPBarWidget.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"

void UMonsterHPBarWidget::UpdateHP(float Current, float Max)
{
	if (!PB_HP)
	{
		return;
	}

	const float Percent = (Max > 0.f) ? (Current / Max) : 0.f;
	PB_HP->SetPercent(Percent);
}

void UMonsterHPBarWidget::UpdateMonsterName(const FText& NewName)
{
	if (!TXT_MonsterName)
	{
		return;
	}

	TXT_MonsterName->SetText(NewName);
}