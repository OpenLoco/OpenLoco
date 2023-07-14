#include "Debug.h"
#include "Effects/EffectsManager.h"
#include "Entities/EntityManager.h"
#include "Entities/EntityTweener.h"
#include "GameState.h"
#include "Localisation/Formatting.h"
#include "Vehicles/Vehicle.h"
#include "World/IndustryManager.h"

#include <algorithm>
#include <imgui.h>

namespace OpenLoco::Ui::Windows::Debug
{
    static bool _open = true;

    void open()
    {
        _open = true;
    }

    template<typename T, EntityManager::EntityListType TListType>
    auto getSortedEntityList()
    {
        std::vector<T*> ents;
        for (auto* ent : EntityManager::EntityList<EntityManager::EntityListIterator<T>, TListType>())
        {
            ents.push_back(ent);
        }
        std::sort(ents.begin(), ents.end(), [](auto* a, auto* b) {
            return a->id < b->id;
        });
        return ents;
    }

    template<typename T, EntityManager::EntityListType TListType, typename FDraw>
    auto drawEntityList(const char* listName, FDraw&& func)
    {
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        bool listOpen = ImGui::TreeNodeEx(listName, ImGuiTreeNodeFlags_SpanFullWidth);
        ImGui::TableNextColumn();
        ImGui::TextDisabled("");
        if (listOpen)
        {
            for (auto* ent : getSortedEntityList<T, TListType>())
            {
                char nameBuf[256]{};
                StringManager::formatString(nameBuf, ent->name);

                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                bool entityCollapsed = false;
                if (nameBuf[0] != '\0')
                    entityCollapsed = ImGui::TreeNodeEx(ent, ImGuiTreeNodeFlags_SpanFullWidth, "%u - %s", enumValue(ent->id), nameBuf);
                else
                    entityCollapsed = ImGui::TreeNodeEx(ent, ImGuiTreeNodeFlags_SpanFullWidth, "%u", enumValue(ent->id), nameBuf);
                ImGui::TableNextColumn();
                ImGui::TextDisabled("");
                if (entityCollapsed)
                {
                    bool dataChanged = func(ent);
                    if (dataChanged)
                    {
                        EntityTweener::get().removeEntity(ent);
                        ent->invalidateSprite();
                    }
                    ImGui::TreePop();
                }
            }
            ImGui::TreePop();
        }
    }

    template<typename TId, typename TVar>
    static bool drawVariable(const char* name, TId id, TVar& var)
    {
        const char s8_one = 1;
        const ImU8 u8_one = 1;
        const short s16_one = 1;
        const ImU16 u16_one = 1;
        const ImS32 s32_one = 1;
        const ImU32 u32_one = 1;

        using TVal = std::decay_t<TVar>;

        char varId[256];
        std::snprintf(varId, std::size(varId), "%s##%u", name, static_cast<uint32_t>(id));

        bool dataChanged = false;
        if constexpr (std::is_same_v<TVal, int8_t>)
            dataChanged = ImGui::InputScalar(varId, ImGuiDataType_S8, &var, &s8_one, "%d");
        else if constexpr (std::is_same_v<TVal, uint8_t>)
            dataChanged = ImGui::InputScalar(varId, ImGuiDataType_U8, &var, &u8_one, "%u");
        else if constexpr (std::is_same_v<TVal, int16_t>)
            dataChanged = ImGui::InputScalar(varId, ImGuiDataType_S16, &var, &s16_one, "%d");
        else if constexpr (std::is_same_v<TVal, uint16_t>)
            dataChanged = ImGui::InputScalar(varId, ImGuiDataType_U16, &var, &u16_one, "%u");
        else if constexpr (std::is_same_v<TVal, int32_t>)
            dataChanged = ImGui::InputScalar(varId, ImGuiDataType_S32, &var, &s32_one, "%d");
        else if constexpr (std::is_same_v<TVal, uint32_t>)
            dataChanged = ImGui::InputScalar(varId, ImGuiDataType_U32, &var, &u32_one, "%u");

        return dataChanged;
    }

    template<typename TEnt, typename TVar>
    static bool drawEntityVariable(const char* name, TEnt* ent, TVar& var)
    {
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::TextDisabled("");
        ImGui::TableNextColumn();
        if constexpr (std::is_enum_v<std::decay_t<TVar>>)
        {
            return drawVariable(name, enumValue(ent->id), reinterpret_cast<std::underlying_type_t<std::decay_t<TVar>>&>(var));
        }
        else
        {
            return drawVariable(name, enumValue(ent->id), var);
        }
    }

