#include "PCH.h"

#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/spdlog.h>

#include "MCM.h"
#include "Settings.h"
#include "WalkSpeedHook.h"

namespace {

    void OnDataLoaded() {
        spdlog::info("[Main] kDataLoaded");
        WalkSpeedTuner::Settings::Load();

        if (auto* task = SKSE::GetTaskInterface()) {
            task->AddTask([] { WalkSpeedTuner::MCM::Register(); });
        } else {
            WalkSpeedTuner::MCM::Register();
        }
    }

    void OnPostLoadGame() {
        spdlog::info("[Main] kPostLoadGame — tickling cache");
        WalkSpeedTuner::WalkSpeedHook::TickleNow();
    }

    void OnMessage(SKSE::MessagingInterface::Message* message) {
        if (!message) return;
        switch (message->type) {
            case SKSE::MessagingInterface::kDataLoaded:
                OnDataLoaded();
                break;
            case SKSE::MessagingInterface::kPostLoadGame:
            case SKSE::MessagingInterface::kNewGame:
                OnPostLoadGame();
                break;
            default:
                break;
        }
    }

    void InitLogging() {
        auto path = SKSE::log::log_directory();
        if (!path) {
            SKSE::stl::report_and_fail("SKSE log_directory unavailable");
        }
        *path /= std::format("{}.log", SKSE::PluginDeclaration::GetSingleton()->GetName());

        auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(path->string(), true);
        auto logger    = std::make_shared<spdlog::logger>("global",
                                                          spdlog::sinks_init_list{ file_sink });

        logger->set_level(spdlog::level::info);
        logger->flush_on(spdlog::level::warn);
        spdlog::set_default_logger(logger);
        spdlog::flush_every(std::chrono::seconds(1));
        spdlog::set_pattern("[%H:%M:%S.%e] [%l] %v");
    }

}

SKSEPluginLoad(const SKSE::LoadInterface* skse) {
    SKSE::Init(skse);
    InitLogging();

    if (auto* decl = SKSE::PluginDeclaration::GetSingleton()) {
        const auto v = decl->GetVersion();
        spdlog::info("=== Walk Speed Tuner v{}.{}.{} ===", v.major(), v.minor(), v.patch());
    } else {
        spdlog::info("=== Walk Speed Tuner ===");
    }

    SKSE::GetMessagingInterface()->RegisterListener(OnMessage);

    WalkSpeedTuner::WalkSpeedHook::Install();

    return true;
}
