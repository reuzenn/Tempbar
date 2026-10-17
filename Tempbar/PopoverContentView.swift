import SwiftUI

struct PopoverContentView: View {
    @ObservedObject var monitor: HardwareMonitor
    @ObservedObject var lang = LanguageManager.shared

    @AppStorage("liquidGlassOpacity") private var liquidGlassOpacity: Double = 0.85
    @State private var selectedInterval: Double = 3.0

    var body: some View {
        VStack(alignment: .leading, spacing: 13) {
            headerSection

            Divider()

            if let data = monitor.data {
                processorSection(data: data)

                Divider()

                otherSensorsSection(data: data)

                Divider()

                coolingSection(data: data)
            } else if let error = monitor.errorMessage {
                errorState(error: error)
            } else {
                loadingState
            }

            Divider()

            footerSection
        }
        .padding(16)
        .frame(width: 324)
        .background(
            ZStack {
                Rectangle()
                    .fill(.ultraThinMaterial)
                    .opacity(liquidGlassOpacity)

                LinearGradient(
                    colors: [
                        Color.white.opacity(0.14 * liquidGlassOpacity),
                        Color.clear,
                        Color.black.opacity(0.12 * (1.0 - liquidGlassOpacity))
                    ],
                    startPoint: .topLeading,
                    endPoint: .bottomTrailing
                )
            }
        )
        // Adapt dynamically to macOS system appearance
        .preferredColorScheme(nil)
    }

    // MARK: - Header View
    private var headerSection: some View {
        VStack(alignment: .leading, spacing: 4) {
            // Attribution watermark
            HStack {
                Spacer()
                Text("made by reuzen with ❤️")
                    .font(.system(size: 10, weight: .medium, design: .rounded))
                    .foregroundColor(.secondary.opacity(0.40))
                Spacer()
            }
            .padding(.top, -3)
            .padding(.bottom, 2)

            HStack {
                Text(lang.text(.headerTitle))
                    .font(.headline)
                    .fontWeight(.bold)

                Spacer()

                if let data = monitor.data {
                    Text(data.hardwareId)
                        .font(.caption2)
                        .fontWeight(.semibold)
                        .padding(.horizontal, 6)
                        .padding(.vertical, 2)
                        .background(Color.secondary.opacity(0.18))
                        .clipShape(Capsule())
                }
            }

            if let data = monitor.data {
                Text(data.model)
                    .font(.subheadline)
                    .foregroundColor(.secondary)
            } else {
                Text(lang.text(.readingSensors))
                    .font(.subheadline)
                    .foregroundColor(.secondary)
            }
        }
    }

    // MARK: - Processor & Graphics Telemetry
    private func processorSection(data: HardwareReport) -> some View {
        VStack(alignment: .leading, spacing: 10) {
            Text(lang.text(.sectionProcessor))
                .font(.system(size: 10, weight: .semibold))
                .foregroundColor(.secondary)
                .tracking(0.5)

            // CPU thermals
            if let cpuAvg = data.temperatures.cpuAvg {
                MetricRow(
                    icon: "cpu",
                    iconBg: Color.blue,
                    title: lang.text(.cpuTitle),
                    subtitle: data.temperatures.cpuMax != nil ? "\(lang.text(.maxPrefix))\(formatTemp(data.temperatures.cpuMax!))" : nil,
                    value: formatTemp(cpuAvg),
                    tempValue: cpuAvg
                )
            }

            // GPU thermals
            if let gpuAvg = data.temperatures.gpuAvg {
                MetricRow(
                    icon: "display",
                    iconBg: Color.purple,
                    title: lang.text(.gpuTitle),
                    subtitle: data.temperatures.gpuMax != nil ? "\(lang.text(.maxPrefix))\(formatTemp(data.temperatures.gpuMax!))" : nil,
                    value: formatTemp(gpuAvg),
                    tempValue: gpuAvg
                )
            }
        }
    }

