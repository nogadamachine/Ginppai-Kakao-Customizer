// Declaration module only, never linked. Final accessors force direct calls to
// the shipped symbols, without assuming the native class's field/vtable layout.
@_weakLinked public class UnreadCountHelper {
    public static var shared:UnreadCountHelper {fatalError("declaration only")}
    public final var openChatUnreadCount:Int {fatalError("declaration only")}
}