    template<typename T>
    static bool drawEntityBase(T* ent)
    {
        bool dataChanged = false;
        dataChanged |= drawEntityVariable("nextQuadrantId", ent, ent->nextQuadrantId);
        dataChanged |= drawEntityVariable("nextThingId", ent, ent->nextThingId);
        dataChanged |= drawEntityVariable("llPreviousId", ent, ent->llPreviousId);
        dataChanged |= drawEntityVariable("linkedListOffset", ent, ent->linkedListOffset);
        dataChanged |= drawEntityVariable("spriteHeightNegative", ent, ent->spriteHeightNegative);
        dataChanged |= drawEntityVariable("id", ent, ent->id);
        dataChanged |= drawEntityVariable("vehicleFlags", ent, ent->vehicleFlags);
        dataChanged |= drawEntityVariable("position.x", ent, ent->position.x);
        dataChanged |= drawEntityVariable("position.y", ent, ent->position.y);
        dataChanged |= drawEntityVariable("position.z", ent, ent->position.z);
        dataChanged |= drawEntityVariable("spriteWidth", ent, ent->spriteWidth);
        dataChanged |= drawEntityVariable("spriteHeightPositive", ent, ent->spriteHeightPositive);
        dataChanged |= drawEntityVariable("spriteLeft", ent, ent->spriteLeft);
        dataChanged |= drawEntityVariable("spriteTop", ent, ent->spriteTop);
        dataChanged |= drawEntityVariable("spriteRight", ent, ent->spriteRight);
        dataChanged |= drawEntityVariable("spriteBottom", ent, ent->spriteBottom);
        dataChanged |= drawEntityVariable("spriteYaw", ent, ent->spriteYaw);
        dataChanged |= drawEntityVariable("spritePitch", ent, ent->spritePitch);
        dataChanged |= drawEntityVariable("pad_20", ent, ent->pad_20);
        dataChanged |= drawEntityVariable("owner", ent, ent->owner);
        dataChanged |= drawEntityVariable("name", ent, ent->name);
        return dataChanged;
    }

