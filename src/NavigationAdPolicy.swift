import Foundation

// A presentation-only plan. Callers keep the original identifiers and delete
// from a snapshot copy, never from a conversation model or persistent store.
enum KCNavigationAdPolicy {
    private static let chatSections: Set<String> = [
        "KakaoTalk.ChatListSection",
        "KakaoTalk.ChatsTableViewController.ChatsTableSection",
        "KakaoTalk.ChannelChatsViewModel.Section"
    ]
    private static let openSection = "OpenLink.OpenChatsTableSection"
    private static let openItem = "OpenLink.OpenChatsTableItem"

    static func removals(
        sections: [(type: String, role: String)],
        items: [(type: String, role: String)]
    ) -> (sections: [Int], items: [Int])? {
        guard (1...256).contains(sections.count), items.count <= 50_000,
              let type = sections.first?.type,
              sections.allSatisfy({ $0.type == type }) else { return nil }
        if chatSections.contains(type) {
            // These models have a dedicated advertisement section. Their item
            // types are intentionally opaque because no item is removed here.
            let removed = sections.indices.filter { sections[$0].role == "bizboard" }
            guard removed.count <= 1 else { return nil }
            return (removed, [])
        }
        guard type == openSection,
              items.allSatisfy({ $0.type == openItem }) else { return nil }
        let advertisementSections: Set<String> = ["bizboard", "feed", "feedRecommends"]
        let advertisementItems: Set<String> = ["bizboard", "adFitBanner", "feedAD", "feed", "feedHidden"]
        // Keep chat/threadChat and penalty/unsupported identifiers intact. The
        // latter may represent a genuine service restriction or failure state.
        return (
            sections.indices.filter { advertisementSections.contains(sections[$0].role) },
            items.indices.filter { advertisementItems.contains(items[$0].role) }
        )
    }

    static func moreHomeIndex(_ roles: [(type: String, role: String)]) -> Int? {
        let known: Set<String> = ["home", "wallet", "game", "dynamic"]
        guard (1...4).contains(roles.count),
              roles.allSatisfy({ $0.type == "KakaoTalk.MoreTabContentType" && known.contains($0.role) }),
              Set(roles.map(\.role)).count == roles.count else { return nil }
        return roles.firstIndex(where: { $0.role == "home" })
    }
}
