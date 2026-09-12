import Foundation

// Read only the selected viewer's media fields. Do not inspect user/contact
// records, network clients, arbitrary model graphs, or all cached images.
@objc(KCMediaResolver)
public final class KCMediaResolver: NSObject {
    private static func unwrap(_ value: Any?) -> Any? {
        guard let value else { return nil }
        let mirror = Mirror(reflecting: value)
        if mirror.displayStyle == .optional { return mirror.children.first.flatMap { unwrap($0.value) } }
        return value
    }
    private static func field(_ value: Any?, _ name: String) -> Any? {
        guard let value = unwrap(value) else { return nil }
        var mirror: Mirror? = Mirror(reflecting: value)
        while let current = mirror {
            if let child = current.children.first(where: { $0.label == name }) { return unwrap(child.value) }
            mirror = current.superclassMirror
        }
        return nil
    }
    private static func url(_ value: Any?) -> URL? {
        guard let value = unwrap(value) else { return nil }
        let result: URL?
        if let direct = value as? URL { result = direct }
        else if let string = value as? String { result = URL(string: string) }
        else { result = nil }
        guard let result, ["https", "http", "file"].contains(result.scheme?.lowercased() ?? "") else { return nil }
        return result
    }
    // This thumbnail form is observed alongside the exact original URL in
    // ProfileImageDataModel. Keep signatures and every unrelated query item.
    private static func withoutThumbnail(_ value: URL) -> URL {
        guard value.host == "chat.kakaocdn.net", var parts = URLComponents(url: value, resolvingAgainstBaseURL: false),
              parts.queryItems?.contains(where: { $0.name == "type" && $0.value == "thumb" }) == true else { return value }
        parts.queryItems = parts.queryItems?.filter { $0.name != "type" && $0.name != "opt" }
        if parts.queryItems?.isEmpty == true { parts.queryItems = nil }
        return parts.url ?? value
    }
    private static func sameResource(_ a: URL, _ b: URL) -> Bool { withoutThumbnail(a) == withoutThumbnail(b) }

    @objc public static func resolve(_ object: AnyObject, contexts: [AnyObject]) -> NSDictionary {
        let model = field(object, "viewModel")
        let className = String(reflecting: type(of: object))
        let isVideoCell = className == "Profile.ProfilePostDetailVideoCell"
        let isImageCell = className == "Profile.ProfilePostDetailImageCell"
        let source = url(field(model, "sourceURL"))
            ?? url(field(model, isVideoCell ? "videoURL" : "imageURL"))
        guard let source else { return [:] }

        var selected = withoutThumbnail(source)
        var video = isVideoCell || ["mp4", "mov", "m4v", "m3u8"].contains(source.pathExtension.lowercased())
        var originalField = false
        for context in contexts {
            let service = field(field(context, "viewModel"), "service")
            let profile = field(service, "profile")
            for key in ["profileImage", "backgroundImage"] {
                guard let media = field(profile, key) else { continue }
                let names = ["thumbnailUrl", "mediumUrl", "originalUrl", "mediumAnimatedUrl", "originalAnimatedUrl"]
                let urls = names.compactMap { url(field(media, $0)) }
                guard urls.contains(where: { sameResource(source, $0) }) else { continue }
                if !isImageCell, let originalVideo = url(field(media, "originalAnimatedUrl")) {
                    selected = originalVideo; video = true; originalField = true
                } else if video, let availableVideo = url(field(media, "mediumAnimatedUrl")) {
                    selected = availableVideo
                } else if let originalImage = url(field(media, "originalUrl")) {
                    selected = originalImage; video = false; originalField = true
                }
            }
        }
        return ["url": selected, "video": video, "originalField": originalField,
                "source": isVideoCell ? "videoURL" : (className.contains("ImageCell") ? "imageURL" : "sourceURL")]
    }
}
