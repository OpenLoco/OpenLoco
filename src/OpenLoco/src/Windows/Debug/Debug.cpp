#include "Debug.h"
#include "Effects/EffectsManager.h"
#include "Entities/EntityManager.h"
#include "Entities/EntityTweener.h"
#include "GameState.h"
#include "Vehicles/Vehicle.h"

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
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                bool entityOpen = ImGui::TreeNodeEx(ent, ImGuiTreeNodeFlags_SpanFullWidth, "%u", enumValue(ent->id));
                ImGui::TableNextColumn();
                ImGui::TextDisabled("");
                if (entityOpen)
                {
                    func(ent);
                    ImGui::TreePop();
                }
            }
            ImGui::TreePop();
        }
    }

    template<typename TEnt, typename TVar>
    static void drawVariable(const char* name, TEnt* ent, TVar& var)
    {
        const char s8_one = 1;
        const ImU8 u8_one = 1;
        const short s16_one = 1;
        const ImU16 u16_one = 1;
        const ImS32 s32_one = 1;
        const ImU32 u32_one = 1;

        using TVal = std::decay_t<TVar>;

        char varId[256];
        std::snprintf(varId, std::size(varId), "%s##%u", name, enumValue(ent->id));

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

        if (dataChanged)
        {
            EntityTweener::get().removeEntity(ent);
            ent->invalidateSprite();
        }
    }

    template<typename TEnt, typename TVar>
    static void drawDataVariable(const char* name, TEnt* ent, TVar& var)
    {
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::TextDisabled("");
        ImGui::TableNextColumn();

        if constexpr (std::is_enum_v<std::decay_t<TVar>>)
        {
            drawVariable(name, ent, reinterpret_cast<std::underlying_type_t<std::decay_t<TVar>>&>(var));
        }
        else
        {
            drawVariable(name, ent, var);
        }
    }

    template<typename T>
    static void drawEntityBase(T* ent)
    {
        drawDataVariable("nextQuadrantId", ent, ent->nextQuadrantId);
        drawDataVariable("nextThingId", ent, ent->nextThingId);
        drawDataVariable("llPreviousId", ent, ent->llPreviousId);
        drawDataVariable("linkedListOffset", ent, ent->linkedListOffset);
        drawDataVariable("spriteHeightNegative", ent, ent->spriteHeightNegative);
        drawDataVariable("id", ent, ent->id);
        drawDataVariable("vehicleFlags", ent, ent->vehicleFlags);
        drawDataVariable("position.x", ent, ent->position.x);
        drawDataVariable("position.y", ent, ent->position.y);
        drawDataVariable("position.z", ent, ent->position.z);
        drawDataVariable("spriteWidth", ent, ent->spriteWidth);
        drawDataVariable("spriteHeightPositive", ent, ent->spriteHeightPositive);
        drawDataVariable("spriteLeft", ent, ent->spriteLeft);
        drawDataVariable("spriteTop", ent, ent->spriteTop);
        drawDataVariable("spriteRight", ent, ent->spriteRight);
        drawDataVariable("spriteBottom", ent, ent->spriteBottom);
        drawDataVariable("spriteYaw", ent, ent->spriteYaw);
        drawDataVariable("spritePitch", ent, ent->spritePitch);
        drawDataVariable("pad_20", ent, ent->pad_20);
        drawDataVariable("owner", ent, ent->owner);
        drawDataVariable("name", ent, ent->name);
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
                drawEntityBase(ent);
                drawDataVariable("head", ent, ent->head);
                drawDataVariable("remainingDistance", ent, ent->remainingDistance);
                drawDataVariable("trackAndDirection", ent, ent->trackAndDirection);
                drawDataVariable("subPosition", ent, ent->subPosition);
                drawDataVariable("tileX", ent, ent->tileX);
                drawDataVariable("tileY", ent, ent->tileY);
                drawDataVariable("tileBaseZ", ent, ent->tileBaseZ);
                drawDataVariable("trackType", ent, ent->trackType);
                drawDataVariable("routingHandle", ent, ent->routingHandle);
                drawDataVariable("var_38", ent, ent->var_38);
                drawDataVariable("pad_39", ent, ent->pad_39);
                drawDataVariable("nextCarId", ent, ent->nextCarId);
                drawDataVariable("var_3C", ent, ent->var_3C);
                drawDataVariable("mode", ent, ent->mode);
                drawDataVariable("ordinalNumber", ent, ent->ordinalNumber);
                drawDataVariable("orderTableOffset", ent, ent->orderTableOffset);
                drawDataVariable("currentOrder", ent, ent->currentOrder);
                drawDataVariable("sizeOfOrderTable", ent, ent->sizeOfOrderTable);
                drawDataVariable("var_4E", ent, ent->var_4E);
                drawDataVariable("var_52", ent, ent->var_52);
                drawDataVariable("stationId", ent, ent->stationId);
                drawDataVariable("cargoTransferTimeout", ent, ent->cargoTransferTimeout);
                drawDataVariable("var_58", ent, ent->var_58);
                drawDataVariable("var_5C", ent, ent->var_5C);
                drawDataVariable("status", ent, ent->status);
                drawDataVariable("vehicleType", ent, ent->vehicleType);
                drawDataVariable("breakdownFlags", ent, ent->breakdownFlags);
                drawDataVariable("var_60", ent, ent->var_60);
                drawDataVariable("var_61", ent, ent->var_61);
                drawDataVariable("airportMovementEdge", ent, ent->airportMovementEdge);
                drawDataVariable("totalRefundCost", ent, ent->totalRefundCost);
                drawDataVariable("var_6E", ent, ent->var_6E);
                drawDataVariable("var_6F", ent, ent->var_6F);
                drawDataVariable("var_71", ent, ent->var_71);
                drawDataVariable("var_73", ent, ent->var_73);
                drawDataVariable("lastAverageSpeed", ent, ent->lastAverageSpeed);
                drawDataVariable("var_79", ent, ent->var_79);
            });
            drawEntityList<Vehicles::VehicleBase, EntityManager::EntityListType::vehicle>("Vehicle", [](Vehicles::VehicleBase* ent) {
                drawEntityBase(ent);
            });
            drawEntityList<MiscBase, EntityManager::EntityListType::misc>("Misc", [](auto* ent) {
                drawEntityBase(ent);
            });

            ImGui::EndTable();
        }
    }

    static void drawIndustriesTab()
    {
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
