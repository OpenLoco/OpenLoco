#pragma once
#include "Entities/EntityManager.h"

namespace OpenLoco
{
    // Structure for holding global state information that should generally
    // not be replicated across the network, or (de)serialized on save/reload.
    struct TempState
    {
        EntityId deleteAfterConfirmation; // The entity to be deleted if the player confirms their intent
        bool confirmedEntityDeletion;     // Confirmation that the player intends to delete the above entity
    };

    TempState* GetTempState(); // Returns a pointer to the global TempState instance
    void ResetTempState();     // Reinitializes the global TempState instance (all values 0)
}