    static void drawEntitiesTab()
    {
        constexpr ImGuiTableFlags flags = ImGuiTableFlags_BordersV | ImGuiTableFlags_BordersOuterH | ImGuiTableFlags_Resizable | ImGuiTableFlags_RowBg | ImGuiTableFlags_NoBordersInBody;
        if (ImGui::BeginTable("Entities", 2, flags))
        {
            // The first column will use the default _WidthStretch when ScrollX is Off and _WidthFixed when ScrollX is On
            ImGui::TableSetupColumn("Entity", ImGuiTableColumnFlags_NoHide);
            ImGui::TableSetupColumn("Data", 0);
            ImGui::TableHeadersRow();

            drawEntityList<Vehicles::VehicleHead, EntityManager::EntityListType::vehicleHead>("Vehicle Head", [](Vehicles::VehicleHead* ent) {
                bool dataChanged = false;
                dataChanged |= drawEntityBase(ent);
                dataChanged |= drawEntityVariable("head", ent, ent->head);
                dataChanged |= drawEntityVariable("remainingDistance", ent, ent->remainingDistance);
                dataChanged |= drawEntityVariable("trackAndDirection", ent, ent->trackAndDirection);
                dataChanged |= drawEntityVariable("subPosition", ent, ent->subPosition);
                dataChanged |= drawEntityVariable("tileX", ent, ent->tileX);
                dataChanged |= drawEntityVariable("tileY", ent, ent->tileY);
                dataChanged |= drawEntityVariable("tileBaseZ", ent, ent->tileBaseZ);
                dataChanged |= drawEntityVariable("trackType", ent, ent->trackType);
                dataChanged |= drawEntityVariable("routingHandle", ent, ent->routingHandle);
                dataChanged |= drawEntityVariable("var_38", ent, ent->var_38);
                dataChanged |= drawEntityVariable("pad_39", ent, ent->pad_39);
                dataChanged |= drawEntityVariable("nextCarId", ent, ent->nextCarId);
                dataChanged |= drawEntityVariable("var_3C", ent, ent->var_3C);
                dataChanged |= drawEntityVariable("mode", ent, ent->mode);
                dataChanged |= drawEntityVariable("ordinalNumber", ent, ent->ordinalNumber);
                dataChanged |= drawEntityVariable("orderTableOffset", ent, ent->orderTableOffset);
                dataChanged |= drawEntityVariable("currentOrder", ent, ent->currentOrder);
                dataChanged |= drawEntityVariable("sizeOfOrderTable", ent, ent->sizeOfOrderTable);
                dataChanged |= drawEntityVariable("var_4E", ent, ent->var_4E);
                dataChanged |= drawEntityVariable("var_52", ent, ent->var_52);
                dataChanged |= drawEntityVariable("stationId", ent, ent->stationId);
                dataChanged |= drawEntityVariable("cargoTransferTimeout", ent, ent->cargoTransferTimeout);
                dataChanged |= drawEntityVariable("var_58", ent, ent->var_58);
                dataChanged |= drawEntityVariable("var_5C", ent, ent->var_5C);
                dataChanged |= drawEntityVariable("status", ent, ent->status);
                dataChanged |= drawEntityVariable("vehicleType", ent, ent->vehicleType);
                dataChanged |= drawEntityVariable("breakdownFlags", ent, ent->breakdownFlags);
                dataChanged |= drawEntityVariable("var_60", ent, ent->var_60);
                dataChanged |= drawEntityVariable("var_61", ent, ent->var_61);
                dataChanged |= drawEntityVariable("airportMovementEdge", ent, ent->airportMovementEdge);
                dataChanged |= drawEntityVariable("totalRefundCost", ent, ent->totalRefundCost);
                dataChanged |= drawEntityVariable("var_6E", ent, ent->var_6E);
                dataChanged |= drawEntityVariable("var_6F", ent, ent->var_6F);
                dataChanged |= drawEntityVariable("var_71", ent, ent->var_71);
                dataChanged |= drawEntityVariable("var_73", ent, ent->var_73);
                dataChanged |= drawEntityVariable("lastAverageSpeed", ent, ent->lastAverageSpeed);
                dataChanged |= drawEntityVariable("var_79", ent, ent->var_79);
                return dataChanged;
            });

            drawEntityList<Vehicles::VehicleBase, EntityManager::EntityListType::vehicle>("Vehicle", [](Vehicles::VehicleBase* ent) {
                bool dataChanged = false;
                dataChanged |= drawEntityBase(ent);
                return dataChanged;
            });

            drawEntityList<MiscBase, EntityManager::EntityListType::misc>("Misc", [](auto* ent) {
                bool dataChanged = false;
                dataChanged |= drawEntityBase(ent);
                return dataChanged;
            });

            ImGui::EndTable();
        }
    }

    template<typename FDraw>
    auto drawIndustryList(FDraw&& func)
    {
        for (Industry& industry : IndustryManager::industries())
        {
            char nameBuf[256]{};
            StringManager::formatString(nameBuf, industry.name);

            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            bool industryCollapsed = false;
            if (nameBuf[0] != '\0')
                industryCollapsed = ImGui::TreeNodeEx(&industry, ImGuiTreeNodeFlags_SpanFullWidth, "%u - %s", enumValue(industry.id()), nameBuf);
            else
                industryCollapsed = ImGui::TreeNodeEx(&industry, ImGuiTreeNodeFlags_SpanFullWidth, "%u", enumValue(industry.id()));
            ImGui::TableNextColumn();
            ImGui::TextDisabled("");
            if (industryCollapsed)
            {
                func(industry);
                ImGui::TreePop();
            }
        }
    }

