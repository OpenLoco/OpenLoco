#include "CompetitorObject.h"
#include "Drawing/SoftwareDrawingEngine.h"
#include "Graphics/Colour.h"
#include "Graphics/Gfx.h"
#include "Interop/Interop.hpp"
#include "Localisation/FormatArguments.hpp"
#include "Localisation/StringIds.h"

namespace OpenLoco
{
    // TODO: Should only be defined in ObjectSelectionWindow
    static constexpr uint8_t kDescriptionRowHeight = 10;
    static constexpr Ui::Size kObjectPreviewSize = { 112, 112 };

    // 0x00434D5B
    void CompetitorObject::drawPreviewImage(Gfx::RenderTarget& rt, const int16_t x, const int16_t y) const
    {
        auto drawingCtx = Gfx::getDrawingEngine().getDrawingContext();

        drawingCtx.drawRect(rt, 0, 0, kObjectPreviewSize.width, kObjectPreviewSize.height, AdvancedColour(Colour::mutedOrange).inset().u8());

        auto image = Gfx::recolour(images[0], Colour::mutedSeaGreen);
        drawingCtx.drawImage(&rt, x - 32, y - 32, image);
    }

    // 0x00434DA7
    void CompetitorObject::drawDescription(Gfx::RenderTarget& rt, const int16_t x, const int16_t y, [[maybe_unused]] const int16_t width) const
    {
        auto drawingCtx = Gfx::getDrawingEngine().getDrawingContext();
        Ui::Point rowPosition = { x, y };
        {
            auto args = FormatArguments();
            args.push<uint16_t>(intelligence);
            args.push(aiRatingToLevel(intelligence));

            drawingCtx.drawStringLeft(rt, rowPosition.x, rowPosition.y, Colour::black, StringIds::company_details_intelligence, &args);
            rowPosition.y += kDescriptionRowHeight;
        }
        {
            auto args = FormatArguments();
            args.push<uint16_t>(aggressiveness);
            args.push(aiRatingToLevel(aggressiveness));

            drawingCtx.drawStringLeft(rt, rowPosition.x, rowPosition.y, Colour::black, StringIds::company_details_aggressiveness, &args);
            rowPosition.y += kDescriptionRowHeight;
        }
        {
            auto args = FormatArguments();
            args.push<uint16_t>(competitiveness);
            args.push(aiRatingToLevel(competitiveness));

            drawingCtx.drawStringLeft(rt, rowPosition.x, rowPosition.y, Colour::black, StringIds::company_details_competitiveness, &args);
        }
    }

    // 0x00434D24
    bool CompetitorObject::validate() const
    {
        if (!(emotions & (1 << 0)))
        {
            return false;
        }
        if (intelligence < 1 || intelligence > 9)
        {
            return false;
        }
        if (aggressiveness < 1 || aggressiveness > 9)
        {
            return false;
        }
        return competitiveness >= 1 && competitiveness <= 9;
    }

    // 0x00434CA0
    void CompetitorObject::load(const LoadedObjectHandle& handle, stdx::span<const std::byte> data, ObjectManager::DependentObjects*)
    {
        Interop::registers regs;
        regs.esi = Interop::X86Pointer(this);
        regs.ebx = handle.id;
        regs.ecx = enumValue(handle.type);
        Interop::call(0x00434CA0, regs);
    }

    // 0x00434D08
    void CompetitorObject::unload()
    {
        var_00 = 0;
        var_02 = 0;

        std::fill(std::begin(images), std::end(images), 0);
    }

    static std::array<string_id, 10> aiRatingToLevelArray = {
        {
            StringIds::low,
            StringIds::low,
            StringIds::low,
            StringIds::low,
            StringIds::medium,
            StringIds::medium,
            StringIds::medium,
            StringIds::high,
            StringIds::high,
            StringIds::high,
        }
    };

    [[nodiscard]] string_id aiRatingToLevel(const uint8_t rating)
    {
        return aiRatingToLevelArray[std::min(rating, static_cast<uint8_t>(aiRatingToLevelArray.size()))];
    }
}
