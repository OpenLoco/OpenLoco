#include "MessageManager.h"
#include "CompanyManager.h"
#include "Config.h"
#include "Date.h"
#include "Entities/EntityManager.h"
#include "GameState.h"
#include "IndustryManager.h"
#include "Interop/Interop.hpp"
#include "Localisation/FormatArguments.hpp"
#include "Objects/CargoObject.h"
#include "Objects/IndustryObject.h"
#include "OpenLoco.h"
#include "StationManager.h"
#include "TownManager.h"
#include "Ui/WindowManager.h"
#include "Vehicles/Vehicle.h"

using namespace OpenLoco::Interop;

namespace OpenLoco::MessageManager
{
    static void remove(const MessageId id);

    static auto& rawMessages() { return getGameState().messages; }
    static auto& numMessages() { return getGameState().numMessages; }

    Message* get(MessageId id)
    {
        if (enumValue(id) >= Limits::kMaxMessages)
        {
            return nullptr;
        }
        return &rawMessages()[enumValue(id)];
    }

    MessageId getActiveIndex() { return getGameState().activeMessageIndex; }
    void setActiveIndex(const MessageId newIndex) { getGameState().activeMessageIndex = newIndex; }

    void post(
        MessageType type,
        CompanyId companyId,
        uint16_t subjectIdA,
        uint16_t subjectIdB,
        uint16_t subjectIdC)
    {
        Message newMessage{};
        newMessage.companyId = companyId;
        newMessage.type = type;
        newMessage.date = getCurrentDay();
        newMessage.itemSubjects[0] = subjectIdA;
        newMessage.itemSubjects[1] = subjectIdB;
        newMessage.itemSubjects[2] = subjectIdC;

        if (getMessageTypeDescriptor(type).hasFlag(MessageTypeFlags::unk0))
        {
            for (auto i = 0; i < numMessages(); ++i)
            {
                auto& message = rawMessages()[i];
                if (message == newMessage)
                {
                    return;
                }
            }
        }
        if (numMessages() > Limits::kMaxMessages)
        {
            MessageId oldestMessage = MessageId::null;
            int32_t oldest = -1;
            for (auto i = 0; i < numMessages(); ++i)
            {
                auto& message = rawMessages()[i];
                if (message.timeActive != 0)
                {
                    const int32_t age = getMessageTypeDescriptor(message.type).duration + message.date - getCurrentDay();
                    if (age > oldest)
                    {
                        oldest = age;
                        oldestMessage = static_cast<MessageId>(i);
                    }
                }
            }
            // Nothing found so now search for messages that are active
            if (oldest == -1)
            {
                for (auto i = 0; i < numMessages(); ++i)
                {
                    auto& message = rawMessages()[i];

                    const int32_t age = getMessageTypeDescriptor(message.type).duration + message.date - getCurrentDay();
                    if (age > oldest)
                    {
                        oldest = age;
                        oldestMessage = static_cast<MessageId>(i);
                    }
                }
            }
            remove(oldestMessage);
        }
        rawMessages()[numMessages()] = newMessage;
        auto& message = rawMessages()[numMessages()];
        numMessages()++;
        // A buffer that is larger than message.messageString
        char tempBuffer[512]{};
        switch (message.type)
        {
            case MessageType::cantWaitForFullLoad:
            {
                // 0x0042870F
                FormatArguments args{};
                auto* head = EntityManager::get<Vehicles::VehicleHead>(static_cast<EntityId>(message.itemSubjects[0]));
                if (head == nullptr)
                {
                    break;
                }
                args.push(head->ordinalNumber);
                args.push(head->name);

                auto* station = StationManager::get(static_cast<StationId>(message.itemSubjects[1]));
                args.push(station->name);
                args.push(station->town);
                StringManager::formatString(tempBuffer, StringIds::message_not_allowed_to_wait_for_full_load_at, &args);
            }
            break;
            case MessageType::industryClosingDown:
            {
                // 0x00428770
                FormatArguments args{};
                auto* industry = IndustryManager::get(static_cast<IndustryId>(message.itemSubjects[0]));
                args.push(industry->name);
                args.push(industry->town);
                auto* industryObj = industry->getObject();
                StringManager::formatString(tempBuffer, industryObj->nameClosingDown, &args);
            }
            break;
            case MessageType::vehicleSlipped:
            {
                // 0x004287BA
                FormatArguments args{};
                auto* head = EntityManager::get<Vehicles::VehicleHead>(static_cast<EntityId>(message.itemSubjects[0]));
                if (head == nullptr)
                {
                    break;
                }
                args.push(head->ordinalNumber);
                args.push(head->name);
                StringManager::formatString(tempBuffer, StringIds::message_has_slipped_to_a_halt_on_incline, &args);
            }
            break;
            case MessageType::unk3:
            case MessageType::unk4:
            case MessageType::unk5:
            case MessageType::unk6:
            case MessageType::unk7:
            case MessageType::unk8:
                // 0x004287F4 nop
                break;
            case MessageType::cargoNowAccepted:
            {
                // 0x004287FB
                FormatArguments args{};
                auto* station = StationManager::get(static_cast<StationId>(message.itemSubjects[0]));
                args.push(station->name);
                args.push(station->town);
                auto* cargoObj = ObjectManager::get<CargoObject>(message.itemSubjects[1]);
                args.push(cargoObj->name);
                StringManager::formatString(tempBuffer, StringIds::message_now_accepts, &args);
            }
            break;
            case MessageType::cargoNoLongerAccepted:
            {
                // 0x0042884F
                FormatArguments args{};
                auto* station = StationManager::get(static_cast<StationId>(message.itemSubjects[0]));
                args.push(station->name);
                args.push(station->town);
                auto* cargoObj = ObjectManager::get<CargoObject>(message.itemSubjects[1]);
                args.push(cargoObj->name);
                StringManager::formatString(tempBuffer, StringIds::message_no_longer_accepts, &args);
            }
            break;
            case MessageType::newCompany:
            {
                // 0x004288A3
                FormatArguments args{};
                auto* company = CompanyManager::get(static_cast<CompanyId>(message.itemSubjects[0]));
                args.push(company->name);
                args.skip(2);
                auto* town = TownManager::get(static_cast<TownId>(message.itemSubjects[1]));
                args.push(town->name);
                StringManager::formatString(tempBuffer, StringIds::message_new_transport_company, &args);
            }
            break;
            case MessageType::unableToLandAtAirport:
            {
                // 0x004288ED
                FormatArguments args{};
                auto* head = EntityManager::get<Vehicles::VehicleHead>(static_cast<EntityId>(message.itemSubjects[0]));
                if (head == nullptr)
                {
                    break;
                }
                args.push(head->ordinalNumber);
                args.push(head->name);
                auto* station = StationManager::get(static_cast<StationId>(message.itemSubjects[1]));
                args.push(station->name);
                args.push(station->town);
                StringManager::formatString(tempBuffer, StringIds::message_not_able_to_land_at, &args);
            }
            break;
            case MessageType::citizensCelebrate:
            {
                // 0x0042894E
                FormatArguments args{};
                auto* cargoObj = ObjectManager::get<CargoObject>(message.itemSubjects[2] >> 8);
                args.push(cargoObj->name);
                auto* town = TownManager::get(static_cast<TownId>(message.itemSubjects[2] & 0xFF));
                args.push(town->name);

                const auto formatStr = cargoObj->flags & CargoObjectFlags::unk2 ? StringIds::message_first_string_delivery_arives_at : StringIds::message_first_string_arrive_at;
                StringManager::formatString(tempBuffer, formatStr, &args);
            }
            break;
            case MessageType::workersCelebrate:
            {
                // 0x0042899C
                FormatArguments args{};
                auto* cargoObj = ObjectManager::get<CargoObject>(message.itemSubjects[2] >> 8);
                args.push(cargoObj->name);
                auto* industry = IndustryManager::get(static_cast<IndustryId>(message.itemSubjects[2] & 0xFF));
                args.push(industry->name);
                args.push(industry->town);

                StringManager::formatString(tempBuffer, StringIds::message_first_string_delivered_to, &args);
            }
            break;
            case MessageType::newVehicle:
            {
                // 0x004289F0
                FormatArguments args{};
                auto* vehicleObj = ObjectManager::get<VehicleObject>(message.itemSubjects[0]);
                args.push(vehicleObj->name);

                StringManager::formatString(tempBuffer, StringIds::message_new_vehicle_invented, &args);
            }
            break;
            case MessageType::companyPromoted:
            {
                // 0x00428A1D
                FormatArguments args{};
                auto* company = CompanyManager::get(static_cast<CompanyId>(message.itemSubjects[0]));
                args.push(company->ownerName);
                args.skip(2);
                args.push(getCorporateRatingAsStringId(static_cast<CorporateRating>(message.itemSubjects[1] - 1)));
                args.push(getCorporateRatingAsStringId(static_cast<CorporateRating>(message.itemSubjects[1])));
                StringManager::formatString(tempBuffer, StringIds::message_is_promoted_to, &args);
            }
            break;
            case MessageType::newIndustry:
            {
                // 0x00428A66
                FormatArguments args{};
                auto* industry = IndustryManager::get(static_cast<IndustryId>(message.itemSubjects[0]));
                auto* industryObj = industry->getObject();
                args.push(industryObj->name);
                auto* town = TownManager::get(industry->town);
                args.push(town->name);
                StringManager::formatString(tempBuffer, StringIds::message_new_string_under_construction_near, &args);
            }
            break;
            case MessageType::industryProductionUp:
            {
                // 0x00428ABB
                FormatArguments args{};
                auto* industry = IndustryManager::get(static_cast<IndustryId>(message.itemSubjects[0]));
                args.push(industry->name);
                args.push(industry->town);
                auto* industryObj = industry->getObject();
                StringManager::formatString(tempBuffer, industryObj->nameUpProduction, &args);
            }
            break;
            case MessageType::industryProductionDown:
            {
                // 0x00428B05
                FormatArguments args{};
                auto* industry = IndustryManager::get(static_cast<IndustryId>(message.itemSubjects[0]));
                args.push(industry->name);
                args.push(industry->town);
                auto* industryObj = industry->getObject();
                StringManager::formatString(tempBuffer, industryObj->nameDownProduction, &args);
            }
            break;
            case MessageType::congratulationsCompleted:
            {
                // 0x00428B4F
                FormatArguments args{};
                auto* company = CompanyManager::get(static_cast<CompanyId>(message.itemSubjects[0]));
                args.push(company->ownerName);
                StringManager::formatString(tempBuffer, StringIds::message_congratulations_you_have_completed, &args);
            }
            break;
            case MessageType::failedObjectives:
            {
                // 0x00428B7F
                FormatArguments args{};
                auto* company = CompanyManager::get(static_cast<CompanyId>(message.itemSubjects[0]));
                args.push(company->ownerName);
                StringManager::formatString(tempBuffer, StringIds::message_failure_you_have_failed, &args);
            }
            break;
            case MessageType::haveBeenBeaten:
            {
                // 0x00428BAF
                FormatArguments args{};
                auto* company = CompanyManager::get(static_cast<CompanyId>(message.itemSubjects[0]));
                args.push(company->ownerName);
                StringManager::formatString(tempBuffer, StringIds::message_beaten_has_completed, &args);
            }
            break;
            case MessageType::bankruptcyWarning6Months:
            {
                // 0x00428BDF
                FormatArguments args{};
                auto* company = CompanyManager::get(static_cast<CompanyId>(message.itemSubjects[0]));
                args.push(company->name);
                StringManager::formatString(tempBuffer, StringIds::message_bankruptcy_warning_6_month, &args);
            }
            break;
            case MessageType::bankruptcyWarning3Months:
            {
                // 0x00428C0F
                FormatArguments args{};
                auto* company = CompanyManager::get(static_cast<CompanyId>(message.itemSubjects[0]));
                args.push(company->name);
                StringManager::formatString(tempBuffer, StringIds::message_bankruptcy_warning_3_month, &args);
            }
            break;
            case MessageType::bankruptcyDeclared:
            {
                // 0x00428C3F
                FormatArguments args{};
                auto* company = CompanyManager::get(static_cast<CompanyId>(message.itemSubjects[0]));
                args.push(company->name);
                StringManager::formatString(tempBuffer, StringIds::message_bankrupt, &args);
            }
            break;
            case MessageType::bankruptcyDeclared2:
            {
                // 0x00428C6F
                FormatArguments args{};
                auto* company = CompanyManager::get(static_cast<CompanyId>(message.itemSubjects[0]));
                args.push(company->name);
                StringManager::formatString(tempBuffer, StringIds::message_bankrupt_2, &args);
            }
            break;
            case MessageType::vehicleCrashed:
            {
                // 0x00428C9F
                FormatArguments args{};
                auto* head = EntityManager::get<Vehicles::VehicleHead>(static_cast<EntityId>(message.itemSubjects[0]));
                if (head == nullptr)
                {
                    break;
                }
                args.push(head->ordinalNumber);
                args.push(head->name);
                StringManager::formatString(tempBuffer, StringIds::message_has_crashed, &args);
            }
            break;
            case MessageType::companyCheated:
            {
                // 0x00428CD9
                FormatArguments args{};
                auto* company = CompanyManager::get(static_cast<CompanyId>(message.itemSubjects[0]));
                args.push(company->ownerName);
                StringManager::formatString(tempBuffer, StringIds::message_corporate_scandal, &args);
            }
            break;
            case MessageType::newSpeedRecord:
            case MessageType::newSpeedRecord2:
            {
                // 0x00428D06
                FormatArguments args{};
                static constexpr string_id recordTypeStrings[3] = {
                    StringIds::land,
                    StringIds::air,
                    StringIds::water,
                };
                static constexpr string_id vehicleTypeStrings[6] = {
                    StringIds::train_2,
                    StringIds::bus_2,
                    StringIds::truck_2,
                    StringIds::tram_2,
                    StringIds::aircraft_2,
                    StringIds::ship_2,
                };
                auto* head = EntityManager::get<Vehicles::VehicleHead>(static_cast<EntityId>(message.itemSubjects[0]));
                if (head == nullptr)
                {
                    break;
                }
                args.push(recordTypeStrings[message.itemSubjects[2]]);
                args.push(vehicleTypeStrings[enumValue(head->vehicleType)]);
                args.push(getGameState().recordSpeed[message.itemSubjects[2]]);
                StringManager::formatString(tempBuffer, StringIds::message_new_speed_record, &args);
            }
            break;
        }

        std::copy_n(std::begin(tempBuffer), std::size(message.messageString) - 1, std::begin(message.messageString));
        // I don't quite see the significance of 0xFF as its not a control code but for now just follow vanilla
        for (auto i = std::size(message.messageString) - 3; i < std::size(message.messageString) - 1; ++i)
        {
            if (message.messageString[i] == '\xFF')
            {
                message.messageString[i] = '\0';
                break;
            }
        }
        Ui::WindowManager::invalidate(Ui::WindowType::messages);
    }

