#include "CompanyManager.h"
#include "Economy/Economy.h"
#include "GameCommands.h"
#include "Localisation/StringIds.h"
#include "Ui/WindowManager.h"

using namespace OpenLoco::Interop;

namespace OpenLoco::GameCommands
{
    // 0x0046DE88
    static uint32_t changeLoan(const currency32_t newLoan, const uint8_t flags)
    {
        GameCommands::setExpenditureType(ExpenditureType::LoanInterest);

        auto* company = CompanyManager::get(CompanyManager::getUpdatingCompanyId());
        const currency32_t loanDifference = company->currentLoan - newLoan;

        if (company->currentLoan > newLoan)
        {
            if (company->cash < loanDifference)
            {
                GameCommands::setErrorText(StringIds::not_enough_cash_available);
                return FAILURE;
            }
        }
        else
        {
            const auto maxLoan = Economy::getInflationAdjustedCost(CompanyManager::getMaxLoanSize(), 0, 8);
            if (newLoan > maxLoan)
            {
                GameCommands::setErrorText(StringIds::bank_refuses_to_increase_loan);
                return FAILURE;
            }
        }

        if (flags & Flags::apply)
        {
            company->currentLoan = newLoan;
            company->cash -= loanDifference;
            Ui::WindowManager::invalidate(Ui::WindowType::company, static_cast<uint16_t>(CompanyManager::getUpdatingCompanyId()));
            if (CompanyManager::getControllingId() == CompanyManager::getUpdatingCompanyId())
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
