#include "BossFelledWidget.h"
#include "Components/TextBlock.h"

void UBossFelledWidget::SetBossFelledText(const FText& InText)
{
	if (TXT_BossFelled)
	{
		TXT_BossFelled->SetText(InText);
	}
}