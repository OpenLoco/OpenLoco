#include "ChangeAutopayMinimumBalance.h"
#include "Economy/Economy.h"
#include "GameCommands/GameCommands.h"
#include "Localisation/StringIds.h"
#include "Ui/WindowManager.h"
#include "World/CompanyManager.h"

using namespace OpenLoco::Interop;

namespace OpenLoco::GameCommands
{
    static const currency32_t kMaximumAutopayMinimumBalance = 500000;

    static uint32_t changeAutopayMinBalance(const currency32_t newMinimumBalance, const uint8_t flags)
    {
        auto* company = CompanyManager::get(GameCommands::getUpdatingCompanyId());

        if (newMinimumBalance < 0)
        {
            return 0;
        }
        else if (newMinimumBalance > kMaximumAutopayMinimumBalance)
        {
            GameCommands::setErrorTitle(StringIds::loan_autopay_min_balance_upper_limit_title);
            GameCommands::setErrorText(StringIds::loan_autopay_min_balance_upper_limit_text);
            return FAILURE;
        }

        if (flags & Flags::apply)
        {
            company->setLoanAutopayMinimumBalance(newMinimumBalance);
            Ui::WindowManager::invalidate(Ui::WindowType::company, static_cast<uint16_t>(GameCommands::getUpdatingCompanyId()));
            if (CompanyManager::getControllingId() == GameCommands::getUpdatingCompanyId())
            {
                Ui::Windows::PlayerInfoPanel::invalidateFrame();
            }
        }
        return 0;
    }

    void changeAutopayMinBalance(registers& regs)
    {
        ChangeAutopayMinimumBalanceArgs args(regs);
        regs.ebx = changeAutopayMinBalance(args.newMinimumBalance, regs.bl);
    }
}
