import Foundation

// MARK: - Hardware Telemetry Model
struct HardwareReport: Codable {
    let model: String
    let hardwareId: String
    let cpuModel: String
    let osVersion: String
    let temperatures: Temperatures
    let fans: [FanDetail]

    enum CodingKeys: String, CodingKey {
        case model
        case hardwareId = "hardware_id"
        case cpuModel = "cpu_model"
        case osVersion = "os_version"
        case temperatures
        case fans
    }
}

// MARK: - Component Temperatures
struct Temperatures: Codable {
    let cpuAvg: Double?
    let cpuMax: Double?
    let gpuAvg: Double?
    let gpuMax: Double?
    let ramAvg: Double?
    let batteryAvg: Double?
    let socAvg: Double?

    enum CodingKeys: String, CodingKey {
        case cpuAvg = "cpu_avg"
        case cpuMax = "cpu_max"
        case gpuAvg = "gpu_avg"
        case gpuMax = "gpu_max"
        case ramAvg = "ram_avg"
        case batteryAvg = "battery_avg"
        case socAvg = "soc_avg"
    }
}

// MARK: - Fan Metrics
struct FanDetail: Codable, Identifiable {
    let id: Int
    let name: String
    let currentRpm: Double
    let minRpm: Double
    let maxRpm: Double
    let targetRpm: Double

    enum CodingKeys: String, CodingKey {
        case id
        case name
        case currentRpm = "current_rpm"
        case minRpm = "min_rpm"
        case maxRpm = "max_rpm"
        case targetRpm = "target_rpm"
    }

    var percentage: Double {
        guard maxRpm > minRpm else { return 0.0 }
        let clamped = max(min(currentRpm, maxRpm), 0)
        return (clamped / maxRpm) * 100.0
    }

    var isPassive: Bool {
        return currentRpm <= 0
    }
}
