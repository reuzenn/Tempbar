#include <iostream>
#include <iomanip>
#include <vector>
#include <string>
#include <cstring>
#include <chrono>
#include <thread>
#include <csignal>
#include <atomic>
#include <numeric>
#include <algorithm>
#include <cctype>
#include <sys/sysctl.h>
#include <IOKit/IOKitLib.h>
#include <CoreFoundation/CoreFoundation.h>

// Apple SMC command selectors & constants
#define KERNEL_INDEX_SMC     2
#define SMC_CMD_READ_KEYINFO 9
#define SMC_CMD_READ_BYTES   5
#define SMC_CMD_READ_INDEX   8

// SMC data types
#define DATATYPE_FPE2  0x66706532 // 'fpe2' - 14.2 fixed point
#define DATATYPE_FLT   0x666c7420 // 'flt ' - 32-bit IEEE float
#define DATATYPE_UI8   0x75693820 // 'ui8 ' - 8-bit unsigned int
#define DATATYPE_UI16  0x75693136 // 'ui16' - 16-bit unsigned int
#define DATATYPE_UI32  0x75693332 // 'ui32' - 32-bit unsigned int
#define DATATYPE_SP78  0x73703738 // 'sp78' - signed 8.8 fixed point

typedef struct {
    unsigned char major;
    unsigned char minor;
    unsigned char build;
    unsigned char reserved[1];
    unsigned short release;
} SMCKeyData_vers_t;

typedef struct {
    uint16_t version;
    uint16_t length;
    uint32_t cpuPLimit;
    uint32_t gpuPLimit;
    uint32_t memPLimit;
} SMCKeyData_pLimitData_t;

typedef struct {
    uint32_t dataSize;
    uint32_t dataType;
    uint8_t dataAttributes;
} SMCKeyData_keyInfo_t;

typedef struct {
    uint32_t key;
    SMCKeyData_vers_t vers;
    SMCKeyData_pLimitData_t pLimitData;
    SMCKeyData_keyInfo_t keyInfo;
    uint8_t result;
    uint8_t status;
    uint8_t data8;
    uint32_t data32;
    uint8_t bytes[32];
} SMCKeyData_t;

typedef struct {
    uint32_t key;
    uint32_t dataSize;
    uint32_t dataType;
    uint8_t bytes[32];
} SMCVal_t;

struct FanInfo {
    int id;
    std::string name;
    float currentRpm;
    float minRpm;
    float maxRpm;
    float targetRpm;
};

struct TempStats {
    float average = 0.0f;
    float maximum = 0.0f;
    size_t sensorCount = 0;
};

class AppleSMCManager {
public:
    AppleSMCManager() : connection(0), initializedSensors(false) {}
    ~AppleSMCManager() { close(); }

    bool open() {
        CFDictionaryRef matching = IOServiceMatching("AppleSMC");
        if (!matching) return false;

        io_service_t service = IOServiceGetMatchingService(kIOMainPortDefault, matching);
        if (!service) return false;

        kern_return_t kr = IOServiceOpen(service, mach_task_self(), 0, &connection);
        IOObjectRelease(service);

        if (kr == kIOReturnSuccess) {
            discoverSensors();
            return true;
        }
        return false;
    }

    void close() {
        if (connection) {
            IOServiceClose(connection);
            connection = 0;
        }
    }

    bool isConnected() const {
        return connection != 0;
    }

    int getFanCount() {
        SMCVal_t val;
        if (readKey("FNum", val) && val.dataSize >= 1) {
            return static_cast<int>(val.bytes[0]);
        }
        return 0;
    }

    std::vector<FanInfo> getFans() {
        std::vector<FanInfo> fans;
        int count = getFanCount();
        for (int i = 0; i < count; ++i) {
            FanInfo fan;
            fan.id = i;
            fan.name = (i == 0) ? "Fan 1 (Left)" : (i == 1 ? "Fan 2 (Right)" : "Fan " + std::to_string(i + 1));
            fan.currentRpm = readRpmKey("F" + std::to_string(i) + "Ac");
            fan.minRpm = readRpmKey("F" + std::to_string(i) + "Mn");
            fan.maxRpm = readRpmKey("F" + std::to_string(i) + "Mx");
            fan.targetRpm = readRpmKey("F" + std::to_string(i) + "Tg");
            fans.push_back(fan);
        }
        return fans;
    }