    // MARK: - Secondary Components
    private func otherSensorsSection(data: HardwareReport) -> some View {
        VStack(alignment: .leading, spacing: 10) {
            Text(lang.text(.sectionOther))
                .font(.system(size: 10, weight: .semibold))
                .foregroundColor(.secondary)
                .tracking(0.5)

            if let ram = data.temperatures.ramAvg {
                MetricRow(
                    icon: "memorychip",
                    iconBg: Color.green,
                    title: lang.text(.ramTitle),
                    subtitle: nil,
                    value: formatTemp(ram),
                    tempValue: ram
                )
            }

            if let bat = data.temperatures.batteryAvg {
                MetricRow(
                    icon: "battery.100",
                    iconBg: Color.orange,
                    title: lang.text(.batteryTitle),
                    subtitle: nil,
                    value: formatTemp(bat),
                    tempValue: bat
                )
            }

            if let soc = data.temperatures.socAvg {
                MetricRow(
                    icon: "square.3.layers.3d",
                    iconBg: Color.teal,
                    title: lang.text(.socTitle),
                    subtitle: nil,
                    value: formatTemp(soc),
                    tempValue: soc
                )
            }
        }
    }

    // MARK: - Cooling Telemetry
    private func coolingSection(data: HardwareReport) -> some View {
        VStack(alignment: .leading, spacing: 10) {
            Text(lang.text(.sectionCooling))
                .font(.system(size: 10, weight: .semibold))
                .foregroundColor(.secondary)
                .tracking(0.5)

            if data.fans.isEmpty {
                HStack(spacing: 10) {
                    Circle()
                        .fill(Color.gray.opacity(0.3))
                        .frame(width: 28, height: 28)
                        .overlay(
                            Image(systemName: "fanblades.slash")
                                .font(.system(size: 13))
                                .foregroundColor(.secondary)
                        )

                    VStack(alignment: .leading, spacing: 1) {
                        Text(lang.text(.fanlessTitle))
                            .font(.system(size: 13, weight: .medium))
                        Text(lang.text(.fanlessSubtitle))
                            .font(.caption2)
                            .foregroundColor(.secondary)
                    }
                }
            } else {
                ForEach(data.fans) { fan in
                    HStack(spacing: 10) {
                        Circle()
                            .fill(fan.isPassive ? Color.gray.opacity(0.3) : Color.cyan)
                            .frame(width: 28, height: 28)
                            .overlay(
                                Image(systemName: fan.isPassive ? "fanblades.slash" : "fanblades.fill")
                                    .font(.system(size: 13))
                                    .foregroundColor(fan.isPassive ? .secondary : .white)
                            )

                        VStack(alignment: .leading, spacing: 1) {
                            Text(fan.name)
                                .font(.system(size: 13, weight: .medium))

                            if fan.isPassive {
                                Text(lang.text(.silentMode))
                                    .font(.caption2)
                                    .foregroundColor(.secondary)
                            } else {
                                Text("\(Int(fan.currentRpm)) / \(Int(fan.maxRpm)) RPM (%\(Int(fan.percentage)))")
                                    .font(.caption2)
                                    .foregroundColor(.secondary)
                            }
                        }

                        Spacer()

                        Text(fan.isPassive ? "0 RPM" : "\(Int(fan.currentRpm)) RPM")
                            .font(.system(size: 13, weight: .semibold))
                            .foregroundColor(fan.isPassive ? .secondary : .primary)
                    }
                }
            }
        }
    }

    // MARK: - State Indicators
    private var loadingState: some View {
        VStack(spacing: 12) {
            ProgressView()
                .scaleEffect(0.9)
            Text(lang.text(.loadingSensors))
                .font(.caption)
                .foregroundColor(.secondary)
        }
        .frame(maxWidth: .infinity)
        .padding(.vertical, 20)
    }

    private func errorState(error: String) -> some View {
        VStack(spacing: 8) {
            Image(systemName: "exclamationmark.triangle.fill")
                .foregroundColor(.orange)
                .font(.title3)
            Text(error)
                .font(.caption)
                .multilineTextAlignment(.center)
                .foregroundColor(.secondary)
        }
        .frame(maxWidth: .infinity)
        .padding(.vertical, 16)
    }

