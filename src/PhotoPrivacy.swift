import Foundation
import ImageIO

/// Produces a separate, lossless upload copy. The Photos asset is never written.
enum KCPhotoPrivacy {
    static func sanitizedCopy(_ data: Data) -> Data? {
        guard !data.isEmpty, data.count <= 256 * 1024 * 1024,
              let source = CGImageSourceCreateWithData(data as CFData, nil),
              CGImageSourceGetStatus(source) == .statusComplete,
              let type = CGImageSourceGetType(source) else { return nil }
        let count = CGImageSourceGetCount(source)
        guard count > 0, count <= 500 else { return nil }
        let properties = CGImageSourceCopyPropertiesAtIndex(source, 0, nil) as? [String: Any] ?? [:]
        let orientation = (properties[kCGImagePropertyOrientation as String] as? NSNumber)?.intValue ?? 1
        guard (1...8).contains(orientation) else { return nil }
        if ["public.heic", "public.heif"].contains(type as String) {
            guard let output = KCHEIFPrivacy.clean(data, orientation: orientation),
                  let checked = CGImageSourceCreateWithData(output as CFData, nil),
                  CGImageSourceGetCount(checked) == count,
                  metadataIsClean(checked, orientation: orientation) else { return nil }
            return output
        }
        if type as String == "public.png" {
            guard let output = KCPNGPrivacy.clean(data, orientation: orientation),
                  let checked = CGImageSourceCreateWithData(output as CFData, nil),
                  CGImageSourceGetCount(checked) == count,
                  metadataIsClean(checked, orientation: orientation) else { return nil }
            return output
        }
        // Other containers require independent metadata and animation tests.
        guard type as String == "public.jpeg" else { return nil }
        guard count == 1, let output = KCJPEGPrivacy.clean(data, orientation: orientation),
              let checked = CGImageSourceCreateWithData(output as CFData, nil),
              CGImageSourceGetCount(checked) == count,
              metadataIsClean(checked, orientation: orientation) else { return nil }
        return output
    }
    static func metadataIsClean(_ source: CGImageSource, orientation: Int) -> Bool {
        let clean = CGImageSourceCopyPropertiesAtIndex(source, 0, nil) as? [String: Any] ?? [:]
        guard (clean[kCGImagePropertyOrientation as String] as? NSNumber)?.intValue ?? 1 == orientation,
              clean[kCGImagePropertyGPSDictionary as String] == nil,
              clean[kCGImagePropertyIPTCDictionary as String] == nil,
              clean[kCGImagePropertyExifDictionary as String] == nil else { return false }
        if let tiff = clean[kCGImagePropertyTIFFDictionary as String] as? [String: Any] {
            let allowed: Set<String> = [kCGImagePropertyTIFFOrientation as String, kCGImagePropertyTIFFXResolution as String,
                                        kCGImagePropertyTIFFYResolution as String, kCGImagePropertyTIFFResolutionUnit as String,
                                        "TileLength", "TileWidth"]
            guard Set(tiff.keys).isSubset(of: allowed) else { return false }
        }
        return true
    }
}

enum KCJPEGPrivacy {
    static func clean(_ data: Data, orientation: Int) -> Data? {
        let b = [UInt8](data)
        guard b.starts(with: [0xff,0xd8]), (1...8).contains(orientation) else { return nil }
        let exif: [UInt8] = [69,120,105,102,0,0,0x49,0x49,42,0,8,0,0,0,1,0,0x12,1,3,0,1,0,0,0,UInt8(orientation),0,0,0,0,0,0,0]
        var output = Data([0xff,0xd8,0xff,0xe1,0,UInt8(exif.count+2)] + exif)
        var cursor = 2, inScan = false, sawScan = false, segments = 0
        while cursor < b.count {
            if inScan {
                let start = cursor
                while cursor < b.count {
                    if b[cursor] != 0xff { cursor += 1; continue }
                    let markerStart = cursor
                    while cursor < b.count && b[cursor] == 0xff { cursor += 1 }
                    guard cursor < b.count else { return nil }
                    if b[cursor] == 0 || (0xd0...0xd7).contains(b[cursor]) { cursor += 1; continue }
                    output.append(contentsOf: b[start..<markerStart]); cursor = markerStart; inScan = false; break
                }
                if inScan { return nil }
            }
            let start = cursor
            guard b[cursor] == 0xff, segments < 100000 else { return nil }
            while cursor < b.count && b[cursor] == 0xff { cursor += 1 }
            guard cursor < b.count else { return nil }
            let marker = b[cursor]; cursor += 1; segments += 1
            if marker == 0xd9 {
                guard sawScan, cursor == b.count else { return nil }
                output.append(contentsOf: b[start..<cursor]); return output
            }
            guard marker != 0, marker != 0xd8, marker != 1, !(0xd0...0xd7).contains(marker), cursor+2 <= b.count else { return nil }
            let size = Int(b[cursor])*256+Int(b[cursor+1])
            guard size >= 2, size <= b.count-cursor else { return nil }
            let end = cursor+size, payload = b[(cursor+2)..<end]
            var keep = !(0xe0...0xef).contains(marker) && marker != 0xfe
            if marker == 0xe0 { keep = payload.starts(with: Array("JFIF\0".utf8)) }
            if marker == 0xe2 {
                // MPF/gain-map offsets cannot be retained after shrinking the
                // header. Leave these unsupported images to the native sender.
                guard payload.starts(with: Array("ICC_PROFILE\0".utf8)) else { return nil }
                keep = true
            }
            if marker == 0xee { keep = payload.starts(with: Array("Adobe".utf8)) }
            if keep { output.append(contentsOf: b[start..<end]) }
            cursor = end
            if marker == 0xda { inScan = true; sawScan = true }
        }
        return nil
    }
}