    // 0x00428E0F
    void clearActiveMessage()
    {
        auto* message = get(getGameState().activeMessageIndex);
        if (message != nullptr)
        {
            message->setActive(false);
        }
        Ui::WindowManager::close(Ui::WindowType::news);
        getGameState().activeMessageIndex = MessageId::null;
    }

    // 0x00428DAB
    static void remove(const MessageId id)
    {
        if (getGameState().activeMessageIndex != MessageId::null)
        {
            if (id == getGameState().activeMessageIndex)
            {
                clearActiveMessage();
            }
            else if (enumValue(id) < enumValue(getGameState().activeMessageIndex))
            {
                getGameState().activeMessageIndex = static_cast<MessageId>(enumValue(getGameState().activeMessageIndex) - 1);
            }
        }
        numMessages()--;
        // Move element to end of array (this seems excessive you could just move to end of numMessages)
        if (enumValue(id) < Limits::kMaxMessages - 1)
        {
            std::rotate(get(id), get(id) + 1, std::end(rawMessages()));
        }
        Ui::WindowManager::invalidate(Ui::WindowType::messages);
    }

    // 0x0042851C
    void removeAllSubjectRefs(const uint16_t subject, MessageItemArgumentType type)
    {
        for (auto i = numMessages(); i > 0; --i)
        {
            auto& message = rawMessages()[i];
            const auto& descriptor = getMessageTypeDescriptor(message.type);
            bool hasModified = false;
            for (auto j = 0; j < Message::kNumSubjects; ++j)
            {
                if (descriptor.argumentTypes[j] == type
                    && message.itemSubjects[j] == subject)
                {
                    message.itemSubjects[j] = 0xFFFF;
                    hasModified = true;
                }
            }
            if (hasModified && getGameState().activeMessageIndex == static_cast<MessageId>(i))
            {
                Ui::WindowManager::invalidate(Ui::WindowType::news);
            }
        }
    }

