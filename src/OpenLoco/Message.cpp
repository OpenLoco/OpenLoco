#include "Message.h"
#include "Audio/Audio.h"

namespace OpenLoco
{
    // 0x004F8BE4
    static constexpr uint16_t _MessageTypeFlagsArr[31] = {
        MessageTypeFlags::unk0 | MessageTypeFlags::hasFirstItem | MessageTypeFlags::hasSecondItem,
        MessageTypeFlags::unk0 | MessageTypeFlags::unk1 | MessageTypeFlags::hasFirstItem,
        MessageTypeFlags::unk0 | MessageTypeFlags::hasFirstItem,
        MessageTypeFlags::unk0 | MessageTypeFlags::unk1 | MessageTypeFlags::hasFirstItem | MessageTypeFlags::hasSecondItem,
        MessageTypeFlags::unk0 | MessageTypeFlags::unk1 | MessageTypeFlags::hasFirstItem | MessageTypeFlags::hasSecondItem,
        MessageTypeFlags::unk0 | MessageTypeFlags::unk1 | MessageTypeFlags::hasFirstItem | MessageTypeFlags::hasSecondItem,
        MessageTypeFlags::unk0 | MessageTypeFlags::unk1 | MessageTypeFlags::hasFirstItem | MessageTypeFlags::hasSecondItem,
        MessageTypeFlags::unk0 | MessageTypeFlags::unk1 | MessageTypeFlags::hasFirstItem | MessageTypeFlags::hasSecondItem,
        MessageTypeFlags::unk0 | MessageTypeFlags::unk1 | MessageTypeFlags::hasFirstItem | MessageTypeFlags::hasSecondItem,
        MessageTypeFlags::hasFirstItem,
        MessageTypeFlags::hasFirstItem,
        MessageTypeFlags::unk1 | MessageTypeFlags::hasFirstItem | MessageTypeFlags::hasSecondItem,
        MessageTypeFlags::unk0 | MessageTypeFlags::hasFirstItem | MessageTypeFlags::hasSecondItem,
        MessageTypeFlags::unk0 | MessageTypeFlags::unk1 | MessageTypeFlags::hasFirstItem | MessageTypeFlags::hasSecondItem,
        MessageTypeFlags::unk0 | MessageTypeFlags::unk1 | MessageTypeFlags::hasFirstItem | MessageTypeFlags::hasSecondItem,
        MessageTypeFlags::unk1 | MessageTypeFlags::hasFirstItem,
        MessageTypeFlags::unk1 | MessageTypeFlags::hasFirstItem,
        MessageTypeFlags::unk0 | MessageTypeFlags::unk1 | MessageTypeFlags::hasFirstItem,
        MessageTypeFlags::unk1 | MessageTypeFlags::hasFirstItem,
        MessageTypeFlags::unk1 | MessageTypeFlags::hasFirstItem,
        MessageTypeFlags::unk1 | MessageTypeFlags::hasFirstItem,
        MessageTypeFlags::unk1 | MessageTypeFlags::hasFirstItem,
        MessageTypeFlags::unk1 | MessageTypeFlags::hasFirstItem,
        MessageTypeFlags::unk1 | MessageTypeFlags::hasFirstItem,
        MessageTypeFlags::unk1 | MessageTypeFlags::hasFirstItem,
        MessageTypeFlags::unk1 | MessageTypeFlags::hasFirstItem,
        MessageTypeFlags::unk1 | MessageTypeFlags::hasFirstItem,
        MessageTypeFlags::unk0 | MessageTypeFlags::unk1 | MessageTypeFlags::hasFirstItem,
        MessageTypeFlags::hasSecondItem,
        MessageTypeFlags::unk1 | MessageTypeFlags::hasFirstItem | MessageTypeFlags::hasSecondItem,
        MessageTypeFlags::unk1 | MessageTypeFlags::hasFirstItem | MessageTypeFlags::hasSecondItem,
    };

    bool hasMessageTypeFlag(const MessageType type, const uint16_t flag)
    {
        return _MessageTypeFlagsArr[enumValue(type)] & flag;
    }

