import SwiftUI

@main
struct TempbarApp: App {
    @StateObject private var monitor = HardwareMonitor()

    var body: some Scene {
        MenuBarExtra {
            PopoverContentView(monitor: monitor)
        } label: {
            HStack(spacing: 5) {
                Image(systemName: "thermometer.medium")
                    .imageScale(.medium)

                if let cpu = monitor.data?.temperatures.cpuAvg {
                    Text(String(format: "%.0f°C", cpu))
                        .font(.system(size: 12, weight: .medium, design: .rounded))
                } else {
                    Text("--°C")
                        .font(.system(size: 12, weight: .medium, design: .rounded))
                }

                // Display active fan speed if tachometer registers rotation
                if let fans = monitor.data?.fans,
                   let activeFan = fans.first(where: { $0.currentRpm > 0 }) {
                    Image(systemName: "fanblades.fill")
                        .imageScale(.small)
                    Text("\(Int(activeFan.currentRpm))")
                        .font(.system(size: 11, weight: .regular, design: .rounded))
                }
            }
        }
        .menuBarExtraStyle(.window)
    }
}