    TempStats getCpuTemperature()     { return aggregateTemps(cpuKeys); }
    TempStats getGpuTemperature()     { return aggregateTemps(gpuKeys); }
    TempStats getRamTemperature()     { return aggregateTemps(ramKeys); }
    TempStats getBatteryTemperature() { return aggregateTemps(batteryKeys); }
    TempStats getSocTemperature()     { return aggregateTemps(socKeys); }

    size_t getTotalSensors() const {
        return cpuKeys.size() + gpuKeys.size() + ramKeys.size() + batteryKeys.size() + socKeys.size();
    }

private:
    io_connect_t connection;
    bool initializedSensors;
    std::vector<std::string> cpuKeys;
    std::vector<std::string> gpuKeys;
    std::vector<std::string> ramKeys;
    std::vector<std::string> batteryKeys;
    std::vector<std::string> socKeys;

    static uint32_t strToKey(const std::string& str) {
        uint32_t key = 0;
        for (size_t i = 0; i < 4 && i < str.length(); ++i) {
            key = (key << 8) | static_cast<uint8_t>(str[i]);
        }
        return key;
    }

    static void keyToStr(uint32_t key, char* str) {
        str[0] = (key >> 24) & 0xFF;
        str[1] = (key >> 16) & 0xFF;
        str[2] = (key >> 8) & 0xFF;
        str[3] = key & 0xFF;
        str[4] = '\0';
    }

    bool callSMC(int index, SMCKeyData_t* input, SMCKeyData_t* output) {
        size_t inSize = sizeof(SMCKeyData_t);
        size_t outSize = sizeof(SMCKeyData_t);
        kern_return_t kr = IOConnectCallStructMethod(connection, index, input, inSize, output, &outSize);
        return (kr == kIOReturnSuccess && output->result == 0);
    }

    bool readKey(const std::string& keyStr, SMCVal_t& val) {
        if (!connection) return false;

        SMCKeyData_t in, out;
        std::memset(&in, 0, sizeof(in));
        std::memset(&out, 0, sizeof(out));

        in.key = strToKey(keyStr);
        in.data8 = SMC_CMD_READ_KEYINFO;

        if (!callSMC(KERNEL_INDEX_SMC, &in, &out)) return false;

        val.key = in.key;
        val.dataSize = out.keyInfo.dataSize;
        val.dataType = out.keyInfo.dataType;

        std::memset(&in, 0, sizeof(in));
        std::memset(&out, 0, sizeof(out));
        in.key = val.key;
        in.keyInfo.dataSize = val.dataSize;
        in.data8 = SMC_CMD_READ_BYTES;

        if (!callSMC(KERNEL_INDEX_SMC, &in, &out)) return false;

        std::memcpy(val.bytes, out.bytes, sizeof(val.bytes));
        return true;
    }

    bool readTemperatureValue(const std::string& keyStr, float& outTemp) {
        SMCVal_t val;
        if (!readKey(keyStr, val)) return false;

        // Apple Silicon float ('flt ')
        if (val.dataType == DATATYPE_FLT && val.dataSize == 4) {
            float f = 0.0f;
            std::memcpy(&f, val.bytes, sizeof(float));
            if (f > 0.0f && f < 135.0f) {
                outTemp = f;
                return true;
            }
        }
        // Intel signed fixed-point 8.8 ('sp78')
        if (val.dataType == DATATYPE_SP78 && val.dataSize == 2) {
            int16_t raw = static_cast<int16_t>((val.bytes[0] << 8) | val.bytes[1]);
            float f = raw / 256.0f;
            if (f > 0.0f && f < 135.0f) {
                outTemp = f;
                return true;
            }
        }
        return false;
    }

