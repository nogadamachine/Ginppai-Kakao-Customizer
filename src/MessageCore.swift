import Foundation
import CoreFoundation
import Darwin

// Native iOS implementation. Third-party source attribution is in NOTICE.md.
// Only the selected message record is reflected. Account stores, credentials,
// delegates and arbitrary object graphs are never traversed.
enum KCRecord {
    static func unwrap(_ value: Any) -> Any? {
        let mirror = Mirror(reflecting: value)
        if mirror.displayStyle != .optional { return value }
        return mirror.children.first.flatMap { unwrap($0.value) }
    }
    static func field(_ object: Any, _ name: String) -> Any? {
        guard let value = unwrap(object) else { return nil }
        var mirror: Mirror? = Mirror(reflecting: value)
        while let current = mirror {
            if let child = current.children.first(where: { $0.label == name }) {
                return unwrap(child.value)
            }
            mirror = current.superclassMirror
        }
        return nil
    }
    static func json(_ source: Any, depth: Int = 0) -> Any {
        guard depth < 12, let value = unwrap(source) else { return NSNull() }
        if let s = value as? String { return String(s.prefix(262_144)) }
        if let n = value as? NSNumber { return n }
        if let d = value as? Date { return ISO8601DateFormatter().string(from: d) }
        if let u = value as? URL { return u.absoluteString }
        if let d = value as? Data { return ["byteCount": d.count] }
        if let raw = value as? any RawRepresentable { return json(raw.rawValue, depth: depth + 1) }
        let mirror = Mirror(reflecting: value)
        if mirror.displayStyle == .collection || mirror.displayStyle == .set {
            return mirror.children.prefix(2048).map { json($0.value, depth: depth + 1) }
        }
        if mirror.displayStyle == .dictionary {
            var result = [String: Any]()
            for child in mirror.children.prefix(2048) {
                let pair = Array(Mirror(reflecting: child.value).children)
                guard pair.count == 2 else { continue }
                let key = pair[0].value as? String ?? (pair[0].value as? NSNumber)?.stringValue
                if let key { result[key] = json(pair[1].value, depth: depth + 1) }
            }
            return result
        }
        let name = String(reflecting: type(of: value))
        // MessageRecord, message status enums and their value fields only.
        if mirror.displayStyle == .class && depth > 0 { return ["nativeType": name] }
        var result = [String: Any]()
        var fields = [Mirror.Child]()
        var level: Mirror? = mirror
        while let current = level, fields.count < 128 {
            fields.append(contentsOf: current.children.prefix(128 - fields.count))
            level = current.superclassMirror
        }
        for (index, child) in fields.enumerated() {
            if let label = child.label, result[label] != nil { continue }
            result[child.label ?? "value\(index)"] = json(child.value, depth: depth + 1)
        }
        if result.isEmpty { return ["nativeType": name] }
        return result
    }
    static func identifier(_ value: Any?) -> String? {
        guard let value else { return nil }
        if let s = value as? String, let number = Int64(s) { return String(number) }
        guard let n = value as? NSNumber,
              CFGetTypeID(n) != CFBooleanGetTypeID(),
              let number = Int64(n.stringValue) else { return nil }
        return String(number)
    }
    static func snapshot(_ object: Any) -> [String: Any]? {
        guard let record = field(object, "record"),
              String(reflecting: type(of: record)) == "TalkAppBase.MessageRecord",
              let fields = json(record) as? [String: Any],
              let chatID = identifier(fields["chatID"]),
              let logID = identifier(fields["serverLogID"]),
              let senderID = identifier(fields["userID"]) else { return nil }
        var result: [String: Any] = ["nativeType": String(reflecting: type(of: object)),
            "chatID": chatID, "logID": logID, "senderID": senderID, "record": fields]
        if let attachment = field(object, "attachmentCache") {
            result["decodedAttachment"] = json(attachment)
        }
        // Do not fall back to record.message: it may be ciphertext.
        if let text = field(record, "decryptedMessage") as? String {
            result["text"] = String(text.prefix(262_144))
        }
        return result
    }
}

