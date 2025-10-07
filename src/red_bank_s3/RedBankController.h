#pragma once

#include <vector>
#include "mesh/generated/meshtastic/mesh.pb.h"
#include "NodeDB.h"

#include "AntennaManager.h"
// variant.h 已经通过 NodeDB.h -> configuration.h 包含，不需要再次包含

namespace RedBankS3
{
    class RedBankController
    {

    public:
        RedBankController();
        ~RedBankController();
        void setup();
        void loop();


    private:

    };
}