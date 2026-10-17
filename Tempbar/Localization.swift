import Foundation
import SwiftUI

enum AppLanguage: String, CaseIterable, Identifiable {
    case tr = "TR"
    case en = "EN"
    case fr = "FR"
    case it = "ITA"
    case ru = "RUS"

    var id: String { rawValue }

    var flag: String {
        switch self {
        case .tr: return "🇹🇷"
        case .en: return "🇬🇧"
        case .fr: return "🇫🇷"
        case .it: return "🇮🇹"
        case .ru: return "🇷🇺"
        }
    }

    var displayName: String {
        switch self {
        case .tr: return "Türkçe"
        case .en: return "English"
        case .fr: return "Français"
        case .it: return "Italiano"
        case .ru: return "Русский"
        }
    }
}

enum AppTheme: String, CaseIterable, Identifiable {
    case system = "System"
    case light = "Light"
    case dark = "Dark"

    var id: String { rawValue }

    var colorScheme: ColorScheme? {
        switch self {
        case .system: return nil
        case .light: return .light
        case .dark: return .dark
        }
    }

    var icon: String {
        switch self {
        case .system: return "circle.lefthalf.filled"
        case .light: return "sun.max.fill"
        case .dark: return "moon.fill"
        }
    }
}

class LanguageManager: ObservableObject {
    static let shared = LanguageManager()

    @AppStorage("selectedAppLanguage") var currentLanguage: String = "TR" {
        didSet {
            objectWillChange.send()
        }
    }

    var language: AppLanguage {
        get { AppLanguage(rawValue: currentLanguage) ?? .tr }
        set { currentLanguage = newValue.rawValue }
    }

