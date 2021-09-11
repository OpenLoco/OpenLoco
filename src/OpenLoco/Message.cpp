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
}
