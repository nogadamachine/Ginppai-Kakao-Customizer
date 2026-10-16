import Foundation
@_silgen_name("KCTestFriendAdSnapshot")
func nativeFilter(_ input: UnsafeMutableRawPointer, _ enabled: Bool, _ verified: Bool) -> UnsafeMutableRawPointer
// Exact runtime module/nesting name used by KakaoTalk's enum reflection.
final class FriendsListDataSource {
    enum SectionType: Hashable { case bizboard, updateProfile, newFriend, eventBanner, specialFriend, birthdayFriend, favoriteFriend, kanana, friend, futureSection }
}
enum OtherSection: Hashable { case bizboard, friend }
enum AssociatedSection: Hashable { case bizboard(Int) }
final class FixtureSnapshot: NSObject, NSCopying {
    var sections: [AnyHashable]
    var items: [AnyHashable: [String]]
    var brokenCopy = false
    var brokenDelete = false
    init(_ sections: [AnyHashable]) {
        self.sections=sections
        self.items=Dictionary(uniqueKeysWithValues:sections.map{($0,["fixture item for \($0)"])})
    }
    @objc var sectionIdentifiers: NSArray { sections as NSArray }
    @objc var numberOfSections: Int { sections.count }
    func copy(with zone: NSZone? = nil) -> Any {
        if brokenCopy { return self }
        let result=FixtureSnapshot(sections);result.items=items;result.brokenDelete=brokenDelete;return result
    }
    @objc(deleteSectionsWithIdentifiers:) func deleteSections(_ identifiers: NSArray) {
        if brokenDelete { return }
        let ids=Set(identifiers.compactMap{$0 as? AnyHashable})
        sections.removeAll{ids.contains($0)}
        for id in ids { items.removeValue(forKey:id) }
    }
}
@main enum FriendAdSnapshotTests {
    static func main() {
        typealias S=FriendsListDataSource.SectionType
        var checks=0
        func check(_ ok:Bool,_ name:String){checks+=1;if !ok{fatalError(name)}}
        func filter(_ snapshot:FixtureSnapshot,_ enabled:Bool=true,_ verified:Bool=true)->FixtureSnapshot {
            let result=nativeFilter(Unmanaged.passUnretained(snapshot).toOpaque(),enabled,verified)
            return Unmanaged<AnyObject>.fromOpaque(result).takeRetainedValue() as! FixtureSnapshot
        }
        let snapshot=FixtureSnapshot([S.bizboard,S.updateProfile,S.specialFriend,S.birthdayFriend,S.favoriteFriend,S.friend])
        let expected:[AnyHashable]=[S.updateProfile,S.birthdayFriend,S.favoriteFriend,S.friend]
        check(KCMessageBridge.friendAdSectionIndexes(snapshot.sections)==[0,2],"Actual Swift bridge finds native enum ad roles")
        let result=filter(snapshot)
        check(result !== snapshot && result.sections==expected,"Actual ObjC snapshot hook removes both complete ad sections")
        check(snapshot.sections.count==6 && snapshot.items.count==6,"Original snapshot not mutated")
        check(result.items.count==4 && expected.allSatisfy{result.items[$0]==snapshot.items[$0]},"Profile and friend items remain identical")
        check(filter(result)===result,"Repeated filtered snapshot is unchanged")
        check(filter(snapshot,false)===snapshot,"hideAds off bypasses filtering")
        check(filter(snapshot,true,false)===snapshot,"Unsupported build bypasses filtering")
        let reordered=FixtureSnapshot([S.friend,S.bizboard,S.futureSection])
        check(filter(reordered).sections==[AnyHashable(S.friend),AnyHashable(S.futureSection)],"Position-independent removal with future sections preserved")
        let only=FixtureSnapshot([S.bizboard]);check(filter(only).sections.isEmpty,"Ad-only snapshot becomes empty")
        let foreign=FixtureSnapshot([OtherSection.bizboard,OtherSection.friend]);check(filter(foreign)===foreign,"Foreign enum untouched")
        let associated=FixtureSnapshot([AssociatedSection.bizboard(1)]);check(filter(associated)===associated,"Associated-value/foreign enum untouched")
        let empty=FixtureSnapshot([]);check(filter(empty)===empty,"Empty snapshot safe")
        snapshot.brokenCopy=true;check(filter(snapshot)===snapshot,"Self-returning copy rejected");snapshot.brokenCopy=false
        snapshot.brokenDelete=true;check(filter(snapshot)===snapshot,"Unexpected delete result rejected");snapshot.brokenDelete=false
        print("Friend ad snapshot integration: \(checks) checks passed with real Swift enums, bridge and ObjC mutation")
    }
}
