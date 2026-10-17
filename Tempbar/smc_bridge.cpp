#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#define TEMPBAR_LIBRARY 1
#include "macos.cpp"

static AppleSMCManager g_smc_inst;
static std::string g_smc_json;
static std::string g_smc_model;
static std::string g_smc_hw;
static std::string g_smc_cpu;
static std::string g_smc_os;
static bool g_smc_inited = false;

extern "C" {

// Initializes SMC connection and caches discovered keys once
bool tempbar_init(void) {
    if (g_smc_inited && g_smc_inst.isConnected()) return true;
    if (g_smc_inst.open()) {
        g_smc_model = getMacModelName();
        g_smc_hw = getSysctlString("hw.model");
        g_smc_cpu = getSysctlString("machdep.cpu.brand_string");
        g_smc_os = getSysctlString("kern.osproductversion");
        g_smc_inited = true;
        return true;
    }
    return false;
}

// Queries cached sensors in-process (< 0.2ms, zero subprocess overhead)
const char* tempbar_get_json(void) {
    if (!g_smc_inited) {
        if (!tempbar_init()) return "{}";
    }

    TempStats cpu = g_smc_inst.getCpuTemperature();
    TempStats gpu = g_smc_inst.getGpuTemperature();
    TempStats ram = g_smc_inst.getRamTemperature();
    TempStats bat = g_smc_inst.getBatteryTemperature();
    TempStats soc = g_smc_inst.getSocTemperature();
    std::vector<FanInfo> fans = g_smc_inst.getFans();

    std::string displayModel = g_smc_model;
    if (fans.empty()) {
        displayModel = (displayModel.find("MacBook Air") != std::string::npos)
                           ? "MacBook Air - No fans"
                           : displayModel + " - No fans";
    }

    std::ostringstream ss;
    ss << "{\n";
    ss << "  \"model\": \"" << displayModel << "\",\n";
    ss << "  \"hardware_id\": \"" << g_smc_hw << "\",\n";
    ss << "  \"cpu_model\": \"" << g_smc_cpu << "\",\n";
    ss << "  \"os_version\": \"" << g_smc_os << "\",\n";
    ss << "  \"temperatures\": {\n";
    ss << "    \"cpu_avg\": " << cpu.average << ",\n";
    ss << "    \"cpu_max\": " << cpu.maximum << ",\n";
    ss << "    \"gpu_avg\": " << gpu.average << ",\n";
    ss << "    \"gpu_max\": " << gpu.maximum << ",\n";
    ss << "    \"ram_avg\": " << ram.average << ",\n";
    ss << "    \"battery_avg\": " << bat.average << ",\n";
    ss << "    \"soc_avg\": " << soc.average << "\n";
    ss << "  },\n";
    ss << "  \"fans\": [\n";
    for (size_t i = 0; i < fans.size(); ++i) {
        const auto& fan = fans[i];
        ss << "    {\n";
        ss << "      \"id\": " << fan.id << ",\n";
        ss << "      \"name\": \"" << fan.name << "\",\n";
        ss << "      \"current_rpm\": " << fan.currentRpm << ",\n";
        ss << "      \"min_rpm\": " << fan.minRpm << ",\n";
        ss << "      \"max_rpm\": " << fan.maxRpm << ",\n";
        ss << "      \"target_rpm\": " << fan.targetRpm << "\n";
        ss << "    }" << (i + 1 < fans.size() ? "," : "") << "\n";
    }
    ss << "  ]\n";
    ss << "}\n";

    g_smc_json = ss.str();
    return g_smc_json.c_str();
}

}