enum KCReadReceipts {
    static func unreadCount(logID: Int64, userID: Int64, readMarks: [Int64], writerIndices: [Int64], bypass: Bool, lastReadLogID: Int64) -> Int {
        let writer = writerIndices.firstIndex(of: userID)
        var count = bypass && lastReadLogID < logID ? 1 : 0
        for (index, mark) in readMarks.enumerated() where index != writer && mark < logID { count += 1 }
        return count
    }
    static func decode(_ native: Any, memberIDs: [Any]) -> [String: Int64]? {
        var result = [String: Int64]()
        if let values = native as? [Any] {
            // The verified iOS getter returns [Int64]. Its native updater finds
            // an index in activeMemberIDs, then reads/writes readMarks[index].
            // Preserve indices even when an individual field is invalid.
            guard values.count <= memberIDs.count else { return nil }
            var ambiguous = Set<String>()
            for (index, value) in values.enumerated() {
                guard let id = KCRecord.identifier(memberIDs[index]), (Int64(id) ?? 0) > 0,
                      let number = KCRecord.identifier(value), let mark = Int64(number), mark >= 0 else { continue }
                if result[id] != nil || ambiguous.contains(id) {
                    result.removeValue(forKey: id);ambiguous.insert(id)
                } else { result[id] = mark }
            }
            return result
        }
        if let values = native as? NSDictionary {
            for (key, value) in values {
                guard let id = KCRecord.identifier(key), (Int64(id) ?? 0) > 0,
                      let number = KCRecord.identifier(value), let mark = Int64(number), mark >= 0 else { continue }
                result[id] = mark
            }
            return result
        }
        return nil
    }
    // A missing watermark is unknown, never fabricated as an unread member.
    // Sender and the current user are read, matching upstream's classification.
    static func classify(logID: Int64, senderID: String, myID: String,
                         memberIDs: [String], watermarks: [String: Int64],
                         activeCount: Int) -> [String: Any] {
        var read = [String](), unread = [String](), unknown = [String]()
        let ids = Set(memberIDs).union(watermarks.keys).union([senderID, myID]).filter { (Int64($0) ?? 0) > 0 }.sorted()
        for id in ids {
            if id == senderID || id == myID { read.append(id) }
            else if let mark = watermarks[id] {
                if mark >= logID { read.append(id) } else { unread.append(id) }
            } else { unknown.append(id) }
        }
        return ["read": read, "unread": unread, "unknown": unknown,
                "missingMembers": max(0, activeCount - ids.count)]
    }
}

enum KCTrackerPolicy {
    // Endpoints verified in the iOS Tiara binary and the pinned SDK tracker patch.
    // Never use substring matching: credentials and lookalike domains must not
    // cause an unrelated request to be intercepted.
    static func blocks(_ url: URL) -> Bool {
        guard ["https", "http"].contains(url.scheme?.lowercased() ?? ""),
              let host = url.host?.lowercased() else { return false }
        return ["ad.daum.net", "stat.tiara.daum.net", "sandbox-stat.tiara.daum.net"].contains(host)
    }
    static func blocksShareLog(_ url: URL, endpoint: URL?) -> Bool {
        guard let endpoint, endpoint.scheme == "https", let host = endpoint.host, !host.isEmpty, endpoint.user == nil, endpoint.password == nil,
              endpoint.query == nil, endpoint.fragment == nil,
              endpoint.path.hasSuffix("/talk_share/log.json"),
              url.scheme == endpoint.scheme, url.host?.lowercased() == endpoint.host?.lowercased(),
              url.port == endpoint.port, url.user == nil, url.password == nil else { return false }
        return url.path == endpoint.path
    }
    // The app's exported getter supplies its actual configured host and API
    // prefix. No broad kakao.com filter and no payload inspection are needed.
    // This getter builds a URL string; it performs no network request.
    static func nativeShareLogEndpoint(_ native: String) -> URL? {
        // This API getter returns host/path on iOS 26.7.3. Its caller adds
        // https:// before constructing the request. Preserve that contract.
        let absolute = native.contains("://") ? native : "https://" + native
        guard let endpoint = URL(string: absolute), blocksShareLog(endpoint, endpoint: endpoint) else { return nil }
        return endpoint
    }
    static var shareLogEndpoint: URL? {
        // The app configures its API host after tweak initialization. Resolve
        // it when matching a request so an early placeholder never stays cached.
        guard let symbol = dlsym(UnsafeMutableRawPointer(bitPattern: -2), "$s11TalkNetwork6APIURLV12talkShareLogSSvgZ") else { return nil }
        let getter = unsafeBitCast(symbol, to: (@convention(thin) () -> String).self)
        return nativeShareLogEndpoint(getter())
    }
}