    float readRpmKey(const std::string& keyStr) {
        SMCVal_t val;
        if (!readKey(keyStr, val)) return 0.0f;

        if (val.dataType == DATATYPE_FLT && val.dataSize == 4) {
            float f = 0.0f;
            std::memcpy(&f, val.bytes, sizeof(float));
            return (f < 0.0f) ? 0.0f : f;
        }
        if (val.dataType == DATATYPE_FPE2 && val.dataSize == 2) {
            return ((val.bytes[0] << 6) | (val.bytes[1] >> 2)) + (val.bytes[1] & 0x03) * 0.25f;
        }
        if (val.dataType == DATATYPE_UI16 && val.dataSize == 2) {
            uint16_t u = (val.bytes[0] << 8) | val.bytes[1];
            return static_cast<float>(u);
        }
        return 0.0f;
    }

    void discoverSensors() {
        if (initializedSensors) return;

        SMCKeyData_t in, out;
        std::memset(&in, 0, sizeof(in));
        in.key = strToKey("#KEY");
        in.data8 = SMC_CMD_READ_BYTES;
        in.keyInfo.dataSize = 4;

        uint32_t totalKeys = 0;
        if (callSMC(KERNEL_INDEX_SMC, &in, &out)) {
            totalKeys = (out.bytes[0] << 24) | (out.bytes[1] << 16) | (out.bytes[2] << 8) | out.bytes[3];
        }

        // Cache valid sensor keys for fast periodic reads
        for (uint32_t i = 0; i < totalKeys; ++i) {
            std::memset(&in, 0, sizeof(in));
            in.data8 = SMC_CMD_READ_INDEX;
            in.data32 = i;

            if (callSMC(KERNEL_INDEX_SMC, &in, &out)) {
                char kStr[5];
                keyToStr(out.key, kStr);
                if (kStr[0] == 'T') {
                    float testVal = 0.0f;
                    if (readTemperatureValue(kStr, testVal)) {
                        if (kStr[1] == 'p' || (kStr[1] == 'C' && kStr[2] == '0')) {
                            cpuKeys.push_back(kStr);
                        } else if (kStr[1] == 'g' || (kStr[1] == 'G' && kStr[2] == '0')) {
                            gpuKeys.push_back(kStr);
                        } else if (kStr[1] == 'm') {
                            ramKeys.push_back(kStr);
                        } else if (kStr[1] == 'B') {
                            batteryKeys.push_back(kStr);
                        } else if (kStr[1] == 's' || kStr[1] == 'f') {
                            socKeys.push_back(kStr);
                        }
                    }
                }
            }
        }
        initializedSensors = true;
    }

    TempStats aggregateTemps(const std::vector<std::string>& keys) {
        TempStats stats;
        std::vector<float> values;
        values.reserve(keys.size());

        for (const auto& k : keys) {
            float val = 0.0f;
            if (readTemperatureValue(k, val)) {
                values.push_back(val);
            }
        }

        stats.sensorCount = values.size();
        if (values.empty()) return stats;

        stats.maximum = *std::max_element(values.begin(), values.end());
        float sum = std::accumulate(values.begin(), values.end(), 0.0f);
        stats.average = sum / values.size();
        return stats;
    }
};

std::string getSysctlString(const char* name) {
    char buffer[256];
    size_t size = sizeof(buffer);
    if (sysctlbyname(name, buffer, &size, nullptr, 0) == 0) {
        return std::string(buffer);
    }
    return "Unknown";
}