    // 0x004F8B08
    static constexpr MessageItemType _MessageItemArgumentType[31]{
        { MessageItemArgumentType::vehicle, MessageItemArgumentType::station, MessageItemArgumentType::null },
        { MessageItemArgumentType::industry, MessageItemArgumentType::null, MessageItemArgumentType::null },
        { MessageItemArgumentType::vehicle, MessageItemArgumentType::null, MessageItemArgumentType::null },
        { MessageItemArgumentType::vehicle, MessageItemArgumentType::station, MessageItemArgumentType::null },
        { MessageItemArgumentType::vehicle, MessageItemArgumentType::station, MessageItemArgumentType::null },
        { MessageItemArgumentType::vehicle, MessageItemArgumentType::station, MessageItemArgumentType::null },
        { MessageItemArgumentType::vehicle, MessageItemArgumentType::station, MessageItemArgumentType::null },
        { MessageItemArgumentType::vehicle, MessageItemArgumentType::station, MessageItemArgumentType::null },
        { MessageItemArgumentType::vehicle, MessageItemArgumentType::station, MessageItemArgumentType::null },
        { MessageItemArgumentType::station, MessageItemArgumentType::null, MessageItemArgumentType::null },
        { MessageItemArgumentType::station, MessageItemArgumentType::null, MessageItemArgumentType::null },
        { MessageItemArgumentType::company, MessageItemArgumentType::town, MessageItemArgumentType::null },
        { MessageItemArgumentType::vehicle, MessageItemArgumentType::station, MessageItemArgumentType::null },
        { MessageItemArgumentType::vehicle, MessageItemArgumentType::town, MessageItemArgumentType::town },
        { MessageItemArgumentType::vehicle, MessageItemArgumentType::industry, MessageItemArgumentType::industry },
        { MessageItemArgumentType::vehicleTab, MessageItemArgumentType::null, MessageItemArgumentType::null },
        { MessageItemArgumentType::company, MessageItemArgumentType::null, MessageItemArgumentType::null },
        { MessageItemArgumentType::industry, MessageItemArgumentType::null, MessageItemArgumentType::null },
        { MessageItemArgumentType::industry, MessageItemArgumentType::null, MessageItemArgumentType::null },
        { MessageItemArgumentType::industry, MessageItemArgumentType::null, MessageItemArgumentType::null },
        { MessageItemArgumentType::company, MessageItemArgumentType::null, MessageItemArgumentType::null },
        { MessageItemArgumentType::company, MessageItemArgumentType::null, MessageItemArgumentType::null },
        { MessageItemArgumentType::company, MessageItemArgumentType::null, MessageItemArgumentType::null },
        { MessageItemArgumentType::company, MessageItemArgumentType::null, MessageItemArgumentType::null },
        { MessageItemArgumentType::company, MessageItemArgumentType::null, MessageItemArgumentType::null },
        { MessageItemArgumentType::company, MessageItemArgumentType::null, MessageItemArgumentType::null },
        { MessageItemArgumentType::company, MessageItemArgumentType::null, MessageItemArgumentType::null },
        { MessageItemArgumentType::vehicle, MessageItemArgumentType::null, MessageItemArgumentType::null },
        { MessageItemArgumentType::company, MessageItemArgumentType::null, MessageItemArgumentType::null },
        { MessageItemArgumentType::vehicle, MessageItemArgumentType::company, MessageItemArgumentType::null },
        { MessageItemArgumentType::vehicle, MessageItemArgumentType::company, MessageItemArgumentType::null },
    };

    const MessageItemType& getMessageItemArgumentType(const MessageType type)
    {
        return _MessageItemArgumentType[enumValue(type)];
    }

    // 0x004F8C22
    static constexpr MessageCriticality messageCriticalities[31] = {
        MessageCriticality::advice,
        MessageCriticality::general,
        MessageCriticality::advice,
        MessageCriticality::minorCompany,
        MessageCriticality::minorCompany,
        MessageCriticality::minorCompany,
        MessageCriticality::minorCompany,
        MessageCriticality::minorCompany,
        MessageCriticality::minorCompany,
        MessageCriticality::advice,
        MessageCriticality::advice,
        MessageCriticality::majorCompany,
        MessageCriticality::advice,
        MessageCriticality::minorCompany,
        MessageCriticality::minorCompany,
        MessageCriticality::general,
        MessageCriticality::majorCompany,
        MessageCriticality::general,
        MessageCriticality::general,
        MessageCriticality::general,
        MessageCriticality::majorCompany,
        MessageCriticality::majorCompany,
        MessageCriticality::majorCompany,
        MessageCriticality::majorCompany,
        MessageCriticality::majorCompany,
        MessageCriticality::majorCompany,
        MessageCriticality::majorCompetitor,
        MessageCriticality::majorCompany,
        MessageCriticality::majorCompany,
        MessageCriticality::majorCompany,
        MessageCriticality::majorCompetitor,
    };

    MessageCriticality getMessageCriticality(const MessageType type)
    {
        return messageCriticalities[enumValue(type)];
    }

    // 0x004F8C41
    constexpr static Audio::SoundId messageSounds[31] = {
        Audio::SoundId::notification,
        Audio::SoundId::newsAwww,
        Audio::SoundId::notification,
        Audio::SoundId::notification,
        Audio::SoundId::notification,
        Audio::SoundId::notification,
        Audio::SoundId::notification,
        Audio::SoundId::notification,
        Audio::SoundId::notification,
        Audio::SoundId::notification,
        Audio::SoundId::notification,
        Audio::SoundId::notification,
        Audio::SoundId::notification,
        Audio::SoundId::applause2,
        Audio::SoundId::applause2,
        Audio::SoundId::newsOooh,
        Audio::SoundId::applause2,
        Audio::SoundId::newsOooh,
        Audio::SoundId::newsOooh,
        Audio::SoundId::newsAwww,
        Audio::SoundId::applause2,
        Audio::SoundId::newsAwww,
        Audio::SoundId::newsAwww,
        Audio::SoundId::newsAwww,
        Audio::SoundId::newsAwww,
        Audio::SoundId::newsAwww,
        Audio::SoundId::applause2,
        Audio::SoundId::notification,
        Audio::SoundId::newsAwww,
        Audio::SoundId::applause2,
        Audio::SoundId::newsOooh
    };

    Audio::SoundId getMessageSound(const MessageType type)
    {
        return messageSounds[enumValue(type)];
    }
}