enum KCLayoutPolicy {
    static func specialFriendSectionIndex(_ sections: [(type: String, name: String)]) -> Int? {
        guard (1...32).contains(sections.count), sections.allSatisfy({ $0.type == "FriendsFeedPresentation.FriendsListDataSource.SectionType" }) else { return nil }
        let matches = sections.indices.filter { sections[$0].name == "specialFriend" }
        return matches.count == 1 ? matches[0] : nil
    }
    static func friendChipRole(typeName: String, caseName: String) -> Int {
        guard typeName.hasPrefix("FriendsFeedPresentation.FriendsTabHeaderView."),
              typeName.hasSuffix(".ChipItem") else { return -1 }
        return caseName == "list" ? 0 : (caseName == "feed" ? 1 : -1)
    }
    static func friendListIndex(_ roles: [Int]) -> Int? {
        guard (1...2).contains(roles.count), roles.filter({ $0 == 0 }).count == 1,
              roles.allSatisfy({ $0 == 0 || $0 == 1 }) else { return nil }
        return roles.firstIndex(of: 0)
    }
}

struct KCHistoryEntry: Codable, Equatable {
    var chatID: String
    var logID: String
    var senderID: String
    var observedAt: TimeInterval
    var revisions: [KCHistoryRevision]
}
struct KCHistoryRevision: Codable, Equatable {
    var text: String
    var type: Int
    var observedAt: TimeInterval
    var attachment: String? = nil
}
enum KCHistoryPolicy {
    static let maxEntries = 5000
    static let maxRevisions = 20
    static func attachmentJSON(_ value: Any?) -> String? {
        guard var value else { return nil }
        if var fields = value as? [String: Any] {
            // Local send progress is added after an upload. It is not a
            // server-side edit to the message or its media attachment.
            fields.removeValue(forKey: "sendingInfo")
            value = fields
        }
        guard JSONSerialization.isValidJSONObject(value),
              let data = try? JSONSerialization.data(withJSONObject: value, options: [.sortedKeys]),
              data.count <= 65_536 else { return nil }
        return String(data: data, encoding: .utf8)
    }
    static func merge(_ old: KCHistoryEntry?, chatID: String, logID: String,
                      senderID: String, text: String, type: Int, now: TimeInterval, attachment: String? = nil) -> KCHistoryEntry {
        var entry = old ?? KCHistoryEntry(chatID: chatID, logID: logID, senderID: senderID, observedAt: now, revisions: [])
        let text = String(text.prefix(16_384))
        let attachment = attachment ?? entry.revisions.last?.attachment
        if let last = entry.revisions.last, last.type == type,
           last.text.isEmpty, !text.isEmpty,
           last.attachment == nil || last.attachment == attachment {
            entry.revisions[entry.revisions.count-1].text = text
            entry.revisions[entry.revisions.count-1].attachment = attachment
            return entry
        }
        if let last = entry.revisions.last, last.text == text, last.type == type {
            if last.attachment == attachment { return entry }
            if last.attachment == nil {
                // A decoded attachment can become available after the native
                // save callback. Enrich that revision without inventing an edit.
                entry.revisions[entry.revisions.count-1].attachment = attachment
                return entry
            }
        }
        entry.observedAt = now
        entry.revisions.append(KCHistoryRevision(text: text, type: type, observedAt: now, attachment: attachment))
        if entry.revisions.count > maxRevisions {
            // Keep the first received original and the most recent revisions.
            entry.revisions = [entry.revisions[0]] + entry.revisions.suffix(maxRevisions - 1)
        }
        return entry
    }
}

