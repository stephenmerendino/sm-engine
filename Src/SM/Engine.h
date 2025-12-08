#pragma once

#include "SM/Util.h"

namespace SM
{
    struct EngineConfig
    {
        const char* m_rawAssetsDir = nullptr;
    };

    void Init(const EngineConfig& config);
    void Exit();
    bool ExitRequested();
    const char* GetRawAssetsDir();
    bool IsRunningDebugBuild();

}