// Retrieves human-readable model name from IODeviceTree:/product
std::string getMacModelName() {
    std::string model;
    io_registry_entry_t entry = IORegistryEntryFromPath(kIOMainPortDefault, "IODeviceTree:/product");
    if (entry) {
        CFTypeRef prop = IORegistryEntryCreateCFProperty(entry, CFSTR("product-name"), kCFAllocatorDefault, 0);
        if (prop) {
            if (CFGetTypeID(prop) == CFDataGetTypeID()) {
                CFDataRef data = static_cast<CFDataRef>(prop);
                const char* ptr = reinterpret_cast<const char*>(CFDataGetBytePtr(data));
                size_t len = CFDataGetLength(data);
                model = std::string(ptr, len);
            } else if (CFGetTypeID(prop) == CFStringGetTypeID()) {
                char buf[128];
                if (CFStringGetCString(static_cast<CFStringRef>(prop), buf, sizeof(buf), kCFStringEncodingUTF8)) {
                    model = buf;
                }
            }
            CFRelease(prop);
        }
        IOObjectRelease(entry);
    }

    while (!model.empty() && (model.back() == '\0' || std::isspace(static_cast<unsigned char>(model.back())))) {
        model.pop_back();
    }

    if (model.empty()) {
        model = getSysctlString("hw.model");
    }
    return model;
}

std::string getProgressBar(float current, float max, int width = 20) {
    if (max <= 0.0f) return std::string(width, ' ');
    float ratio = current / max;
    if (ratio < 0.0f) ratio = 0.0f;
    if (ratio > 1.0f) ratio = 1.0f;

    int filled = static_cast<int>(ratio * width);
    std::string bar;
    for (int i = 0; i < width; ++i) {
        if (i < filled) bar += "█";
        else bar += "░";
    }
    return bar;
}

std::string getTempColor(float temp) {
    if (temp <= 0.0f) return "\033[0m";
    if (temp < 50.0f) return "\033[1;32m"; // Green  - Cool
    if (temp < 70.0f) return "\033[1;34m"; // Blue   - Normal
    if (temp < 85.0f) return "\033[1;33m"; // Yellow - Warm
    return "\033[1;31m";                   // Red    - High
}

std::string getFanColor(float percent, float rpm) {
    if (rpm == 0.0f) return "\033[1;32m";  // Silent
    if (percent < 50.0f) return "\033[1;34m";
    if (percent < 80.0f) return "\033[1;33m";
    return "\033[1;31m";
}

std::atomic<bool> g_running(true);

void handleSignal(int) {
    g_running = false;
}

