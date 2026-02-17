#include "ChangeLoan.h"
#include "Economy/Economy.h"
#include "GameCommands/GameCommands.h"
#include "Localisation/StringIds.h"
#include "Ui/WindowManager.h"
#include "World/CompanyManager.h"

#include <algorithm>

namespace OpenLoco::GameCommands
{
    // 0x0046DE88
    static uint32_t changeLoan(const currency32_t newLoan, const uint8_t flags)
    {
        GameCommands::setExpenditureType(ExpenditureType::LoanInterest);

        const auto maxLoan = Economy::getInflationAdjustedCost(CompanyManager::getMaxLoanSize(), 0, 8) / 100 * 100;
        auto* company = CompanyManager::get(GameCommands::getUpdatingCompanyId());

        const currency32_t clampedNewLoan = std::clamp<currency32_t>(newLoan, 0, maxLoan);
        const currency32_t loanDifference = company->currentLoan - clampedNewLoan;

        if (company->currentLoan > clampedNewLoan)
        {
            if (company->cash < loanDifference)
            {
                GameCommands::setErrorText(StringIds::not_enough_cash_available);
                return FAILURE;
            }
        }
        else
        {
            if (company->currentLoan >= maxLoan)
            {
                GameCommands::setErrorText(StringIds::bank_refuses_to_increase_loan);
                return FAILURE;
            }
        }

        if (flags & Flags::apply)
        {
            company->currentLoan = clampedNewLoan;
            company->cash -= loanDifference;
            Ui::WindowManager::invalidate(Ui::WindowType::company, static_cast<uint16_t>(GameCommands::getUpdatingCompanyId()));
            if (CompanyManager::getControllingId() == GameCommands::getUpdatingCompanyId())
            {
                Ui::Windows::PlayerInfoPanel::invalidateFrame();
            }
        }
        return 0;
    }

    void changeLoan(registers& regs)
    {
        ChangeLoanArgs args(regs);
        regs.ebx = changeLoan(args.newLoan, regs.bl);
    }
}
