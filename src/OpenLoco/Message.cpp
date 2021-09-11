#include "Message.h"
#include "Audio/Audio.h"

namespace OpenLoco
{
    // clang-format off
    // 0x004F8BE4, 0x004F8B08, 0x004F8C22, 0x004F8C41
    static constexpr MessageTypeDescriptor _messageTypeDescriptors[31] = {
        /* cantWaitForFullLoad      */ { MessageCriticality::advice,          Audio::SoundId::notification, MessageTypeFlags::unk0 | MessageTypeFlags::hasFirstItem | MessageTypeFlags::hasSecondItem,                          { MessageItemArgumentType::vehicle, MessageItemArgumentType::station, MessageItemArgumentType::null }      },
        /* industryClosingDown      */ { MessageCriticality::general,         Audio::SoundId::newsAwww,     MessageTypeFlags::unk0 | MessageTypeFlags::unk1 | MessageTypeFlags::hasFirstItem,                                   { MessageItemArgumentType::industry, MessageItemArgumentType::null, MessageItemArgumentType::null }        },
        /*                          */ { MessageCriticality::advice,          Audio::SoundId::notification, MessageTypeFlags::unk0 | MessageTypeFlags::hasFirstItem,                                                            { MessageItemArgumentType::vehicle, MessageItemArgumentType::null, MessageItemArgumentType::null }         },
        /*                          */ { MessageCriticality::minorCompany,    Audio::SoundId::notification, MessageTypeFlags::unk0 | MessageTypeFlags::unk1 | MessageTypeFlags::hasFirstItem | MessageTypeFlags::hasSecondItem, { MessageItemArgumentType::vehicle, MessageItemArgumentType::station, MessageItemArgumentType::null }      },
        /*                          */ { MessageCriticality::minorCompany,    Audio::SoundId::notification, MessageTypeFlags::unk0 | MessageTypeFlags::unk1 | MessageTypeFlags::hasFirstItem | MessageTypeFlags::hasSecondItem, { MessageItemArgumentType::vehicle, MessageItemArgumentType::station, MessageItemArgumentType::null }      },
        /*                          */ { MessageCriticality::minorCompany,    Audio::SoundId::notification, MessageTypeFlags::unk0 | MessageTypeFlags::unk1 | MessageTypeFlags::hasFirstItem | MessageTypeFlags::hasSecondItem, { MessageItemArgumentType::vehicle, MessageItemArgumentType::station, MessageItemArgumentType::null }      },
        /*                          */ { MessageCriticality::minorCompany,    Audio::SoundId::notification, MessageTypeFlags::unk0 | MessageTypeFlags::unk1 | MessageTypeFlags::hasFirstItem | MessageTypeFlags::hasSecondItem, { MessageItemArgumentType::vehicle, MessageItemArgumentType::station, MessageItemArgumentType::null }      },
        /*                          */ { MessageCriticality::minorCompany,    Audio::SoundId::notification, MessageTypeFlags::unk0 | MessageTypeFlags::unk1 | MessageTypeFlags::hasFirstItem | MessageTypeFlags::hasSecondItem, { MessageItemArgumentType::vehicle, MessageItemArgumentType::station, MessageItemArgumentType::null }      },
        /*                          */ { MessageCriticality::minorCompany,    Audio::SoundId::notification, MessageTypeFlags::unk0 | MessageTypeFlags::unk1 | MessageTypeFlags::hasFirstItem | MessageTypeFlags::hasSecondItem, { MessageItemArgumentType::vehicle, MessageItemArgumentType::station, MessageItemArgumentType::null }      },
        /* cargoNowAccepted         */ { MessageCriticality::advice,          Audio::SoundId::notification, MessageTypeFlags::hasFirstItem,                                                                                     { MessageItemArgumentType::station, MessageItemArgumentType::null, MessageItemArgumentType::null }         },
        /* cargoNoLongerAccepted    */ { MessageCriticality::advice,          Audio::SoundId::notification, MessageTypeFlags::hasFirstItem,                                                                                     { MessageItemArgumentType::station, MessageItemArgumentType::null, MessageItemArgumentType::null }         },
        /* newCompany               */ { MessageCriticality::majorCompany,    Audio::SoundId::notification, MessageTypeFlags::unk1 | MessageTypeFlags::hasFirstItem | MessageTypeFlags::hasSecondItem,                          { MessageItemArgumentType::company, MessageItemArgumentType::town, MessageItemArgumentType::null }         },
        /* unableToLandAtAirport    */ { MessageCriticality::advice,          Audio::SoundId::notification, MessageTypeFlags::unk0 | MessageTypeFlags::hasFirstItem | MessageTypeFlags::hasSecondItem,                          { MessageItemArgumentType::vehicle, MessageItemArgumentType::station, MessageItemArgumentType::null }      },
        /* citizensCelebrate        */ { MessageCriticality::minorCompany,    Audio::SoundId::applause2,    MessageTypeFlags::unk0 | MessageTypeFlags::unk1 | MessageTypeFlags::hasFirstItem | MessageTypeFlags::hasSecondItem, { MessageItemArgumentType::vehicle, MessageItemArgumentType::town, MessageItemArgumentType::town }         },
        /* workersCelebrate         */ { MessageCriticality::minorCompany,    Audio::SoundId::applause2,    MessageTypeFlags::unk0 | MessageTypeFlags::unk1 | MessageTypeFlags::hasFirstItem | MessageTypeFlags::hasSecondItem, { MessageItemArgumentType::vehicle, MessageItemArgumentType::industry, MessageItemArgumentType::industry } }, 
        /* newVehicle               */ { MessageCriticality::general,         Audio::SoundId::newsOooh,     MessageTypeFlags::unk1 | MessageTypeFlags::hasFirstItem,                                                            { MessageItemArgumentType::vehicleTab, MessageItemArgumentType::null, MessageItemArgumentType::null }      },
        /*                          */ { MessageCriticality::majorCompany,    Audio::SoundId::applause2,    MessageTypeFlags::unk1 | MessageTypeFlags::hasFirstItem,                                                            { MessageItemArgumentType::company, MessageItemArgumentType::null, MessageItemArgumentType::null }         },
        /* newIndustry              */ { MessageCriticality::general,         Audio::SoundId::newsOooh,     MessageTypeFlags::unk0 | MessageTypeFlags::unk1 | MessageTypeFlags::hasFirstItem,                                   { MessageItemArgumentType::industry, MessageItemArgumentType::null, MessageItemArgumentType::null }        },
        /* industryProductionUp     */ { MessageCriticality::general,         Audio::SoundId::newsOooh,     MessageTypeFlags::unk1 | MessageTypeFlags::hasFirstItem,                                                            { MessageItemArgumentType::industry, MessageItemArgumentType::null, MessageItemArgumentType::null }        },
        /* industryProductionDown   */ { MessageCriticality::general,         Audio::SoundId::newsAwww,     MessageTypeFlags::unk1 | MessageTypeFlags::hasFirstItem,                                                            { MessageItemArgumentType::industry, MessageItemArgumentType::null, MessageItemArgumentType::null }        },
        /*                          */ { MessageCriticality::majorCompany,    Audio::SoundId::applause2,    MessageTypeFlags::unk1 | MessageTypeFlags::hasFirstItem,                                                            { MessageItemArgumentType::company, MessageItemArgumentType::null, MessageItemArgumentType::null }         },
        /*                          */ { MessageCriticality::majorCompany,    Audio::SoundId::newsAwww,     MessageTypeFlags::unk1 | MessageTypeFlags::hasFirstItem,                                                            { MessageItemArgumentType::company, MessageItemArgumentType::null, MessageItemArgumentType::null }         },
        /*                          */ { MessageCriticality::majorCompany,    Audio::SoundId::newsAwww,     MessageTypeFlags::unk1 | MessageTypeFlags::hasFirstItem,                                                            { MessageItemArgumentType::company, MessageItemArgumentType::null, MessageItemArgumentType::null }         },
        /* bankruptcyWarning6Months */ { MessageCriticality::majorCompany,    Audio::SoundId::newsAwww,     MessageTypeFlags::unk1 | MessageTypeFlags::hasFirstItem,                                                            { MessageItemArgumentType::company, MessageItemArgumentType::null, MessageItemArgumentType::null }         },
        /* bankruptcyWarning3Months */ { MessageCriticality::majorCompany,    Audio::SoundId::newsAwww,     MessageTypeFlags::unk1 | MessageTypeFlags::hasFirstItem,                                                            { MessageItemArgumentType::company, MessageItemArgumentType::null, MessageItemArgumentType::null }         },
        /* bankruptcyDeclared       */ { MessageCriticality::majorCompany,    Audio::SoundId::newsAwww,     MessageTypeFlags::unk1 | MessageTypeFlags::hasFirstItem,                                                            { MessageItemArgumentType::company, MessageItemArgumentType::null, MessageItemArgumentType::null }         },
        /*                          */ { MessageCriticality::majorCompetitor, Audio::SoundId::applause2,    MessageTypeFlags::unk1 | MessageTypeFlags::hasFirstItem,                                                            { MessageItemArgumentType::company, MessageItemArgumentType::null, MessageItemArgumentType::null }         },
        /* vehicleCrashed           */ { MessageCriticality::majorCompany,    Audio::SoundId::notification, MessageTypeFlags::unk0 | MessageTypeFlags::unk1 | MessageTypeFlags::hasFirstItem,                                   { MessageItemArgumentType::vehicle, MessageItemArgumentType::null, MessageItemArgumentType::null }         },
        /*                          */ { MessageCriticality::majorCompany,    Audio::SoundId::newsAwww,     MessageTypeFlags::hasSecondItem,                                                                                    { MessageItemArgumentType::company, MessageItemArgumentType::null, MessageItemArgumentType::null }         },
        /* newSpeedRecord           */ { MessageCriticality::majorCompany,    Audio::SoundId::applause2,    MessageTypeFlags::unk1 | MessageTypeFlags::hasFirstItem | MessageTypeFlags::hasSecondItem,                          { MessageItemArgumentType::vehicle, MessageItemArgumentType::company, MessageItemArgumentType::null }      },
        /*                          */ { MessageCriticality::majorCompetitor, Audio::SoundId::newsOooh,     MessageTypeFlags::unk1 | MessageTypeFlags::hasFirstItem | MessageTypeFlags::hasSecondItem,                          { MessageItemArgumentType::vehicle, MessageItemArgumentType::company, MessageItemArgumentType::null }      },
    };
    // clang-format on

    const MessageTypeDescriptor& getMessageTypeDescriptor(const MessageType type)
    {
        return _messageTypeDescriptors[enumValue(type)];
    }
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