void printDashboard(AppleSMCManager& smc, const std::string& modelName, const std::string& hwModel,
                    const std::string& cpuModel, const std::string& osVersion, bool clearScreen) {
    TempStats cpu = smc.getCpuTemperature();
    TempStats gpu = smc.getGpuTemperature();
    TempStats ram = smc.getRamTemperature();
    TempStats bat = smc.getBatteryTemperature();
    TempStats soc = smc.getSocTemperature();
    std::vector<FanInfo> fans = smc.getFans();

    // Format model identifier for fanless architectures
    std::string displayModel = modelName;
    bool isFanless = fans.empty();
    if (isFanless) {
        if (displayModel.find("MacBook Air") != std::string::npos) {
            displayModel = "MacBook Air - No fans";
        } else {
            displayModel += " - No fans";
        }
    }

    if (clearScreen) {
        std::cout << "\033[H\033[J";
    }

    std::cout << "\033[1;36m======================================================================\033[0m\n";
    std::cout << "\033[1;37m        macOS Hardware Monitor — CPU, GPU, Thermals & Fans           \033[0m\n";
    std::cout << "\033[1;36m======================================================================\033[0m\n";
    std::cout << " \033[1;34m• Model:\033[0m            " << displayModel << " \033[2m(" << hwModel << ")\033[0m\n";
    std::cout << " \033[1;34m• Processor:\033[0m        " << cpuModel << "\n";
    std::cout << " \033[1;34m• Operating System:\033[0m macOS " << osVersion << "\n";
    std::cout << " \033[1;34m• Active Sensors:\033[0m   " << smc.getTotalSensors() << " hardware sensors online\n";
    std::cout << "\033[1;36m----------------------------------------------------------------------\033[0m\n";
    std::cout << "\033[1;35m[1] THERMAL METRICS\033[0m\n\n";

    // CPU
    if (cpu.sensorCount > 0) {
        std::string cCol = getTempColor(cpu.maximum);
        std::string bar = getProgressBar(cpu.average, 100.0f, 18);
        std::cout << "  \033[1mCPU (Processor):\033[0m   " << cCol << std::fixed << std::setprecision(1)
                  << std::setw(5) << cpu.average << " °C\033[0m (Max: " << cCol << cpu.maximum << " °C\033[0m) "
                  << "[" << cCol << bar << "\033[0m]  \033[2m(" << cpu.sensorCount << " core sensors)\033[0m\n";
    }

    // GPU
    if (gpu.sensorCount > 0) {
        std::string gCol = getTempColor(gpu.maximum);
        std::string bar = getProgressBar(gpu.average, 100.0f, 18);
        std::cout << "  \033[1mGPU (Graphics):\033[0m    " << gCol << std::fixed << std::setprecision(1)
                  << std::setw(5) << gpu.average << " °C\033[0m (Max: " << gCol << gpu.maximum << " °C\033[0m) "
                  << "[" << gCol << bar << "\033[0m]  \033[2m(" << gpu.sensorCount << " GPU sensors)\033[0m\n";
    }

    // RAM & SoC
    if (ram.sensorCount > 0) {
        std::string rCol = getTempColor(ram.average);
        std::cout << "  \033[1mMemory (RAM):\033[0m      " << rCol << std::fixed << std::setprecision(1)
                  << std::setw(5) << ram.average << " °C\033[0m \033[2m(" << ram.sensorCount << " sensors)\033[0m\n";
    }

    if (soc.sensorCount > 0) {
        std::string sCol = getTempColor(soc.average);
        std::cout << "  \033[1mSoC / Fabric:\033[0m      " << sCol << std::fixed << std::setprecision(1)
                  << std::setw(5) << soc.average << " °C\033[0m\n";
    }

    // Battery
    if (bat.sensorCount > 0) {
        std::string bCol = getTempColor(bat.average);
        std::cout << "  \033[1mBattery:\033[0m           " << bCol << std::fixed << std::setprecision(1)
                  << std::setw(5) << bat.average << " °C\033[0m\n";
    }

    std::cout << "\n\033[1;36m----------------------------------------------------------------------\033[0m\n";
    std::cout << "\033[1;35m[2] COOLING & FAN METRICS\033[0m\n\n";

    if (isFanless) {
        if (modelName.find("MacBook Air") != std::string::npos) {
            std::cout << "  \033[1;33m[i] MacBook Air - No fans (Passive cooling architecture)\033[0m\n";
        } else {
            std::cout << "  \033[1;33m[i] " << modelName << " - No fans (Passive cooling architecture)\033[0m\n";
        }
        std::cout << "  \033[2mNo hardware fans installed on this device.\033[0m\n\n";
    } else {
        for (const auto& fan : fans) {
            float percent = (fan.maxRpm > 0.0f) ? (fan.currentRpm / fan.maxRpm * 100.0f) : 0.0f;
            std::string fCol = getFanColor(percent, fan.currentRpm);
            std::string bar = getProgressBar(fan.currentRpm, fan.maxRpm, 18);

            std::cout << "  \033[1m" << fan.name << ":\033[0m\n";
            std::cout << "    Current Speed: " << fCol << std::setw(5) << static_cast<int>(fan.currentRpm) << " RPM\033[0m "
                      << "[" << fCol << bar << "\033[0m] "
                      << std::fixed << std::setprecision(1) << percent << "%\n";
            std::cout << "    Range:         " << static_cast<int>(fan.minRpm) << " — " << static_cast<int>(fan.maxRpm) << " RPM "
                      << "| Target: " << static_cast<int>(fan.targetRpm) << " RPM\n";

            if (fan.currentRpm == 0.0f) {
                std::cout << "    \033[2mStatus: Passive / Zero-RPM Silent Mode (Fans stopped at low thermals)\033[0m\n";
            } else {
                std::cout << "    \033[2mStatus: Active Cooling Engaged\033[0m\n";
            }
            std::cout << "\n";
        }
    }

    std::cout << "\033[1;36m======================================================================\033[0m\n";
}