    static void drawIndustriesTab()
    {
        constexpr ImGuiTableFlags flags = ImGuiTableFlags_BordersV | ImGuiTableFlags_BordersOuterH | ImGuiTableFlags_Resizable | ImGuiTableFlags_RowBg | ImGuiTableFlags_NoBordersInBody;
        if (ImGui::BeginTable("Industries", 2, flags))
        {
            // The first column will use the default _WidthStretch when ScrollX is Off and _WidthFixed when ScrollX is On
            ImGui::TableSetupColumn("Industry", ImGuiTableColumnFlags_NoHide);
            ImGui::TableSetupColumn("Data", 0);
            ImGui::TableHeadersRow();

            drawIndustryList([](Industry& industry) {
                const auto id = industry.id();
                drawVariable("name", id, industry.name);
                drawVariable("x", id, industry.x);
                drawVariable("y", id, industry.y);
                drawVariable("flags", id, industry.flags);
                drawVariable("prng", id, industry.prng);
                drawVariable("objectId", id, industry.objectId);
                drawVariable("under_construction", id, industry.under_construction);
                drawVariable("numTiles", id, industry.numTiles);
                for (auto i = 0U; i < industry.numTiles; i++)
                {
                    char tileName[16];
                    sprintf(tileName, "tile[%u]", i);
                    drawVariable(tileName, id, industry.tiles[i]);
                }
                drawVariable("town", id, industry.town);
                // drawVariable("tileLoop.current", industry.tileLoop.current());
                drawVariable("var_DB", id, industry.var_DB);
                drawVariable("var_DD", id, industry.var_DD);
                drawVariable("var_DF", id, industry.var_DF);
                drawVariable("owner", id, industry.owner);
                drawVariable("var_E1", id, industry.var_E1.data());
                // TODO: Better support for arrays.
                drawVariable("productionRate[0]", id, industry.productionRate[0]);
                drawVariable("productionRate[1]", id, industry.productionRate[1]);
                drawVariable("var_17D[0]", id, industry.var_17D[0]);
                drawVariable("var_17D[1]", id, industry.var_17D[1]);
                drawVariable("var_181[0]", id, industry.var_181[0]);
                drawVariable("var_181[1]", id, industry.var_181[1]);
                drawVariable("producedCargoQuantityMonthlyTotal[0]", id, industry.producedCargoQuantityMonthlyTotal[0]);
                drawVariable("producedCargoQuantityMonthlyTotal[1]", id, industry.producedCargoQuantityMonthlyTotal[1]);
                drawVariable("producedCargoQuantityPreviousMonth[0]", id, industry.producedCargoQuantityPreviousMonth[0]);
                drawVariable("producedCargoQuantityPreviousMonth[1]", id, industry.producedCargoQuantityPreviousMonth[1]);
                drawVariable("receivedCargoQuantityMonthlyTotal[0]", id, industry.receivedCargoQuantityMonthlyTotal[0]);
                drawVariable("receivedCargoQuantityMonthlyTotal[1]", id, industry.receivedCargoQuantityMonthlyTotal[1]);
                drawVariable("receivedCargoQuantityMonthlyTotal[2]", id, industry.receivedCargoQuantityMonthlyTotal[2]);
                drawVariable("receivedCargoQuantityPreviousMonth[0]", id, industry.receivedCargoQuantityPreviousMonth[0]);
                drawVariable("receivedCargoQuantityPreviousMonth[1]", id, industry.receivedCargoQuantityPreviousMonth[1]);
                drawVariable("receivedCargoQuantityPreviousMonth[2]", id, industry.receivedCargoQuantityPreviousMonth[2]);
                drawVariable("receivedCargoQuantityDailyTotal[0]", id, industry.receivedCargoQuantityDailyTotal[0]);
                drawVariable("receivedCargoQuantityDailyTotal[1]", id, industry.receivedCargoQuantityDailyTotal[1]);
                drawVariable("receivedCargoQuantityDailyTotal[2]", id, industry.receivedCargoQuantityDailyTotal[2]);
                drawVariable("producedCargoPercentTransportedPreviousMonth[0]", id, industry.producedCargoPercentTransportedPreviousMonth[0]);
                drawVariable("producedCargoPercentTransportedPreviousMonth[1]", id, industry.producedCargoPercentTransportedPreviousMonth[1]);
                drawVariable("producedCargoMonthlyHistorySize[0]", id, industry.producedCargoMonthlyHistorySize[0]);
                drawVariable("producedCargoMonthlyHistorySize[1]", id, industry.producedCargoMonthlyHistorySize[1]);
                drawVariable("producedCargoMonthlyHistory1[0]", id, industry.producedCargoMonthlyHistory1[0]);
                drawVariable("producedCargoMonthlyHistory1[1]", id, industry.producedCargoMonthlyHistory1[1]);
            });

            ImGui::EndTable();
        }
    }

    void draw()
    {
        if (!_open)
            return;

        ImGui::ShowDemoWindow(&_open);

        if (ImGui::Begin("Debug", &_open))
        {
            ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_None;
            if (ImGui::BeginTabBar("Debug Data", tab_bar_flags))
            {
                if (ImGui::BeginTabItem("Entities"))
                {
                    drawEntitiesTab();
                    ImGui::EndTabItem();
                }

                if (ImGui::BeginTabItem("Industries"))
                {
                    drawIndustriesTab();
                    ImGui::EndTabItem();
                }

                ImGui::EndTabBar();
            }
        }
        ImGui::End();
    }

}
