import Foundation

class RecordIdentifiers {
    let chatID: Int64
    let serverLogID: Int64
    let userID: Int64
    init(chatID: Int64, serverLogID: Int64, userID: Int64) {
        self.chatID=chatID;self.serverLogID=serverLogID;self.userID=userID
    }
}
final class MessageRecord: RecordIdentifiers {
    let type: Int32
    let message: String
    let decryptedMessage: String?
    let attachment: String
    init(chatID: Int64, serverLogID: Int64, userID: Int64, type: Int32, message: String, decryptedMessage: String?, attachment: String) {
        self.type=type;self.message=message;self.decryptedMessage=decryptedMessage;self.attachment=attachment
        super.init(chatID:chatID,serverLogID:serverLogID,userID:userID)
    }
}
class ChatMessage: NSObject {
    let record: MessageRecord
    let attachmentCache: [String: Any]?
    // A record inspector must not leak an unrelated field on the parent object.
    let accountToken = "fixture-private-account-value"
    init(_ text: String?) {
        record = MessageRecord(chatID: 12, serverLogID: 9_007_199_254_740_999,
                               userID: 34, type: 1, message: "ciphertext-fixture",
                               decryptedMessage: text, attachment: "{}")
        attachmentCache = ["markdown": true, "ids": [12, 34]]
    }
}

enum TestMessageKind: Int32 { case text = 1, removed = 26 }

