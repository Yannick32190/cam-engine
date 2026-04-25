#pragma once
#include "PostProcessor.h"
#include <memory>
#include <map>
#include <vector>
#include <string>

namespace CamEngine {

class PostProcessorRegistry {
public:
    static PostProcessorRegistry& instance();

    void registerProcessor(std::shared_ptr<PostProcessor> pp);
    std::shared_ptr<PostProcessor> get(const std::string& id) const;

    std::vector<std::string> allIds() const;
    std::vector<std::shared_ptr<PostProcessor>> all() const;

    // Enregistre les post-processeurs intégrés (GRBL, LinuxCNC, Fanuc, Mach4)
    static void registerAll();

private:
    PostProcessorRegistry() = default;
    std::map<std::string, std::shared_ptr<PostProcessor>> m_map;
};

} // namespace CamEngine
