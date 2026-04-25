#include "PostProcessorRegistry.h"
#include "GRBLPostProcessor.h"
#include "LinuxCNCPostProcessor.h"
#include "FanucPostProcessor.h"
#include "Mach4PostProcessor.h"

namespace CamEngine {

PostProcessorRegistry& PostProcessorRegistry::instance() {
    static PostProcessorRegistry inst;
    return inst;
}

void PostProcessorRegistry::registerProcessor(std::shared_ptr<PostProcessor> pp) {
    m_map[pp->id()] = std::move(pp);
}

std::shared_ptr<PostProcessor> PostProcessorRegistry::get(const std::string& id) const {
    auto it = m_map.find(id);
    return (it != m_map.end()) ? it->second : nullptr;
}

std::vector<std::string> PostProcessorRegistry::allIds() const {
    std::vector<std::string> ids;
    for (const auto& kv : m_map) ids.push_back(kv.first);
    return ids;
}

std::vector<std::shared_ptr<PostProcessor>> PostProcessorRegistry::all() const {
    std::vector<std::shared_ptr<PostProcessor>> result;
    for (const auto& kv : m_map) result.push_back(kv.second);
    return result;
}

void PostProcessorRegistry::registerAll() {
    auto& reg = instance();
    reg.registerProcessor(std::make_shared<GRBLPostProcessor>());
    reg.registerProcessor(std::make_shared<LinuxCNCPostProcessor>());
    reg.registerProcessor(std::make_shared<FanucPostProcessor>());
    reg.registerProcessor(std::make_shared<Mach4PostProcessor>());
}

} // namespace CamEngine