enum KCPNGPrivacy {
    static func clean(_ data: Data, orientation: Int) -> Data? {
        let b = [UInt8](data)
        guard b.starts(with: [137,80,78,71,13,10,26,10]) else { return nil }
        var output = Data(b.prefix(8)), cursor = 8, chunks = 0
        while cursor < b.count {
            let start = cursor
            guard chunks < 100000, let length = KCHEIFPrivacy.read(b, &cursor, 4, b.count), b.count-cursor >= 8,
                  length <= b.count-cursor-8 else { return nil }
            let type = String(bytes: b[cursor..<cursor+4], encoding: .ascii) ?? ""
            guard chunks > 0 || (type == "IHDR" && length == 13) else { return nil }
            let end = cursor+4+length+4
            if !["eXIf", "iTXt", "tEXt", "zTXt", "tIME"].contains(type) { output.append(contentsOf: b[start..<end]) }
            if type == "IHDR" && orientation != 1 {
                let tiff: [UInt8] = [0x49,0x49,42,0,8,0,0,0,1,0,0x12,1,3,0,1,0,0,0,UInt8(orientation),0,0,0,0,0,0,0]
                let payload = Array("eXIf".utf8) + tiff
                output.append(contentsOf: bigEndian(UInt32(tiff.count)) + payload + bigEndian(crc32(payload)))
            }
            cursor = end; chunks += 1
            if type == "IEND" { return length == 0 && cursor == b.count ? output : nil }
        }
        return nil
    }
    static func bigEndian(_ value: UInt32) -> [UInt8] { [24,16,8,0].map { UInt8(truncatingIfNeeded: value >> $0) } }
    static func crc32(_ bytes: [UInt8]) -> UInt32 {
        var value = UInt32.max
        for byte in bytes {
            value ^= UInt32(byte)
            for _ in 0..<8 { value = value & 1 == 1 ? (value >> 1) ^ 0xedb88320 : value >> 1 }
        }
        return value ^ UInt32.max
    }
}

