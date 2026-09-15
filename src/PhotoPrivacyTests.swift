import Foundation
import ImageIO
import CoreGraphics

@main struct PhotoPrivacyTests {
    static func main() {
        let space = CGColorSpaceCreateDeviceRGB()
        var pixels = [UInt8](repeating: 0, count: 24 * 16 * 4)
        for i in 0..<24*16 { pixels[i*4] = UInt8(i % 251); pixels[i*4+1] = 140; pixels[i*4+2] = 70; pixels[i*4+3] = 255 }
        let image = pixels.withUnsafeMutableBytes { raw in
            CGContext(data: raw.baseAddress, width: 24, height: 16, bitsPerComponent: 8, bytesPerRow: 24*4,
                      space: space, bitmapInfo: CGImageAlphaInfo.premultipliedLast.rawValue)!.makeImage()!
        }
        var tested = 0
        for type in ["public.jpeg", "public.heic", "public.png"] {
        for orientation in 1...8 {
            let original = NSMutableData()
            let dest = CGImageDestinationCreateWithData(original, type as CFString, 1, nil)!
            let props: [CFString: Any] = [
                kCGImagePropertyOrientation: orientation,
                kCGImagePropertyGPSDictionary: [kCGImagePropertyGPSLatitude: 35.0, kCGImagePropertyGPSLatitudeRef: "N"],
                kCGImagePropertyExifDictionary: [kCGImagePropertyExifDateTimeOriginal: "2020:01:02 03:04:05", kCGImagePropertyExifUserComment: "private test"],
                kCGImagePropertyTIFFDictionary: [kCGImagePropertyTIFFMake: "PrivateCamera", kCGImagePropertyTIFFModel: "Model", kCGImagePropertyTIFFArtist: "PrivateAuthor"],
                kCGImagePropertyIPTCDictionary: [kCGImagePropertyIPTCByline: ["PrivateAuthor"]]
            ]
            CGImageDestinationAddImage(dest, image, props as CFDictionary)
            precondition(CGImageDestinationFinalize(dest))
            var before = original as Data
            if type == "public.jpeg" {
                // Photos can add a legacy Photoshop/IPTC APP13 block alongside
                // Exif/XMP. ImageIO metadata replacement leaves this block.
                let byline = Array("LegacySyntheticOwner".utf8)
                let iim: [UInt8] = [0x1c,2,80,0,UInt8(byline.count)] + byline
                let resource = Array("Photoshop 3.0\0".utf8) + Array("8BIM".utf8) + [4,4,0,0] + KCPNGPrivacy.bigEndian(UInt32(iim.count)) + iim + (iim.count % 2 == 1 ? [0] : [])
                let block: [UInt8] = [0xff,0xed,UInt8((resource.count+2) >> 8),UInt8((resource.count+2) & 255)] + resource
                before.insert(contentsOf: block, at: 2)
            }
            let originalBytes = [UInt8](before)
            guard let clean = KCPhotoPrivacy.sanitizedCopy(before) else { fatalError("\(type) orientation \(orientation) failed") }
            let source = CGImageSourceCreateWithData(clean as CFData, nil)!
            let result = CGImageSourceCopyPropertiesAtIndex(source, 0, nil)! as NSDictionary
            precondition(((result[kCGImagePropertyOrientation] as? NSNumber)?.intValue ?? 1) == orientation)
            precondition(result[kCGImagePropertyGPSDictionary] == nil && result[kCGImagePropertyExifDictionary] == nil)
            precondition(!String(describing: result).contains("Private"))
            precondition(clean.range(of: Data("LegacySyntheticOwner".utf8)) == nil, "Legacy IPTC remains")
            if type == "public.jpeg" { precondition(scanData(before) == scanData(clean), "Compressed JPEG pixels changed") }
            let originalImage = CGImageSourceCreateImageAtIndex(CGImageSourceCreateWithData(before as CFData, nil)!,0,nil)!
            let cleanImage = CGImageSourceCreateImageAtIndex(source,0,nil)!
            precondition(originalImage.width == cleanImage.width && originalImage.height == cleanImage.height)
            precondition(originalImage.dataProvider!.data! as Data == cleanImage.dataProvider!.data! as Data, "Decoded pixels changed")
            precondition([UInt8](before) == originalBytes, "Input changed")
            tested += 1
        }
        }
        precondition(KCPhotoPrivacy.sanitizedCopy(Data()) == nil)
        precondition(KCPhotoPrivacy.sanitizedCopy(Data("not an image".utf8)) == nil)
        precondition(KCHEIFPrivacy.clean(Data([0,0,0,1] + Array("meta".utf8) + [UInt8](repeating:255,count:8)),orientation:1) == nil)
        precondition(KCPNGPrivacy.clean(Data([137,80,78,71,13,10,26,10,255,255,255,255]),orientation:1) == nil)
        precondition(KCJPEGPrivacy.clean(Data([0xff,0xd8,0xff,0xe1,0xff,0xff]),orientation:1) == nil)
        precondition(KCJPEGPrivacy.clean(Data([0xff,0xd8,0xff,0xe2,0,6,77,80,70,0,0xff,0xd9]),orientation:1) == nil)
        print("Photo privacy: \(tested + 6) checks passed")
    }
    static func scanData(_ data: Data) -> Data {
        let bytes = [UInt8](data); var i = 2
        while i + 4 <= bytes.count {
            precondition(bytes[i] == 0xff)
            let marker = bytes[i+1]
            if marker == 0xda { return data.subdata(in: i..<data.count) }
            let size = Int(bytes[i+2])*256 + Int(bytes[i+3]); precondition(size >= 2)
            i += 2 + size
        }
        fatalError("Missing scan")
    }
}