    // 0x004284DB
    void updateDaily()
    {
        for (auto i = numMessages() - 1; i >= 0; --i)
        {
            auto& message = rawMessages()[i];
            if (getCurrentDay() >= message.date + getMessageTypeDescriptor(message.type).duration)
            {
                remove(static_cast<MessageId>(i));
            }
        }
    }

    // 0x00428F38
    static MessageId getActiveHighestPriority()
    {
        uint8_t highestPriority = 0;
        MessageId bestId = MessageId::null;

        for (auto i = 0; i < numMessages(); ++i)
        {
            auto& message = rawMessages()[i];
            if (!message.isActive())
            {
                continue;
            }

            const auto priority = message.isUserSelected() ? std::numeric_limits<uint8_t>::max()
                                                           : getMessageTypeDescriptor(message.type).priority;

            if (priority > highestPriority)
            {
                highestPriority = priority;
                bestId = static_cast<MessageId>(i);
            }
        }
        return bestId;
    }

    // 0x00428E47
    void sub_428E47()
    {
        if (isTitleMode())
        {
            return;
        }

        auto* newsWnd = Ui::WindowManager::find(Ui::WindowType::news);
        if (newsWnd == nullptr)
        {
            const auto messageId = getActiveHighestPriority();
            if (messageId != MessageId::null)
            {
                Ui::Windows::NewsWindow::open(messageId);
            }
        }
        else
        {
            if (getActiveIndex() == MessageId::null)
            {
                clearActiveMessage();
                return;
            }
            const auto messageId = getActiveHighestPriority();
            if (messageId != MessageId::null && messageId != getActiveIndex())
            {
                Ui::WindowManager::close(Ui::WindowType::news);
                setActiveIndex(MessageId::null);
                Ui::Windows::NewsWindow::open(messageId);
                return;
            }

            auto* message = get(messageId);
            if (message == nullptr)
            {
                return;
            }
            message->timeActive++;

            const auto numWaitingMessages = numMessages() - enumValue(getActiveIndex());
            uint16_t time = 0;
            switch (numWaitingMessages)
            {
                case 0:
                    time = 480;
                    break;
                case 1:
                case 2:
                    time = 384;
                    break;
                case 3:
                case 4:
                    time = 320;
                    break;
                default:
                    time = 288;
                    break;
            }
            uint16_t time2 = std::numeric_limits<uint16_t>::max();
            if (getGameSpeed() != GameSpeed::Normal)
            {
                if (numWaitingMessages > 0)
                {
                    if (numWaitingMessages <= 3)
                    {
                        time2 = 256;
                    }
                    else
                    {
                        if (Config::get().newsSettings[enumValue(getMessageTypeDescriptor(message->type).criticality)] == Config::NewsType::newsWindow)
                        {
                            time2 = 128;
                        }
                    }
                }
            }
            time = std::min(time, time2);
            if (time > (message->timeActive & 0x7FFF))
            {
                return;
            }
            clearActiveMessage();
        }
    }
}
