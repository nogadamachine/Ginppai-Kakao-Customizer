import Foundation
import SwiftUI
import Darwin

// Rendering witnesses have one ABI for every View. Only these three entries
// are replaced; the app's Body metadata and body getter remain untouched.
// The exact app UUID is checked by the Objective-C caller before installation.
struct KCMoreSection {
    let key: String
    let typeName: String
    let mangledType: String
    let conformance: String
}
let kcMoreSections: [KCMoreSection] = [
    .init(key: "hideMorePay", typeName: "MoreTab.MoreTabKakaoPayView", mangledType: "7MoreTab0aB12KakaoPayViewV", conformance: "$s7MoreTab0aB12KakaoPayViewV7SwiftUI0E0AAMc"),
    .init(key: "hideMorePay", typeName: "MoreTab.MoreTabPayExperimentView", mangledType: "7MoreTab0aB17PayExperimentViewV", conformance: "$s7MoreTab0aB17PayExperimentViewV7SwiftUI0E0AAMc"),
    .init(key: "hideMoreNow", typeName: "MoreTab.MoreTabKakaoNowView", mangledType: "7MoreTab0aB12KakaoNowViewV", conformance: "$s7MoreTab0aB12KakaoNowViewV7SwiftUI0E0AAMc"),
    .init(key: "hideMoreWeather", typeName: "MoreTab.MoreTabWeatherView", mangledType: "7MoreTab0aB11WeatherViewV", conformance: "$s7MoreTab0aB11WeatherViewV7SwiftUI0D0AAMc"),
    .init(key: "hideMoreServices", typeName: "MoreTab.MoreTabGridView", mangledType: "7MoreTab0aB8GridViewV", conformance: "$s7MoreTab0aB8GridViewV7SwiftUI0D0AAMc"),
    .init(key: "hideMoreLinks", typeName: "MoreTab.MoreTabVerticalServicesView", mangledType: "7MoreTab0aB20VerticalServicesViewV", conformance: "$s7MoreTab0aB20VerticalServicesViewV7SwiftUI0E0AAMc"),
    .init(key: "hideAds", typeName: "MoreTab.MoreTabNativeADView", mangledType: "7MoreTab0aB12NativeADViewV", conformance: "$s7MoreTab0aB12NativeADViewV7SwiftUI4ViewAAMc"),
    .init(key: "hideAds", typeName: "MoreTab.MoreTabLocalBizboardView", mangledType: "7MoreTab0aB17LocalBizboardViewV", conformance: "$s7MoreTab0aB17LocalBizboardViewV7SwiftUI0E0AAMc"),
    .init(key: "hideAds", typeName: "MoreTab.MoreTabLocalBizboardContainerView", mangledType: "7MoreTab0aB26LocalBizboardContainerViewV", conformance: "$s7MoreTab0aB26LocalBizboardContainerViewV7SwiftUI0F0AAMc")
]
func kcMoreSectionEnabled(_ key: String, options: [String: Bool]) -> Bool {
    options[key] == true || (key == "hideMoreLinks" && options["hideMoreServices"] == true)
}
private struct KCRenderCanary: View { var body: some View { EmptyView() } }
func kcViewWitness(_ type: any View.Type) -> UnsafeMutablePointer<UInt>? {
    guard MemoryLayout.size(ofValue: type) == 2 * MemoryLayout<UInt>.size else { return nil }
    return withUnsafeBytes(of: type) { UnsafeMutablePointer<UInt>(bitPattern: $0.load(fromByteOffset: MemoryLayout<UInt>.size, as: UInt.self)) }
}
private func kcReadableRegion(_ pointer: UnsafeRawPointer, length: Int, writable: Bool) -> Bool {
    var address = vm_address_t(UInt(bitPattern: pointer)), size: vm_size_t = 0
    var info = vm_region_basic_info_data_64_t(), object: mach_port_t = 0
    var count = mach_msg_type_number_t(MemoryLayout.size(ofValue: info) / MemoryLayout<integer_t>.size)
    let result = withUnsafeMutablePointer(to: &info) {
        $0.withMemoryRebound(to: integer_t.self, capacity: Int(count)) {
            vm_region_64(mach_task_self_, &address, &size, VM_REGION_BASIC_INFO_64, $0, &count, &object)
        }
    }
    if object != 0 { mach_port_deallocate(mach_task_self_, object) }
    let start = UInt(bitPattern: pointer)
    guard result == KERN_SUCCESS, info.protection & VM_PROT_READ != 0,
          start >= address, UInt(length) <= size, start - address <= size - UInt(length) else { return false }
    // Never make read-only or executable memory writable. Unsupported layouts
    // are left alone and the corresponding setting is disabled.
    return !writable || (info.protection & VM_PROT_WRITE != 0 && info.protection & VM_PROT_EXECUTE == 0)
}
func kcRenderSlotsAvailable(_ table: UnsafeMutablePointer<UInt>, descriptor: UInt) -> Bool {
    kcReadableRegion(UnsafeRawPointer(table), length: 7 * MemoryLayout<UInt>.size, writable: true) &&
    table[0] == descriptor && (3...5).allSatisfy { table[$0] != 0 }
}
func kcReplaceRenderSlots(_ table: UnsafeMutablePointer<UInt>, descriptor: UInt, replacements: [UInt]) -> Bool {
    guard replacements.count == 3, replacements.allSatisfy({ $0 != 0 }), kcRenderSlotsAvailable(table, descriptor: descriptor) else { return false }
    for index in 3...5 { table[index] = replacements[index - 3] }
    return (3...5).allSatisfy { table[$0] == replacements[$0 - 3] }
}
private func kcRenderLayoutAvailable() -> Bool {
    guard let table = kcViewWitness(KCRenderCanary.self),
          kcReadableRegion(UnsafeRawPointer(table), length: 7 * MemoryLayout<UInt>.size, writable: false) else { return false }
    let names = (3...6).map { index -> String in
        var info = Dl_info()
        guard dladdr(UnsafeRawPointer(bitPattern: table[index]), &info) != 0 else { return "" }
        return info.dli_sname.map { String(cString: $0) } ?? ""
    }
    return names[0].contains("_make") && names[1].contains("List") && names[2].contains("Count") && names[3].contains("body")
}
@objc(KCMoreTabRendering) final class KCMoreTabRendering: NSObject {
    @objc static func install(_ preferences: NSDictionary) -> NSDictionary {
        var options = Dictionary(uniqueKeysWithValues: Set(kcMoreSections.map(\.key)).map { ($0, (preferences[$0] as? NSNumber)?.boolValue == true) })
        var capabilities = Dictionary(uniqueKeysWithValues: Set(kcMoreSections.map(\.key)).map { ($0, false) })
        var installed = [String]()
        guard kcRenderLayoutAvailable(), let empty = kcViewWitness(EmptyView.self),
              kcReadableRegion(UnsafeRawPointer(empty), length: 7 * MemoryLayout<UInt>.size, writable: false) else {
            return ["capabilities": capabilities, "installed": installed]
        }
        let replacements = (3...5).map { empty[$0] }
        var resolved = [String: [(KCMoreSection, UnsafeMutablePointer<UInt>, UInt)]]()
        for key in Set(kcMoreSections.map(\.key)) {
            let sections = kcMoreSections.filter { $0.key == key }
            var targets = [(KCMoreSection, UnsafeMutablePointer<UInt>, UInt)]()
            for section in sections {
                guard let type = _typeByName(section.mangledType) as? any View.Type,
                      String(reflecting: type) == section.typeName, let table = kcViewWitness(type),
                      let descriptor = dlsym(UnsafeMutableRawPointer(bitPattern: -2), section.conformance),
                      kcRenderSlotsAvailable(table, descriptor: UInt(bitPattern: descriptor)) else { continue }
                targets.append((section, table, UInt(bitPattern: descriptor)))
            }
            guard targets.count == sections.count else { continue }
            capabilities[key] = true
            resolved[key] = targets
        }
        // Hiding the full service group includes the alternate vertical layout.
        capabilities["hideMoreServices"] = capabilities["hideMoreServices"] == true && capabilities["hideMoreLinks"] == true
        if capabilities["hideMoreServices"] != true { options["hideMoreServices"] = false }
        for (key, targets) in resolved where capabilities[key] == true && kcMoreSectionEnabled(key, options: options) {
            for (section, table, descriptor) in targets {
                if kcReplaceRenderSlots(table, descriptor: descriptor, replacements: replacements) { installed.append(section.typeName) }
                else { capabilities[key] = false }
            }
        }
        return ["capabilities": capabilities, "installed": installed]
    }
}