    // MARK: - Footer Controls
    private var footerSection: some View {
        VStack(spacing: 10) {
            // Polling interval selector
            HStack {
                Text(lang.text(.refreshLabel))
                    .font(.caption)
                    .foregroundColor(.secondary)

                Spacer()

                HStack(spacing: 4) {
                    ForEach([1.0, 3.0, 5.0], id: \.self) { sec in
                        Button(action: {
                            selectedInterval = sec
                            monitor.refreshInterval = sec
                        }) {
                            Text("\(Int(sec))s")
                                .font(.caption2)
                                .fontWeight(selectedInterval == sec ? .bold : .regular)
                                .padding(.horizontal, 7)
                                .padding(.vertical, 3)
                                .background(selectedInterval == sec ? Color.accentColor.opacity(0.25) : Color.clear)
                                .cornerRadius(5)
                        }
                        .buttonStyle(.plain)
                    }
                }
            }

            // Section divider matching upper panels
            Divider()

            // Translucency slider, language menu, and termination action
            HStack(spacing: 8) {
                // Liquid glass material opacity slider
                HStack(spacing: 4) {
                    Image(systemName: "drop.fill")
                        .font(.system(size: 10))
                        .foregroundColor(.cyan)

                    Slider(value: $liquidGlassOpacity, in: 0.30...1.0)
                        .tint(.cyan)
                        .controlSize(.mini)
                        .frame(width: 58)

                    Text("%\(Int(liquidGlassOpacity * 100))")
                        .font(.system(size: 9, weight: .bold, design: .monospaced))
                        .foregroundColor(.secondary)
                }
                .padding(.horizontal, 7)
                .padding(.vertical, 4)
                .background(Color.secondary.opacity(0.12))
                .clipShape(Capsule())

                Spacer()

                // Language selection menu
                Menu {
                    ForEach(AppLanguage.allCases) { item in
                        Button(action: {
                            lang.language = item
                        }) {
                            HStack {
                                Text("\(item.flag)  \(item.displayName) (\(item.rawValue))")
                                if lang.language == item {
                                    Spacer()
                                    Image(systemName: "checkmark")
                                }
                            }
                        }
                    }
                } label: {
                    HStack(spacing: 4) {
                        Image(systemName: "globe")
                            .font(.system(size: 11))
                        Text("\(lang.language.flag) \(lang.language.rawValue)")
                            .font(.system(size: 11, weight: .semibold))
                    }
                    .padding(.horizontal, 8)
                    .padding(.vertical, 4)
                    .background(Color.secondary.opacity(0.14))
                    .clipShape(Capsule())
                }
                .menuStyle(.borderlessButton)

                Spacer()

                // Application termination button
                Button(action: {
                    NSApplication.shared.terminate(nil)
                }) {
                    Text(lang.text(.quitButton))
                        .font(.caption)
                        .foregroundColor(.red)
                }
                .buttonStyle(.borderless)
            }
        }
    }

    private func formatTemp(_ value: Double) -> String {
        return String(format: "%.1f °C", value)
    }
}

// MARK: - Metric Row View
struct MetricRow: View {
    let icon: String
    let iconBg: Color
    let title: String
    let subtitle: String?
    let value: String
    let tempValue: Double

    private var thermalColor: Color {
        if tempValue < 50.0 {
            return .green
        } else if tempValue < 75.0 {
            return .orange
        } else {
            return .red
        }
    }

    var body: some View {
        HStack(spacing: 10) {
            Circle()
                .fill(iconBg)
                .frame(width: 28, height: 28)
                .overlay(
                    Image(systemName: icon)
                        .font(.system(size: 13, weight: .semibold))
                        .foregroundColor(.white)
                )

            VStack(alignment: .leading, spacing: 1) {
                Text(title)
                    .font(.system(size: 13, weight: .medium))

                if let sub = subtitle {
                    Text(sub)
                        .font(.caption2)
                        .foregroundColor(.secondary)
                }
            }

            Spacer()

            HStack(spacing: 6) {
                Circle()
                    .fill(thermalColor)
                    .frame(width: 6, height: 6)

                Text(value)
                    .font(.system(size: 13, weight: .semibold))
            }
        }
    }
}