/// Replaces only Exif/XMP item payloads. Box lengths, image items, auxiliary
/// images, color profiles and HEVC bitstreams remain byte-for-byte identical.
/// ImageIO's lossless copy currently retains some HEIC Exif/IPTC fields, so
/// these metadata items need to be scrubbed at the container level.
enum KCHEIFPrivacy {
    struct Box { var type: String; var payload: Int; var end: Int }
    struct Extent { var start: Int; var length: Int; var end: Int { start + length } }
    static func read(_ bytes: [UInt8], _ cursor: inout Int, _ size: Int, _ end: Int) -> Int? {
        guard size >= 0, size <= 8, cursor >= 0, cursor <= end, size <= end-cursor else { return nil }
        var value = 0
        for _ in 0..<size {
            guard value <= (Int.max - Int(bytes[cursor])) / 256 else { return nil }
            value = value * 256 + Int(bytes[cursor]); cursor += 1
        }
        return value
    }
    static func boxes(_ bytes: [UInt8], _ start: Int, _ end: Int) -> [Box]? {
        var result: [Box] = [], cursor = start
        while cursor < end {
            let begin = cursor
            guard result.count < 4096, let short = read(bytes, &cursor, 4, end), cursor+4 <= end else { return nil }
            let type = String(bytes: bytes[cursor..<cursor+4], encoding: .ascii) ?? ""; cursor += 4
            let size: Int
            if short == 1 { guard let long = read(bytes, &cursor, 8, end) else { return nil }; size = long }
            else { size = short == 0 ? end-begin : short }
            guard size >= cursor-begin, size <= end-begin else { return nil }
            result.append(Box(type: type, payload: cursor, end: begin+size)); cursor = begin+size
        }
        return result
    }
    static func clean(_ data: Data, orientation: Int) -> Data? {
        let bytes = [UInt8](data)
        guard let top = boxes(bytes, 0, bytes.count),
              let meta = top.first(where: { $0.type == "meta" }), meta.payload+4 <= meta.end,
              let children = boxes(bytes, meta.payload+4, meta.end),
              let iinf = children.first(where: { $0.type == "iinf" }),
              let iloc = children.first(where: { $0.type == "iloc" }) else { return nil }
        var cursor = iinf.payload
        guard let infoVersion = read(bytes, &cursor, 1, iinf.end), read(bytes, &cursor, 3, iinf.end) != nil,
              let infoCount = read(bytes, &cursor, infoVersion == 0 ? 2 : 4, iinf.end), infoCount <= 4096,
              let entries = boxes(bytes, cursor, iinf.end), entries.count == infoCount else { return nil }
        var targets: [Int: String] = [:]
        for entry in entries {
            guard entry.type == "infe" else { return nil }
            cursor = entry.payload
            guard let version = read(bytes, &cursor, 1, entry.end), [2,3].contains(version),
                  read(bytes, &cursor, 3, entry.end) != nil,
                  let id = read(bytes, &cursor, version == 2 ? 2 : 4, entry.end),
                  let protection = read(bytes, &cursor, 2, entry.end), protection == 0, cursor+4 <= entry.end else { return nil }
            let type = String(bytes: bytes[cursor..<cursor+4], encoding: .ascii) ?? ""; cursor += 4
            if type == "Exif" { targets[id] = type }
            if type == "mime" {
                guard let endName = bytes[cursor..<entry.end].firstIndex(of: 0) else { return nil }
                cursor = endName + 1
                guard let endMime = bytes[cursor..<entry.end].firstIndex(of: 0),
                      String(bytes: bytes[cursor..<endMime], encoding: .ascii) == "application/rdf+xml" else { return nil }
                targets[id] = "XMP"
            }
        }
        cursor = iloc.payload
        guard let version = read(bytes, &cursor, 1, iloc.end), version <= 2,
              read(bytes, &cursor, 3, iloc.end) != nil,
              let sizes = read(bytes, &cursor, 1, iloc.end), let more = read(bytes, &cursor, 1, iloc.end),
              let itemCount = read(bytes, &cursor, version < 2 ? 2 : 4, iloc.end), itemCount <= 4096 else { return nil }
        let offsetSize = sizes >> 4, lengthSize = sizes & 15, baseSize = more >> 4, indexSize = version > 0 ? more & 15 : 0
        guard [offsetSize,lengthSize,baseSize,indexSize].allSatisfy({ $0 <= 8 }) else { return nil }
        var extents: [Int: [Extent]] = [:]
        for _ in 0..<itemCount {
            guard let id = read(bytes, &cursor, version < 2 ? 2 : 4, iloc.end), extents[id] == nil else { return nil }
            var method = 0
            if version > 0 { guard let value = read(bytes, &cursor, 2, iloc.end), value <= 1 else { return nil }; method = value }
            guard read(bytes, &cursor, 2, iloc.end) == 0,
                  let base = read(bytes, &cursor, baseSize, iloc.end), base <= bytes.count,
                  let number = read(bytes, &cursor, 2, iloc.end), number <= 4096 else { return nil }
            var ranges: [Extent] = []
            for _ in 0..<number {
                guard read(bytes, &cursor, indexSize, iloc.end) != nil,
                      let offset = read(bytes, &cursor, offsetSize, iloc.end), offset <= bytes.count-base,
                      let length = read(bytes, &cursor, lengthSize, iloc.end), length > 0 else { return nil }
                let container = method == 1 ? children.first(where: { $0.type == "idat" }) : top.first(where: { $0.type == "mdat" && $0.payload <= base+offset && base+offset < $0.end })
                guard let container else { return nil }
                let origin = method == 1 ? container.payload : 0
                guard base+offset <= bytes.count-origin else { return nil }
                let start = base+offset+origin
                guard start >= container.payload, start <= container.end, length <= container.end-start else { return nil }
                ranges.append(Extent(start: start, length: length))
            }
            extents[id] = ranges
        }
        var output = data
        for (id, type) in targets {
            guard let ranges = extents[id], !ranges.isEmpty else { return nil }
            var capacity = 0
            for r in ranges {
                guard r.length <= 4*1024*1024-capacity else { return nil }; capacity += r.length
                // A malformed alias must never let a metadata rewrite touch an image.
                for (otherID, otherRanges) in extents where otherID != id {
                    guard otherRanges.allSatisfy({ r.end <= $0.start || $0.end <= r.start }) else { return nil }
                }
            }
            let replacement: [UInt8]
            if type == "Exif" {
                // HEIF Exif item: TIFF offset=0, little-endian TIFF, one Orientation tag.
                replacement = [0,0,0,0,0x49,0x49,42,0,8,0,0,0,1,0,0x12,1,3,0,1,0,0,0,UInt8(orientation),0,0,0,0,0,0,0]
            } else { replacement = Array("<x:xmpmeta xmlns:x=\"adobe:ns:meta/\"></x:xmpmeta>".utf8) }
            guard replacement.count <= capacity else { return nil }
            let payload = replacement + [UInt8](repeating: type == "Exif" ? 0 : 32, count: capacity-replacement.count)
            var offset = 0
            for range in ranges { output.replaceSubrange(range.start..<range.end, with: payload[offset..<offset+range.length]); offset += range.length }
        }
        return output
    }
}

@objc(KCPhotoPrivacyBridge) public final class KCPhotoPrivacyBridge: NSObject {
    @objc public static func sanitizedCopy(_ data: Data) -> Data? { KCPhotoPrivacy.sanitizedCopy(data) }
}
