#pragma once

#include <vector>
#include "mesh/generated/meshtastic/mesh.pb.h"
#include "NodeDB.h"

#include "AntennaManager.h"
#include "../variants/red_bank_s3/variant.h"

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