@main struct MessageTests {
    static func main() throws {
        var passed = 0
        func check(_ value: @autoclosure () -> Bool, _ name: String) {
            precondition(value(), name);passed += 1
        }
        let message = ChatMessage("original")
        let snapshot = KCRecord.snapshot(message)!
        check(snapshot["logID"] as? String == "9007199254740999", "64-bit log IDs remain exact")
        check((snapshot["record"] as? [String:Any])?["chatID"] as? Int64 == 12, "Include inherited record identity fields")
        check(snapshot["text"] as? String == "original", "Use decrypted message")
        check(KCRecord.snapshot(ChatMessage(nil))?["text"] == nil, "Never display ciphertext as original text")
        let encoded = try JSONSerialization.data(withJSONObject: snapshot, options: .sortedKeys)
        check(!String(decoding: encoded, as: UTF8.self).contains("fixture-private-account-value"), "Do not traverse account properties")
        check(KCRecord.snapshot(NSObject()) == nil, "Reject unrelated runtime objects")
        let optional: String? = nil
        check(KCRecord.unwrap(optional as Any) == nil, "Optional nil is not an object")
        check((KCRecord.json(TestMessageKind.removed) as? NSNumber)?.intValue == 26, "Native raw-value message state remains numeric")
        check(KCRecord.identifier(NSNumber(value: true)) == nil, "Boolean is not a user ID")
        check(KCRecord.identifier(NSNumber(value: 1.5)) == nil, "Fractional values are not IDs")
        check(KCRecord.identifier("00012") == "12", "Normalize integer IDs before comparison")

        let receipts = KCReadReceipts.classify(logID: 100, senderID: "1", myID: "2",
            memberIDs: ["1", "2", "3", "4", "5", "6"],
            watermarks: ["1": 0, "3": 100, "4": 99, "5": 0], activeCount: 7)
        check(Set(receipts["read"] as! [String]) == ["1", "2", "3"], "Sender, current user and equal watermark count as read")
        check(Set(receipts["unread"] as! [String]) == ["4", "5"], "Watermark zero is unread")
        check(receipts["unknown"] as! [String] == ["6"], "Missing watermark is unknown")
        check(receipts["missingMembers"] as! Int == 1, "Report uncached participants separately")
        let invalidIDs = KCReadReceipts.classify(logID: 100, senderID: "0", myID: "-1", memberIDs: ["not-an-id", "0", "-5"], watermarks: [:], activeCount: 0)
        check((invalidIDs["read"] as! [String]).isEmpty && (invalidIDs["unknown"] as! [String]).isEmpty, "Invalid and sentinel IDs are not participants")
        let positional = KCReadReceipts.decode([100, 0, 99], memberIDs: ["3", "5", "4"])
        check(positional == ["3":100,"5":0,"4":99], "Native watermark arrays use member order")
        check(KCReadReceipts.decode([100], memberIDs:["3","4"]) == ["3":100], "Missing trailing watermark stays unknown")
        check(KCReadReceipts.decode([100,99], memberIDs:["3"]) == nil, "Reject arrays without corresponding member identities")
        check(KCReadReceipts.decode([100,99,98], memberIDs:["3",NSNull(),"5"]) == ["3":100,"5":98], "Invalid member does not shift watermark indices")
        check(KCReadReceipts.decode([100,99], memberIDs:["3","3"]) == [:], "Ambiguous repeated member IDs remain unknown")
        check(KCReadReceipts.decode([Any](), memberIDs:[]) == [:], "Self-chat can have empty native arrays")
        check(KCReadReceipts.decode(["3":NSNumber(value:100)], memberIDs:[]) == ["3":100], "Dictionary watermarks remain supported")
        check(KCReadReceipts.unreadCount(logID:100,userID:9999,readMarks:Array(repeating:0,count:350),writerIndices:[],bypass:false,lastReadLogID:100)==350, "Native replacement never caps at 99")
        check(KCReadReceipts.unreadCount(logID:100,userID:2,readMarks:[0,100,99,101],writerIndices:[1,2,3,4],bypass:false,lastReadLogID:100)==2, "Native count retains boundary and writer exclusion")
        check(KCReadReceipts.unreadCount(logID:100,userID:3,readMarks:[0,100,99,101],writerIndices:[1,2,3,4],bypass:true,lastReadLogID:0)==2, "Native count retains bypass contribution")
        check(KCTrackerPolicy.blocks(URL(string: "https://stat.tiara.daum.net/track")!), "Block verified analytics endpoint")
        check(KCTrackerPolicy.blocks(URL(string: "https://ad.daum.net/a?key=fixture")!), "Block SDK tracker without reading its payload")
        for value in ["https://stat.tiara.daum.net.evil.test/track", "https://stat.tiara.daum.net@ordinary.test/", "https://example.test/?url=ad.daum.net", "https://kakao.com/", "file:///stat.tiara.daum.net"] {
            check(!KCTrackerPolicy.blocks(URL(string: value)!), "Preserve unrelated and lookalike URLs")
        }
        let shareEndpoint = URL(string:"https://example.test/ios/talk_share/log.json")!
        check(KCTrackerPolicy.nativeShareLogEndpoint("example.test/ios/talk_share/log.json") == shareEndpoint, "Native host/path receives the same HTTPS prefix as the app caller")
        check(KCTrackerPolicy.nativeShareLogEndpoint(shareEndpoint.absoluteString) == shareEndpoint, "An already absolute native endpoint stays intact")
        check(KCTrackerPolicy.nativeShareLogEndpoint("/ios/talk_share/log.json") == nil && KCTrackerPolicy.nativeShareLogEndpoint("http://example.test/ios/talk_share/log.json") == nil, "Reject missing hosts and unexpected schemes")
        check(KCTrackerPolicy.blocksShareLog(shareEndpoint,endpoint:shareEndpoint), "Block exact native share analytics endpoint")
        check(KCTrackerPolicy.blocksShareLog(URL(string:shareEndpoint.absoluteString+"?v=1")!,endpoint:shareEndpoint), "Query parameters do not evade the verified endpoint")
        for value in ["https://example.test/ios/talk/share", "https://example.test/ios/talk_share/log.json/other", "https://example.test.evil.test/ios/talk_share/log.json", "https://example.test@ordinary.test/ios/talk_share/log.json", "https://example.test:8443/ios/talk_share/log.json", "http://example.test/ios/talk_share/log.json"] {
            check(!KCTrackerPolicy.blocksShareLog(URL(string:value)!,endpoint:shareEndpoint), "Share log filtering preserves unrelated destinations")
        }
        check(!KCTrackerPolicy.blocksShareLog(shareEndpoint,endpoint:nil), "Unavailable native endpoint never enables a broad filter")
        check(!KCTrackerPolicy.blocksShareLog(shareEndpoint,endpoint:URL(string:"https://example.test/ios/talk/share")), "Reject an unexpected native getter result")

        let sectionType = "FriendsFeedPresentation.FriendsListDataSource.SectionType"
        func sections(_ names: [String]) -> [(type: String, name: String)] { names.map { (sectionType, $0) } }
        check(KCLayoutPolicy.specialFriendSectionIndex(sections(["updateProfile", "specialFriend", "birthdayFriend", "favoriteFriend", "friend"])) == 1, "Only the special-friend promotion section is selected")
        check(KCLayoutPolicy.specialFriendSectionIndex(sections(["friend", "specialFriend"])) == 1, "Promotion detection is independent of section position")
        check(KCLayoutPolicy.specialFriendSectionIndex(sections(["specialFriend"])) == 0, "An interim promotion-only snapshot can become empty")
        check(KCLayoutPolicy.specialFriendSectionIndex(sections(["birthdayFriend", "friend"])) == nil, "Birthday and ordinary friends remain visible")
        check(KCLayoutPolicy.specialFriendSectionIndex(sections(["specialFriend", "specialFriend"])) == nil, "Ambiguous duplicate promotion identifiers are preserved")
        check(KCLayoutPolicy.specialFriendSectionIndex([(sectionType, "specialFriend"), ("Other.SectionType", "friend")]) == nil, "Mixed or foreign section models are not filtered")
        check(KCLayoutPolicy.specialFriendSectionIndex(sections(["specialFriend", "futureSection"])) == 0, "New non-promotion sections remain untouched")
        check(KCLayoutPolicy.specialFriendSectionIndex([]) == nil && KCLayoutPolicy.specialFriendSectionIndex(sections(Array(repeating: "friend", count: 33))) == nil, "Empty and oversized section sets are preserved")
        let chipType="FriendsFeedPresentation.FriendsTabHeaderView.(unknown context at $1234).ChipItem"
        check(KCLayoutPolicy.friendChipRole(typeName:chipType,caseName:"list")==0, "Identify the native friend list without translated labels")
        check(KCLayoutPolicy.friendChipRole(typeName:chipType,caseName:"feed")==1, "Identify the native feed role")
        check(KCLayoutPolicy.friendChipRole(typeName:chipType,caseName:"future")==(-1), "Unknown future chip roles are unchanged")
        for value in ["Other.Header.ChipItem", "FriendsFeedPresentation.FriendsTabHeaderView.ChipSection", "FriendsFeedPresentation.FriendsTabHeaderViewFake.X.ChipItem"] {
            check(KCLayoutPolicy.friendChipRole(typeName:value,caseName:"feed")==(-1), "Preserve unrelated enum types")
        }
        check(KCLayoutPolicy.friendListIndex([0,1])==0 && KCLayoutPolicy.friendListIndex([1,0])==1, "Select the list by role even if ordering changes")
        check(KCLayoutPolicy.friendListIndex([0])==0, "An already filtered list remains valid")
        for roles in [[],[1],[0,0],[0,-1],[0,1,1]] {
            check(KCLayoutPolicy.friendListIndex(roles)==nil, "Do not filter missing, duplicate, unknown or oversized models")
        }

        var history = KCHistoryPolicy.merge(nil, chatID: "12", logID: "100", senderID: "1", text: "original", type: 1, now: 1)
        let duplicate = KCHistoryPolicy.merge(history, chatID: "12", logID: "100", senderID: "1", text: "original", type: 1, now: 2)
        check(duplicate == history, "Repeated display does not duplicate history")
        history = KCHistoryPolicy.merge(history, chatID: "12", logID: "100", senderID: "1", text: "edited", type: 1, now: 3)
        check(history.revisions.map(\.text) == ["original", "edited"], "Keep original and edited versions")
        history = KCHistoryPolicy.merge(history, chatID: "12", logID: "100", senderID: "1", text: "edited", type: 26, now: 4)
        check(history.revisions.count == 3, "Changed message state is a separate revision")
        for index in 0..<40 {
            history = KCHistoryPolicy.merge(history, chatID: "12", logID: "100", senderID: "1", text: "revision \(index)", type: 1, now: Double(index + 5))
        }
        check(history.revisions.count == 20 && history.revisions.first?.text == "original" && history.revisions.last?.text == "revision 39", "History bound retains original and latest versions")
        let roundtrip = try JSONDecoder().decode(KCHistoryEntry.self, from: JSONEncoder().encode(history))
        check(roundtrip == history, "History survives save and reload")
        let oldJSON = Data("{\"text\":\"old\",\"type\":1,\"observedAt\":1}".utf8)
        let legacyRevision = try JSONDecoder().decode(KCHistoryRevision.self, from: oldJSON)
        check(legacyRevision.attachment == nil, "Pre-update archives remain readable")
        let markdown = KCHistoryPolicy.attachmentJSON(["markdown": true])!
        let lateAttachment = KCHistoryPolicy.merge(duplicate, chatID: "12", logID: "100", senderID: "1", text: "original", type: 1, now: 6, attachment: markdown)
        check(lateAttachment.revisions.count == 1 && lateAttachment.revisions[0].attachment == markdown, "Late decoded attachment enriches original revision")
        let clearedAttachment = KCHistoryPolicy.merge(lateAttachment, chatID: "12", logID: "100", senderID: "1", text: "original", type: 1, now: 7, attachment: "{}")
        check(clearedAttachment.revisions.count == 2 && clearedAttachment.revisions[0].attachment == markdown && clearedAttachment.revisions[1].attachment == "{}", "Attachment-only changes retain previous formatting")
        check(KCHistoryPolicy.attachmentJSON(["large": String(repeating:"x",count:70_000)]) == nil, "Oversized attachments are bounded")
        check(KCHistoryPolicy.attachmentJSON(["b":1,"a":2]) == KCHistoryPolicy.attachmentJSON(["a":2,"b":1]), "Attachment serialization is stable")
        let loading = KCHistoryPolicy.merge(nil, chatID: "12", logID: "101", senderID: "1", text: "", type: 1, now: 1)
        let decoded = KCHistoryPolicy.merge(loading, chatID: "12", logID: "101", senderID: "1", text: "decoded original", type: 1, now: 2, attachment: markdown)
        check(decoded.revisions.count == 1 && decoded.revisions[0].text == "decoded original" && decoded.revisions[0].observedAt == 1, "Late plaintext enriches the first observation instead of inventing an edit")
        let nextEdit = KCHistoryPolicy.merge(decoded, chatID: "12", logID: "101", senderID: "1", text: "real edit", type: 1, now: 3, attachment: markdown)
        check(nextEdit.revisions.map(\.text) == ["decoded original", "real edit"], "A later actual edit still keeps the decoded original")
        let media = KCHistoryPolicy.attachmentJSON(["url":"https://example.test/fixture.jpg","w":512])
        let sentMedia = KCHistoryPolicy.attachmentJSON(["url":"https://example.test/fixture.jpg","w":512,"sendingInfo":["progress":1]])
        check(media == sentMedia, "Local send progress never creates an attachment edit")
        print("Message policy tests: \(passed) passed")
    }
}
