#include "../CompanyManager.h"
#include "../Economy/Economy.h"
#include "../Localisation/StringIds.h"
#include "../Ui/WindowManager.h"
#include "GameCommands.h"
using namespace OpenLoco::Interop;

namespace OpenLoco::GameCommands
{
    static loco_global<CompanyId, 0x009C68EB> _updatingCompanyId;
    static loco_global<uint16_t, 0x0052621A> maxLoanSize;

    // 0x0046DE88
    static uint32_t changeLoan(const currency32_t newLoan, const uint8_t flags)
    {
        GameCommands::setExpenditureType(ExpenditureType::LoanInterest);

        auto* company = CompanyManager::get(_updatingCompanyId);
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
            const auto maxLoan = Economy::getInflationAdjustedCost(maxLoanSize, 0, 8);
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
            Ui::WindowManager::invalidate(Ui::WindowType::company, static_cast<uint16_t>(*_updatingCompanyId));
            if (CompanyManager::getControllingId() == _updatingCompanyId)
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
