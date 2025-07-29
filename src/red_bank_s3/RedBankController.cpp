
#include "RedBankController.h"

#include "main.h"

namespace RedBankS3
{
    RedBankController::RedBankController()
    {
    }
    RedBankController::~RedBankController()
    {
    }

    void RedBankController::setup()
    {
        // 初始化天线管理器
        AntennaManager::init(config.lora.region);
    }

    void RedBankController::loop()
    {
        // 使用天线管理器处理天线切换
        AntennaManager::switchAntennaForRegion(config.lora.region);

    }


}