#ifndef TEMPBAR_LIBRARY
int main(int argc, char* argv[]) {
    bool watchMode = false;
    bool jsonMode = false;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-w" || arg == "--watch") {
            watchMode = true;
        } else if (arg == "-j" || arg == "--json") {
            jsonMode = true;
        } else if (arg == "-h" || arg == "--help") {
            std::cout << "Usage: " << argv[0] << " [OPTIONS]\n"
                      << "Options:\n"
                      << "  -w, --watch    Live monitor mode (refreshes every 1 second)\n"
                      << "  -j, --json     Output all sensor metrics in JSON format\n"
                      << "  -h, --help     Show this help message\n";
            return 0;
        }
    }

    AppleSMCManager smc;
    if (!smc.open()) {
        std::cerr << "\033[1;31m[Error]\033[0m Failed to connect to AppleSMC service.\n";
        return 1;
    }

    std::string modelName = getMacModelName();
    std::string hwModel = getSysctlString("hw.model");
    std::string cpuModel = getSysctlString("machdep.cpu.brand_string");
    std::string osVersion = getSysctlString("kern.osproductversion");

    if (jsonMode) {
        TempStats cpu = smc.getCpuTemperature();
        TempStats gpu = smc.getGpuTemperature();
        TempStats ram = smc.getRamTemperature();
        TempStats bat = smc.getBatteryTemperature();
        TempStats soc = smc.getSocTemperature();
        std::vector<FanInfo> fans = smc.getFans();

        std::string displayModel = modelName;
        if (fans.empty()) {
            displayModel = (displayModel.find("MacBook Air") != std::string::npos)
                               ? "MacBook Air - No fans"
                               : displayModel + " - No fans";
        }

        std::cout << "{\n";
        std::cout << "  \"model\": \"" << displayModel << "\",\n";
        std::cout << "  \"hardware_id\": \"" << hwModel << "\",\n";
        std::cout << "  \"cpu_model\": \"" << cpuModel << "\",\n";
        std::cout << "  \"os_version\": \"" << osVersion << "\",\n";
        std::cout << "  \"temperatures\": {\n";
        std::cout << "    \"cpu_avg\": " << cpu.average << ",\n";
        std::cout << "    \"cpu_max\": " << cpu.maximum << ",\n";
        std::cout << "    \"gpu_avg\": " << gpu.average << ",\n";
        std::cout << "    \"gpu_max\": " << gpu.maximum << ",\n";
        std::cout << "    \"ram_avg\": " << ram.average << ",\n";
        std::cout << "    \"battery_avg\": " << bat.average << ",\n";
        std::cout << "    \"soc_avg\": " << soc.average << "\n";
        std::cout << "  },\n";
        std::cout << "  \"fans\": [\n";
        for (size_t i = 0; i < fans.size(); ++i) {
            const auto& fan = fans[i];
            std::cout << "    {\n";
            std::cout << "      \"id\": " << fan.id << ",\n";
            std::cout << "      \"name\": \"" << fan.name << "\",\n";
            std::cout << "      \"current_rpm\": " << fan.currentRpm << ",\n";
            std::cout << "      \"min_rpm\": " << fan.minRpm << ",\n";
            std::cout << "      \"max_rpm\": " << fan.maxRpm << ",\n";
            std::cout << "      \"target_rpm\": " << fan.targetRpm << "\n";
            std::cout << "    }" << (i + 1 < fans.size() ? "," : "") << "\n";
        }
        std::cout << "  ]\n";
        std::cout << "}\n";
        return 0;
    }

    if (watchMode) {
        std::signal(SIGINT, handleSignal);
        std::signal(SIGTERM, handleSignal);

        std::cout << "\033[?25l"; // Hide cursor
        while (g_running) {
            printDashboard(smc, modelName, hwModel, cpuModel, osVersion, true);
            std::cout << "\033[2mLive monitoring active (Press Ctrl+C to exit)...\033[0m\n";
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        }
        std::cout << "\033[?25h\n"; // Restore cursor
    } else {
        printDashboard(smc, modelName, hwModel, cpuModel, osVersion, false);
    }

    return 0;
}
#endif // TEMPBAR_LIBRARY