#if canImport(ObjectiveC)
@_silgen_name("KCGinppaiUnreadCalculator")
func ginppaiUnreadCalculator(_ logID: Int64, _ userID: Int64, _ readMarks: [Int64], _ writerIndices: [Int64], _ bypass: Bool, _ lastReadLogID: Int64) -> Int {
    KCReadReceipts.unreadCount(logID: logID, userID: userID, readMarks: readMarks, writerIndices: writerIndices, bypass: bypass, lastReadLogID: lastReadLogID)
}
@objc(KCMessageBridge) public final class KCMessageBridge: NSObject {
    @objc public static func nativeCoverProbe(_ address: UInt) -> NSDictionary {
        typealias Function = @convention(thin) (Bool, Bool, Bool, Bool, Bool, Bool) -> UInt8
        let function = unsafeBitCast(address, to: Function.self)
        return ["mobileOnly": function(true,true,false,true,false,false),
                "subdeviceConsent": function(true,true,true,false,false,false),
                "locked": function(true,true,false,false,true,false),
                "adultLocked": function(true,true,false,false,false,true),
                "invalid": function(false,true,false,false,false,false),
                "invalidVersion": function(true,false,false,false,false,false)]
    }
    @objc public static func nativeMenuHandler(_ object: AnyObject) -> NSObject? {
        guard String(reflecting: type(of: object)).hasPrefix("ChatBubble."),
              let coordinator = KCRecord.field(object, "coordinator"),
              ["KakaoTalk.TalkSentBubbleCellCoordinator", "KakaoTalk.TalkReceivedBubbleCellCoordinator",
               "KakaoTalk.TalkReceivedMimicBubbleCellCoordinator"].contains(String(reflecting: type(of: coordinator))) else { return nil }
        let handler = KCRecord.field(coordinator, "chatSubmenuHandler") ?? KCRecord.field(coordinator, "mimicChatSubmenuHandler")
        guard let handler = handler as? NSObject,
              ["KakaoTalk.ChatMessageCellViewSubmenuHandler", "KakaoTalk.ChatMessageCellViewMimicSubmenuHandler"].contains(String(reflecting: type(of: handler))) else { return nil }
        return handler
    }
    @objc public static func nativeUnreadProbe(_ address: UInt) -> NSDictionary {
        // Called only after the exact app UUID and leaf function bytes have
        // been verified. All inputs are synthetic; no chat record is accessed.
        typealias Calculator = @convention(thin) (Int64, Int64, [Int64], [Int64], Bool, Int64) -> Int
        let calculate = unsafeBitCast(address, to: Calculator.self)
        var result = [String: Int]()
        for count in [98, 99, 100, 350] {
            result[String(count)] = calculate(100, 9999, Array(repeating: 0, count: count), (1...count).map(Int64.init), false, 100)
        }
        result["mixed"] = calculate(100, 2, [0, 100, 99, 101], [1, 2, 3, 4], false, 100)
        return result as NSDictionary
    }
    @objc public static func bubbleMessage(_ object: AnyObject) -> AnyObject? {
        let name = String(reflecting: type(of: object))
        if name.hasPrefix("ChatBubble.") {
            guard let coordinator = KCRecord.field(object, "coordinator"),
                  ["KakaoTalk.TalkSentBubbleCellCoordinator", "KakaoTalk.TalkReceivedBubbleCellCoordinator",
                   "KakaoTalk.TalkReceivedMimicBubbleCellCoordinator", "KakaoTalk.TalkSystemBubbleCellCoordinator",
                   "KakaoTalk.TalkRichFeedBubbleCellCoordinator"].contains(String(reflecting: type(of: coordinator))) else { return nil }
            if let message = KCRecord.field(coordinator, "message") { return message as AnyObject }
            // Sent cells keep the message in the native submenu handler, while
            // received cells keep it directly on the coordinator.
            guard let handler = KCRecord.field(coordinator, "chatSubmenuHandler") as? NSObject,
                  String(reflecting: type(of: handler)) == "KakaoTalk.ChatMessageCellViewSubmenuHandler",
                  handler.responds(to: NSSelectorFromString("message")) else { return nil }
            return handler.perform(NSSelectorFromString("message"))?.takeUnretainedValue()
        }
        guard name.hasPrefix("KakaoTalk."), let message = KCRecord.field(object, "message") else { return nil }
        return message as AnyObject
    }
    @objc public static func isTrackerURL(_ url: URL) -> Bool { KCTrackerPolicy.blocks(url) }
    @objc public static func isShareLogURL(_ url: URL) -> Bool {
        guard url.path.hasSuffix("/talk_share/log.json") else { return false }
        return KCTrackerPolicy.blocksShareLog(url, endpoint: KCTrackerPolicy.shareLogEndpoint)
    }
    @objc public static func shareLogAvailable() -> Bool { KCTrackerPolicy.shareLogEndpoint != nil }
    @objc public static func friendChipRole(_ object: Any) -> Int {
        let value = (object as? AnyHashable)?.base ?? object
        let mirror = Mirror(reflecting: value)
        guard mirror.displayStyle == .enum, mirror.children.isEmpty else { return -1 }
        return KCLayoutPolicy.friendChipRole(typeName: String(reflecting: type(of: value)), caseName: String(describing: value))
    }
    @objc public static func friendListIndex(_ objects: [Any]) -> Int {
        guard (1...2).contains(objects.count) else { return -1 }
        return KCLayoutPolicy.friendListIndex(objects.map(friendChipRole)) ?? -1
    }
    @objc public static func specialFriendSectionIndex(_ objects: [Any]) -> Int {
        guard (1...32).contains(objects.count) else { return -1 }
        var sections = [(type: String, name: String)]()
        for object in objects {
            let value = (object as? AnyHashable)?.base ?? object, mirror = Mirror(reflecting: (object as? AnyHashable)?.base ?? object)
            guard mirror.displayStyle == .enum, mirror.children.isEmpty else { return -1 }
            let name = String(reflecting: type(of: value))
            guard name == "FriendsFeedPresentation.FriendsListDataSource.SectionType" else { return -1 }
            sections.append((name, String(describing: value)))
        }
        return KCLayoutPolicy.specialFriendSectionIndex(sections) ?? -1
    }
    @objc public static func friendSelectedIndexPath(_ object: AnyObject) -> NSIndexPath? {
        guard String(reflecting: type(of: object)) == "FriendsFeedPresentation.FriendsTabHeaderView",
              let index = KCRecord.field(object, "currentSelectedChipIndexPath") as? IndexPath else { return nil }
        return index as NSIndexPath
    }
    @objc public static func snapshot(_ object: AnyObject) -> NSDictionary? {
        KCRecord.snapshot(object) as NSDictionary?
    }
    @objc public static func fieldObject(_ object: AnyObject, name: String) -> AnyObject? {
        // Callers pass a fixed native field name, never user input.
        let allowed = ["message", "messageProvider", "chat", "inputBar", "textView", "messageAttachment"]
        guard allowed.contains(name), let value = KCRecord.field(object, name) else { return nil }
        return value as AnyObject
    }
    @objc public static func readReceipts(_ input: NSDictionary) -> NSDictionary {
        let rawMembers = input["members"] as? [Any] ?? []
        guard let watermarks = KCReadReceipts.decode(input["watermarks"] ?? [], memberIDs: rawMembers) else {
            return ["error": "이 대화의 읽음 정보를 아직 해석할 수 없습니다."]
        }
        let members = rawMembers.compactMap { KCRecord.identifier($0) }
        return KCReadReceipts.classify(logID: (input["logID"] as? NSNumber)?.int64Value ?? 0,
            senderID: input["senderID"] as? String ?? "", myID: input["myID"] as? String ?? "",
            memberIDs: members, watermarks: watermarks,
            activeCount: (input["activeCount"] as? NSNumber)?.intValue ?? members.count) as NSDictionary
    }
}

