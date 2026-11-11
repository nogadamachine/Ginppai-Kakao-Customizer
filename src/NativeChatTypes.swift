// Compile only as a declaration module named TalkSharedBase. Do not link a
// stub implementation, class or metadata into the customizer. Consumers use
// this nominal type solely for checked closure casts/reference arguments.
// The shipped framework owns Chat, its NSManagedObject layout and behavior.
import CoreData
@_weakLinked @objc(Chat) public class Chat: NSManagedObject {}
@_weakLinked public class NSEUserSettings {
    public static var shared:NSEUserSettings {fatalError("declaration only")}
    public final var excludeBadgeForMutedChat:Bool {fatalError("declaration only")}
}
