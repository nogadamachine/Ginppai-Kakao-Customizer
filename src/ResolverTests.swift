import Foundation

private struct ImageRecord {
    let thumbnailUrl: String
    let mediumUrl: String
    let originalUrl: String
    let mediumAnimatedUrl: String?
    let originalAnimatedUrl: String?
}
private struct ProfileRecord { let profileImage: ImageRecord; let backgroundImage: ImageRecord }
private struct Service { let profile: ProfileRecord? }
private struct HomeModel { let service: Service }
private class Home: NSObject { let viewModel: HomeModel; init(_ p: ProfileRecord) { viewModel = HomeModel(service: Service(profile:p)) } }
private struct MediaModel { let sourceURL: URL }
private class Viewer: NSObject { let viewModel: MediaModel; init(_ url: String) { viewModel = MediaModel(sourceURL:URL(string:url)!) } }
private struct ImageModel { let imageURL: URL? }
class ProfilePostDetailImageCell: NSObject { fileprivate let viewModel: ImageModel?; init(_ url: String) { viewModel = ImageModel(imageURL:URL(string:url)) } }
private struct VideoModel { let videoURL: URL? }
class ProfilePostDetailVideoCell: NSObject { fileprivate let viewModel: VideoModel?; init(_ url: String) { viewModel = VideoModel(videoURL:URL(string:url)) } }

@main struct ResolverTests {
    static func main() {
        let base="https://chat.kakaocdn.net/example/f.jpg"
        let original=base+"?signature=keep-this"
        let thumb=base+"?type=thumb&opt=U640x640&signature=keep-this"
        let photo=ImageRecord(thumbnailUrl:thumb,mediumUrl:thumb,originalUrl:original,mediumAnimatedUrl:nil,originalAnimatedUrl:nil)
        let video=ImageRecord(thumbnailUrl:"https://example.invalid/cover.jpg",mediumUrl:"https://example.invalid/cover.jpg",originalUrl:"https://example.invalid/cover-original.jpg",mediumAnimatedUrl:"https://example.invalid/video-low.mp4",originalAnimatedUrl:"https://example.invalid/video-original.mp4?signature=keep")
        let context=Home(ProfileRecord(profileImage:photo,backgroundImage:video))
        func resolve(_ owner: AnyObject) -> NSDictionary { KCMediaResolver.resolve(owner,contexts:[context]) }
        func expect(_ result:NSDictionary,_ url:String,_ video:Bool) {
            precondition((result["url"] as? URL)?.absoluteString==url)
            precondition(result["video"] as? Bool==video)
        }
        expect(resolve(Viewer(thumb)),original,false)
        expect(KCMediaResolver.resolve(Viewer(thumb),contexts:[]),original,false)
        expect(resolve(Viewer(video.mediumAnimatedUrl!)),video.originalAnimatedUrl!,true)
        expect(resolve(Viewer(video.mediumUrl)),video.originalAnimatedUrl!,true)
        expect(resolve(ProfilePostDetailVideoCell(video.mediumAnimatedUrl!)),video.originalAnimatedUrl!,true)
        // A still history post must remain a still, even when the profile also has a video.
        expect(resolve(ProfilePostDetailImageCell(video.mediumUrl)),video.originalUrl,false)
        // The neighboring/current profile must not replace the selected old photo.
        let old="https://chat.kakaocdn.net/old/f.jpg"
        expect(resolve(ProfilePostDetailImageCell(old)),old,false)
        // Do not remove unrelated transformation/signature fields from other CDNs.
        let other="https://example.invalid/file.jpg?type=thumb&opt=U640x640&signature=keep"
        expect(KCMediaResolver.resolve(Viewer(other),contexts:[]),other,false)
        precondition(KCMediaResolver.resolve(NSObject(),contexts:[context]).count==0)
        print("9 selected-media, original-resolution, video and query-preservation checks passed.")
    }
}