@objc(KCMessageHistory) public final class KCMessageHistory: NSObject {
    private static let queue = DispatchQueue(label: "com.nogadamachine.ginppai.history")
    private static var entries: [String: KCHistoryEntry]?
    private static var flushPending = false
    private static var lastError: String?
    private static var file: URL {
        FileManager.default.urls(for: .applicationSupportDirectory, in: .userDomainMask)[0]
            .appendingPathComponent("GinppaiKakao", isDirectory: true).appendingPathComponent("history.json")
    }
    private static func loadEntries() {
        guard entries == nil else { return }
        entries = [:]
        guard FileManager.default.fileExists(atPath: file.path) else { return }
        do {
            let data = try Data(contentsOf: file)
            guard data.count <= 64 * 1024 * 1024 else { throw CocoaError(.fileReadTooLarge) }
            entries = try JSONDecoder().decode([String: KCHistoryEntry].self, from: data)
        } catch { lastError = "기록 파일을 읽지 못했습니다. 기존 파일은 보존됩니다." }
    }
    @objc public static func observe(_ snapshot: NSDictionary) {
        guard let chatID = snapshot["chatID"] as? String, let logID = snapshot["logID"] as? String,
              (Int64(logID) ?? 0) > 0, let senderID = snapshot["senderID"] as? String,
              let record = snapshot["record"] as? NSDictionary,
              let type = record["type"] as? NSNumber else { return }
        let incomingText = snapshot["text"] as? String
        var attachment = KCHistoryPolicy.attachmentJSON(snapshot["decodedAttachment"])
        // Media attachment caches can be empty until decoding completes.
        // An empty text attachment remains meaningful (e.g. Markdown removed).
        if type.intValue != 1 && attachment == "{}" { attachment = nil }
        let decodedAttachment = attachment
        queue.async {
            loadEntries()
            guard lastError == nil else { return }
            let key = chatID + ":" + logID, old = entries?[key]
            guard incomingText != nil || old != nil else { return }
            let text = incomingText ?? old?.revisions.last?.text ?? ""
            let next = KCHistoryPolicy.merge(old, chatID: chatID, logID: logID, senderID: senderID,
                                             text: text, type: type.intValue, now: Date().timeIntervalSince1970, attachment: decodedAttachment)
            guard next != old else { return }
            entries?[key] = next
            if let all = entries, all.count > KCHistoryPolicy.maxEntries {
                let keep = all.sorted { $0.value.observedAt > $1.value.observedAt }.prefix(KCHistoryPolicy.maxEntries)
                entries = Dictionary(uniqueKeysWithValues: keep.map { ($0.key, $0.value) })
            }
            if let all = entries {
                func cost(_ entry: KCHistoryEntry) -> Int { entry.revisions.reduce(256) { $0 + $1.text.utf8.count + ($1.attachment?.utf8.count ?? 0) + 128 } }
                var bytes = all.values.reduce(0) { $0 + cost($1) }
                if bytes > 12 * 1024 * 1024 {
                    for pair in all.sorted(by: { $0.value.observedAt < $1.value.observedAt }) {
                        guard bytes > 12 * 1024 * 1024 else { break }
                        entries?.removeValue(forKey: pair.key); bytes -= cost(pair.value)
                    }
                }
            }
            guard !flushPending else { return }
            flushPending = true
            queue.asyncAfter(deadline: .now() + 0.8) {
                flushPending = false
                do {
                    let directory = file.deletingLastPathComponent()
                    try FileManager.default.createDirectory(at: directory, withIntermediateDirectories: true)
                    var values = URLResourceValues(); values.isExcludedFromBackup = true
                    var folder = directory; try folder.setResourceValues(values)
                    var data = try JSONEncoder().encode(entries ?? [:])
                    if data.count > 12 * 1024 * 1024 {
                        let oldest = (entries ?? [:]).sorted { $0.value.observedAt < $1.value.observedAt }
                        for group in stride(from: 0, to: oldest.count, by: 100) {
                            for pair in oldest[group..<min(group+100,oldest.count)] { entries?.removeValue(forKey: pair.key) }
                            data = try JSONEncoder().encode(entries ?? [:])
                            if data.count <= 12 * 1024 * 1024 { break }
                        }
                    }
                    #if os(iOS)
                    try data.write(to: file, options: [.atomic, .completeFileProtectionUntilFirstUserAuthentication])
                    #else
                    try data.write(to: file, options: .atomic)
                    #endif
                } catch { lastError = "메시지 기록을 저장하지 못했습니다." }
            }
        }
    }
    @objc public static func history(chatID: String, logID: String) -> NSArray {
        queue.sync {
            loadEntries()
            return (entries?[chatID + ":" + logID]?.revisions ?? []).map {
                var record: [String: Any] = ["text": $0.text, "type": $0.type, "observedAt": $0.observedAt]
                if let attachment = $0.attachment { record["attachment"] = attachment }
                return record as NSDictionary
            } as NSArray
        }
    }
    @objc public static func status() -> NSDictionary {
        queue.sync { loadEntries(); return ["messageCount": entries?.count ?? 0, "error": lastError ?? ""] }
    }
    @objc public static func recentEntries() -> NSArray {
        queue.sync {
            loadEntries()
            return (entries ?? [:]).values.sorted { $0.observedAt > $1.observedAt }.prefix(500).map {
                ["chatID": $0.chatID, "logID": $0.logID, "senderID": $0.senderID,
                 "preview": String(($0.revisions.first?.text ?? "").prefix(160)),
                 "revisions": $0.revisions.count, "observedAt": $0.observedAt] as NSDictionary
            } as NSArray
        }
    }
}
#endif
