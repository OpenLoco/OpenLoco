#include "CompetitorObject.h"
#include "Graphics/Colour.h"
#include "Graphics/Gfx.h"
#include "Graphics/SoftwareDrawingEngine.h"
#include "Localisation/FormatArguments.hpp"
#include "Localisation/StringIds.h"
#include "ObjectImageTable.h"
#include "ObjectStringTable.h"
#include <OpenLoco/Interop/Interop.hpp>

namespace OpenLoco
{
    static loco_global<uint8_t, 0x0112C211> _intelligence;    // return of loadTemporaryObject
    static loco_global<uint8_t, 0x0112C212> _aggressiveness;  // return of loadTemporaryObject
    static loco_global<uint8_t, 0x0112C213> _competitiveness; // return of loadTemporaryObject

    // TODO: Should only be defined in ObjectSelectionWindow
    static constexpr uint8_t kDescriptionRowHeight = 10;
    static constexpr Ui::Size kObjectPreviewSize = { 112, 112 };

    // 0x00434D5B
    void CompetitorObject::drawPreviewImage(Gfx::RenderTarget& rt, const int16_t x, const int16_t y) const
    {
        auto& drawingCtx = Gfx::getDrawingEngine().getDrawingContext();

        drawingCtx.drawRect(rt, 0, 0, kObjectPreviewSize.width, kObjectPreviewSize.height, Colours::getShade(Colour::mutedSeaGreen, 1), Gfx::RectFlags::none);

        auto image = Gfx::recolour(images[0] + 1, Colour::mutedSeaGreen);
        drawingCtx.drawImage(&rt, x - 32, y - 32, image);
    }

    // 0x00434DA7
    void CompetitorObject::drawDescription(Gfx::RenderTarget& rt, const int16_t x, const int16_t y, [[maybe_unused]] const int16_t width) const
    {
        auto& drawingCtx = Gfx::getDrawingEngine().getDrawingContext();
        Ui::Point rowPosition = { x, y };
        {
            FormatArguments args{};
            args.push<uint16_t>(intelligence);
            args.push(aiRatingToLevel(intelligence));

            drawingCtx.drawStringLeft(rt, rowPosition, Colour::black, StringIds::company_details_intelligence, args);
            rowPosition.y += kDescriptionRowHeight;
        }
        {
            FormatArguments args{};
            args.push<uint16_t>(aggressiveness);
            args.push(aiRatingToLevel(aggressiveness));

            drawingCtx.drawStringLeft(rt, rowPosition, Colour::black, StringIds::company_details_aggressiveness, args);
            rowPosition.y += kDescriptionRowHeight;
        }
        {
            FormatArguments args{};
            args.push<uint16_t>(competitiveness);
            args.push(aiRatingToLevel(competitiveness));

            drawingCtx.drawStringLeft(rt, rowPosition, Colour::black, StringIds::company_details_competitiveness, args);
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
    void CompetitorObject::load(const LoadedObjectHandle& handle, std::span<const std::byte> data, ObjectManager::DependentObjects*)
    {
        auto remainingData = data.subspan(sizeof(CompetitorObject));

        // Load object name string
        auto loadString = [&remainingData, &handle](StringId& dst, uint8_t num) {
            auto strRes = ObjectManager::loadStringTable(remainingData, handle, num);
            dst = strRes.str;
            remainingData = remainingData.subspan(strRes.tableLength);
        };

        loadString(name, 0);
        loadString(lastName, 1);

        // Load images
        auto imageRes = ObjectManager::loadImageTable(remainingData);
        const auto baseImageId = imageRes.imageOffset;
        std::fill(std::begin(images), std::end(images), baseImageId);
        for (auto i = 0U, emotionImageOffset = 0U; i < std::size(images); ++i)
        {
            if (emotions & (1 << i))
            {
                images[i] += emotionImageOffset;
                emotionImageOffset += 2;
            }
        }

        // Ensure we've loaded the entire object
        assert(remainingData.size() == imageRes.tableLength);

        // Copy competitor stats to global
        // TODO: Refactor to not pass by global!
        _intelligence = intelligence;
        _aggressiveness = aggressiveness;
        _competitiveness = competitiveness;
    }

    // 0x00434D08
    void CompetitorObject::unload()
    {
        name = 0;
        lastName = 0;

        std::fill(std::begin(images), std::end(images), 0);
    }

    static std::array<StringId, 10> aiRatingToLevelArray = {
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

    [[nodiscard]] StringId aiRatingToLevel(const uint8_t rating)
    {
        return aiRatingToLevelArray[std::min(rating, static_cast<uint8_t>(aiRatingToLevelArray.size()))];
    }
}