    // MARK: - Localized Strings
    func text(_ key: LocalizationKey) -> String {
        switch key {
        case .headerTitle:
            switch language {
            case .tr: return "Donanım & Termal"
            case .en: return "Hardware & Thermals"
            case .fr: return "Matériel & Thermique"
            case .it: return "Hardware e Termiche"
            case .ru: return "Оборудование и тепло"
            }
        case .readingSensors:
            switch language {
            case .tr: return "Sensörler okunuyor..."
            case .en: return "Reading sensors..."
            case .fr: return "Lecture des capteurs..."
            case .it: return "Lettura dei sensori..."
            case .ru: return "Чтение датчиков..."
            }
        case .sectionProcessor:
            switch language {
            case .tr: return "İŞLEMCİ & GRAFİK"
            case .en: return "PROCESSOR & GRAPHICS"
            case .fr: return "PROCESSEUR & GRAPHIQUE"
            case .it: return "PROCESSORE E GRAFICA"
            case .ru: return "ПРОЦЕССОР И ГРАФИКА"
            }
        case .cpuTitle:
            switch language {
            case .tr: return "İşlemci (CPU)"
            case .en: return "Processor (CPU)"
            case .fr: return "Processeur (CPU)"
            case .it: return "Processore (CPU)"
            case .ru: return "Процессор (ЦП)"
            }
        case .gpuTitle:
            switch language {
            case .tr: return "Grafik (GPU)"
            case .en: return "Graphics (GPU)"
            case .fr: return "Graphique (GPU)"
            case .it: return "Grafica (GPU)"
            case .ru: return "Графика (ГП)"
            }
        case .maxPrefix:
            switch language {
            case .tr: return "Maks: "
            case .en: return "Max: "
            case .fr: return "Max : "
            case .it: return "Max: "
            case .ru: return "Макс: "
            }
        case .sectionOther:
            switch language {
            case .tr: return "DİĞER BİLEŞENLER"
            case .en: return "OTHER SENSORS"
            case .fr: return "AUTRES COMPOSANTS"
            case .it: return "ALTRI COMPONENTI"
            case .ru: return "ДРУГИЕ КОМПОНЕНТЫ"
            }
        case .ramTitle:
            switch language {
            case .tr: return "Bellek (RAM)"
            case .en: return "Memory (RAM)"
            case .fr: return "Mémoire (RAM)"
            case .it: return "Memoria (RAM)"
            case .ru: return "Память (ОЗУ)"
            }
        case .batteryTitle:
            switch language {
            case .tr: return "Batarya"
            case .en: return "Battery"
            case .fr: return "Batterie"
            case .it: return "Batteria"
            case .ru: return "Аккумулятор"
            }
        case .socTitle:
            switch language {
            case .tr: return "SoC / Fabric"
            case .en: return "SoC / Fabric"
            case .fr: return "SoC / Fabric"
            case .it: return "SoC / Fabric"
            case .ru: return "SoC / Архитектура"
            }
        case .sectionCooling:
            switch language {
            case .tr: return "SOĞUTMA & FANLAR"
            case .en: return "COOLING & FANS"
            case .fr: return "REFROIDISSEMENT & VENTILATEURS"
            case .it: return "RAFFREDDAMENTO E VENTOLE"
            case .ru: return "ОХЛАЖДЕНИЕ И ВЕНТИЛЯТОРЫ"
            }
        case .fanlessTitle:
            switch language {
            case .tr: return "Fansız Pasif Tasarım"
            case .en: return "Fanless Passive Design"
            case .fr: return "Conception passive sans ventilateur"
            case .it: return "Design passivo senza ventole"
            case .ru: return "Пассивное охлаждение"
            }
        case .fanlessSubtitle:
            switch language {
            case .tr: return "Sıfır gürültü / Tamamen sessiz"
            case .en: return "Zero noise / Completely silent"
            case .fr: return "Zéro bruit / Complètement silencieux"
            case .it: return "Zero rumore / Silenzioso"
            case .ru: return "Бесшумный режим"
            }
        case .silentMode:
            switch language {
            case .tr: return "Sessiz Mod (0 RPM)"
            case .en: return "Silent Mode (0 RPM)"
            case .fr: return "Mode silencieux (0 RPM)"
            case .it: return "Modalità silenciosa (0 RPM)"
            case .ru: return "Тихий режим (0 RPM)"
            }
        case .refreshLabel:
            switch language {
            case .tr: return "Yenileme:"
            case .en: return "Interval:"
            case .fr: return "Intervalle :"
            case .it: return "Intervallo:"
            case .ru: return "Интервал:"
            }
        case .themeLabel:
            switch language {
            case .tr: return "Tema"
            case .en: return "Theme"
            case .fr: return "Thème"
            case .it: return "Tema"
            case .ru: return "Тема"
            }
        case .themeSystem:
            switch language {
            case .tr: return "Sistem"
            case .en: return "System"
            case .fr: return "Système"
            case .it: return "Sistema"
            case .ru: return "Система"
            }
        case .themeLight:
            switch language {
            case .tr: return "Açık"
            case .en: return "Light"
            case .fr: return "Clair"
            case .it: return "Chiaro"
            case .ru: return "Светлая"
            }
        case .themeDark:
            switch language {
            case .tr: return "Koyu"
            case .en: return "Dark"
            case .fr: return "Sombre"
            case .it: return "Scuro"
            case .ru: return "Темная"
            }
        case .liquidGlassLabel:
            switch language {
            case .tr: return "Liquid Glass"
            case .en: return "Liquid Glass"
            case .fr: return "Verre Liquide"
            case .it: return "Vetro Liquido"
            case .ru: return "Жидкое стекло"
            }
        case .languageTitle:
            switch language {
            case .tr: return "Dil"
            case .en: return "Language"
            case .fr: return "Langue"
            case .it: return "Lingua"
            case .ru: return "Язык"
            }
        case .quitButton:
            switch language {
            case .tr: return "Çıkış"
            case .en: return "Quit"
            case .fr: return "Quitter"
            case .it: return "Esci"
            case .ru: return "Выход"
            }
        case .loadingSensors:
            switch language {
            case .tr: return "SMC Verileri Yükleniyor..."
            case .en: return "Loading SMC Metrics..."
            case .fr: return "Chargement des métriques SMC..."
            case .it: return "Caricamento metriche SMC..."
            case .ru: return "Загрузка данных SMC..."
            }
        }
    }
}

enum LocalizationKey {
    case headerTitle
    case readingSensors
    case sectionProcessor
    case cpuTitle
    case gpuTitle
    case maxPrefix
    case sectionOther
    case ramTitle
    case batteryTitle
    case socTitle
    case sectionCooling
    case fanlessTitle
    case fanlessSubtitle
    case silentMode
    case refreshLabel
    case themeLabel
    case themeSystem
    case themeLight
    case themeDark
    case liquidGlassLabel
    case languageTitle
    case quitButton
    case loadingSensors
}
