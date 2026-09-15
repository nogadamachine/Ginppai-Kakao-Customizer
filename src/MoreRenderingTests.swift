import SwiftUI
import AppKit
import Darwin

private struct HiddenFixture: View { var body: some View { Color.red.frame(width: 180, height: 120) } }
private struct KeptFixture: View { var body: some View { Color.green.frame(width: 180, height: 50) } }
@main struct MoreRenderingTests {
    @MainActor static func main() {
        var passed = 0
        func check(_ condition: @autoclosure () -> Bool, line: UInt = #line) { precondition(condition(), "Rendering check at line \(line)"); passed += 1 }
        func size() -> CGSize { NSHostingView(rootView: VStack(spacing: 0) { HiddenFixture(); KeptFixture() }).fittingSize }
        check(size() == CGSize(width: 180, height: 170))
        let target = kcViewWitness(HiddenFixture.self)!, sibling = kcViewWitness(KeptFixture.self)!, empty = kcViewWitness(EmptyView.self)!
        let original = (0...6).map { target[$0] }, siblingOriginal = (0...6).map { sibling[$0] }
        let replacements = (3...5).map { empty[$0] }
        check(!kcReplaceRenderSlots(target, descriptor: original[0] + 1, replacements: replacements))
        check((0...6).map { target[$0] } == original)
        check(!kcReplaceRenderSlots(target, descriptor: original[0], replacements: [0, 0, 0]))
        check(!kcReplaceRenderSlots(target, descriptor: original[0], replacements: [1]))
        check(kcReplaceRenderSlots(target, descriptor: original[0], replacements: replacements))
        check((0...6).filter { target[$0] != original[$0] } == [3, 4, 5])
        check((0...6).map { sibling[$0] } == siblingOriginal)
        check(size() == CGSize(width: 180, height: 50))
        check(kcReplaceRenderSlots(target, descriptor: original[0], replacements: Array(original[3...5])))
        check(size() == CGSize(width: 180, height: 170))
        let pageSize = Int(getpagesize()), mapping = mmap(nil, pageSize, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0)!
        precondition(mapping != MAP_FAILED)
        let readOnly = mapping.bindMemory(to: UInt.self, capacity: 7)
        for index in 0...6 { readOnly[index] = original[index] }
        precondition(mprotect(mapping, pageSize, PROT_READ) == 0)
        check(!kcReplaceRenderSlots(readOnly, descriptor: original[0], replacements: replacements))
        check((0...6).map { readOnly[$0] } == original)
        munmap(mapping, pageSize)
        check(!kcMoreSectionEnabled("hideMoreNow", options: [:]))
        check(kcMoreSectionEnabled("hideMoreNow", options: ["hideMoreNow": true]))
        check(!kcMoreSectionEnabled("hideMoreWeather", options: ["hideMoreNow": true]))
        check(kcMoreSectionEnabled("hideMoreLinks", options: ["hideMoreServices": true]))
        check(!kcMoreSectionEnabled("hideMoreServices", options: ["hideMoreLinks": true]))
        check(kcMoreSectionEnabled("hideMoreLinks", options: ["hideMoreLinks": true]))
        check(Set(kcMoreSections.map(\.typeName)).count == kcMoreSections.count)
        let unavailable = KCMoreTabRendering.install(["countryISO": "JP", "hideMoreNow": true] as NSDictionary)
        check((unavailable["installed"] as? [String]) == [])
        check((unavailable["capabilities"] as? [String: Bool])?.values.allSatisfy { !$0 } == true)
        print("Passed \(passed) More rendering checks including actual SwiftUI layout, isolation and restoration")
    }
}
