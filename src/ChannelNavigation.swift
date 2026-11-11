import Foundation

struct KCNavigationEnumRole: Equatable {
    let type: String
    let role: String
}
struct KCFriendCategoryRole: Equatable {
    let itemType: String
    let itemRole: String
    let categoryType: String?
    let categoryRole: String?
    let payloadType: String?
}
enum KCChannelNavigationPolicy {
    static let friendItemType = "FriendsFeedPresentation.FriendsListDataSource.ItemType"
    static let friendCategoryType = "FriendsFeedPresentation.FriendPresentableCategory"
    static func friendChannelIndexes(_ items: [KCFriendCategoryRole]) -> [Int]? {
        guard items.count <= 20_000, items.allSatisfy({$0.itemType == friendItemType}) else {return nil}
        var indexes=[Int]()
        for (index,item) in items.enumerated() where item.itemRole == "category" {
            guard item.categoryType == friendCategoryType,let role=item.categoryRole else{return nil}
            if role == "plusFriend" {
                guard item.payloadType == "Swift.Int" else{return nil}
                indexes.append(index)
            }
        }
        return indexes
    }
    static func permittedField(type:String,name:String)->Bool {
        let fields:[String:Set<String>]=[
            "FriendsFeedPresentation.FriendsTabContainerViewController":["contentView","friendsTabHeaderView","_currentView"],
            "FriendsFeedPresentation.FriendsTabHeaderView":["currentSelectedChipIndexPath"],
            "FriendsFeedPresentation.FriendsListViewController":["dataSource"],
            "KakaoTalk.MoreTabHomeViewController":["chipTabView","contentType","walletHomeLayoutContext","hidingTitleViewHeightConstraint","hidingTitleView"],
            "KakaoTalk.MoreTabChipTabView":["currentSelectedIndexPath"],
            "KakaoTalk.NowTabContainerViewController":["headerView","contentView","currentContentProvider","currentType","contentFactory","factory","topHeaderView","tabHeaderView"],
            "KakaoTalk.ChatFolderContentCoordinator":["channelChatsViewController","chatsTableViewController","chatsCollectionViewController"],
            "KakaoTalk.ChannelChatsViewController":["viewModel","collectionView","dataSource"],
            "KakaoTalk.ChannelChatsViewModel":["fetchedResultsController"],
            "KakaoTalk.OpenChatsTableViewController":["fetchedResultsController"],
            "KakaoTalk.ChatsArchiveTableViewController":["fetchedResultsController"],
            "KakaoTalk.ChatsViewController":["contentCoordinator","chatFolderContentCoordinator"],
            "KakaoTalk.ChatsTableViewController":["viewModel","fetchedResultsController"],
            "KakaoTalk.ChatsCollectionViewController":["contentCoordinator","viewModel","contentProvider"]]
        return fields[type]?.contains(name) == true
    }
}

#if canImport(UIKit)
import UIKit
import CoreData
import Darwin
import TalkSharedBase
import TalkAppBase

private enum KCNavigationReflection {
    static func unwrap(_ value:Any)->Any? {
        let mirror=Mirror(reflecting:value)
        return mirror.displayStyle == .optional ? mirror.children.first.flatMap{unwrap($0.value)}:value
    }
    static func field(_ object:Any,_ name:String)->Any? {
        var mirror:Mirror?=Mirror(reflecting:object)
        while let current=mirror {
            if let child=current.children.first(where:{$0.label==name || $0.label=="$__lazy_storage_$_"+name}) {return unwrap(child.value)}
            mirror=current.superclassMirror
        }
        return nil
    }
    static func value(_ object:Any)->Any {(object as? AnyHashable)?.base ?? object}
    static func role(_ object:Any)->KCNavigationEnumRole {
        let v=value(object),m=Mirror(reflecting:v),type=String(reflecting:Swift.type(of:v))
        guard m.displayStyle == .enum else{return .init(type:type,role:"<opaque>")}
        if let role=m.children.first?.label{return .init(type:type,role:role)}
        // Only known identifiers may be described, and only a known case name
        // can leave this adapter. Never describe associated chat/friend payloads.
        let allowed:Set<String>=["bizboard","bookmarkEntry","aiSummary","main","chat","channelChats","feedRecommends","penalty","unsupported","adFitBanner","feedAD","feed","feedHidden","threadChat","home","wallet","game","dynamic","list","openChat","shortForm","brand"]
        let typeAllowed = ["KakaoTalk.ChatListSection","KakaoTalk.ChatsTableViewController.ChatsTableSection","OpenLink.OpenChatsTableSection","OpenLink.OpenChatsTableItem","KakaoTalk.MoreTabContentType","TalkAppBase.NowSubTabType"].contains(type) ||
            (type.hasPrefix("KakaoTalk.ChannelChatsViewModel.") && type.hasSuffix(".Section"))
        guard typeAllowed else{return .init(type:type,role:"<unknown>")}
        let candidate=String(describing:v)
        return .init(type:type,role:allowed.contains(candidate) ? candidate:"<unknown>")
    }
}
private enum KCNavigationNative {
    static func boolProperty(_ object:AnyObject,_ name:String)->Bool? {
        let selector=NSSelectorFromString(name)
        guard let method=class_getInstanceMethod(Swift.type(of:object),selector),method_getNumberOfArguments(method)==2 else{return nil}
        let result=method_copyReturnType(method);defer{free(result)}
        guard String(cString:result)=="B" else{return nil}
        typealias Getter = @convention(c) (AnyObject,Selector)->Bool
        return unsafeBitCast(method_getImplementation(method),to:Getter.self)(object,selector)
    }
    static let symbols: [String:String] = [
        "channel":"$s14TalkSharedBase13ChatPredicateO017channelAndPartnerD7IDRangeSo11NSPredicateCyFZ",
        "excludingChannel":"$s14TalkSharedBase13ChatPredicateO026excludingChannelAndPartnerD7IDRangeSo11NSPredicateCyFZ"]
    static func frameworkSymbol(_ name:String)->UnsafeMutableRawPointer? {
        guard let p=dlsym(UnsafeMutableRawPointer(bitPattern:-2),name) else{return nil}
        var info=Dl_info()
        guard dladdr(p,&info) != 0,let path=info.dli_fname,
              String(cString:path).hasSuffix("/TalkSharedBase.framework/TalkSharedBase") else{return nil}
        return p
    }
    static var chatMetadataAvailable:Bool {
        frameworkSymbol("$s14TalkSharedBase4ChatCMa") != nil && frameworkSymbol("$s14TalkSharedBase4ChatCMn") != nil
    }
    static func predicate(_ key:String)->NSPredicate? {
        guard chatMetadataAvailable,let name=symbols[key],let p=frameworkSymbol(name) else{return nil}
        // Exact exported non-generic enum static functions, no Swift self or
        // metadata arguments; the native declaration returns NSPredicate.
        return unsafeBitCast(p,to:(@convention(thin) ()->NSPredicate).self)()
    }
    static func classifies(_ object:AnyObject)->Int {
        guard chatMetadataAvailable,object is Chat else{return -1}
        if let cls=NSClassFromString("OpenChat"),let managed=object as? NSManagedObject,managed.isKind(of:cls) {return 2}
        guard let predicate=predicate("channel") else{return -1}
        return predicate.evaluate(with:object) ? 1:0
    }
}
@objc(KCChannelNavigationBridge) final class KCChannelNavigationBridge:NSObject {
    @objc(canBuildChannelFriendsFrom:) static func canBuildChannelFriendsFrom(_ host:UIViewController)->Bool {
        guard Thread.isMainThread,String(reflecting:Swift.type(of:host))=="FriendsFeedPresentation.FriendsTabContainerViewController",
              host.isViewLoaded,host.view.window != nil else{return false}
        func friends(_ root:UIViewController,_ depth:Int)->UIViewController? {
            guard depth<5 else{return nil}
            if String(reflecting:Swift.type(of:root))=="FriendsFeedPresentation.FriendsListViewController" {return root}
            for child in root.children {if let found=friends(child,depth+1){return found}}
            return nil
        }
        guard let list=friends(host,0),list.isViewLoaded,list.view.window != nil,
              let vm=KCNavigationReflection.field(list,"viewModel"),
              let useCase=KCNavigationReflection.field(vm,"friendUserListUseCase"),
              let repository=KCNavigationReflection.field(useCase,"friendUserListRepository"),
              String(reflecting:Swift.type(of:repository))=="FriendsFeedData.FriendDIUserListRepository",
              let context=KCNavigationReflection.field(repository,"context") as? NSManagedObjectContext,
              context.concurrencyType == .mainQueueConcurrencyType,
              let coordinator=context.persistentStoreCoordinator,!coordinator.persistentStores.isEmpty else{return false}
        return true
    }
    @objc(canBuildChannelChatsFromCoordinator:) static func canBuildChannelChatsFromCoordinator(_ coordinator:Any)->Bool {
        guard Thread.isMainThread,String(reflecting:Swift.type(of:coordinator))=="KakaoTalk.ChatFolderContentCoordinator",
              let normal=KCNavigationReflection.field(coordinator,"chatsTableViewController"),
              let frc=KCNavigationReflection.field(normal,"fetchedResultsController") as? NSFetchedResultsController<NSFetchRequestResult>,
              frc.managedObjectContext.concurrencyType == .mainQueueConcurrencyType,
              let stores=frc.managedObjectContext.persistentStoreCoordinator?.persistentStores,!stores.isEmpty else{return false}
        return true
    }
    @objc(diffableImplementation:) static func diffableImplementation(_ source:Any)->AnyObject? {
        let type=String(reflecting:Swift.type(of:source))
        guard type.hasPrefix("UIKit.UICollectionViewDiffableDataSource<") ||
              type=="FriendsFeedPresentation.FriendsListDataSource" ||
              type=="KakaoTalk.OpenChatsTableDiffableDataSource" ||
              type.hasPrefix("UIKit.UITableViewDiffableDataSource<"),
              let value=KCNavigationReflection.field(source,"_impl") as? NSObject,
              NSStringFromClass(Swift.type(of:value))=="__UIDiffableDataSource" else{return nil}
        return value
    }
    @objc(advertisingRemovalPlan:items:) static func advertisingRemovalPlan(_ sections:[Any],items:[Any])->NSDictionary? {
        guard sections.count<=256,items.count<=20_000 else{return nil}
        let s=sections.map(KCNavigationReflection.role),i=items.map(KCNavigationReflection.role)
        guard let plan=KCNavigationAdPolicy.removals(sections:s.map{($0.type,$0.role)},items:i.map{($0.type,$0.role)}) else{return nil}
        return ["sectionIndexes":plan.sections.map{NSNumber(value:$0)},"itemIndexes":plan.items.map{NSNumber(value:$0)}]
    }
    @objc(field:name:) static func field(_ object:Any,name:String)->AnyObject? {
        let type=String(reflecting:Swift.type(of:object))
        guard KCChannelNavigationPolicy.permittedField(type:type,name:name),let value=KCNavigationReflection.field(object,name) else{return nil}
        return value as AnyObject
    }
    @objc(moreCurrentRole:) static func moreCurrentRole(_ object:Any)->Int {
        guard String(reflecting:Swift.type(of:object))=="KakaoTalk.MoreTabHomeViewController",let value=KCNavigationReflection.field(object,"contentType") else{return -1}
        let role=KCNavigationReflection.role(value)
        guard role.type=="KakaoTalk.MoreTabContentType" else{return -1}
        if role.role=="home" {return 1}
        return ["wallet","game","dynamic"].contains(role.role) ? 0:-1
    }
    @objc(moreHomePaddingState:) static func moreHomePaddingState(_ object:Any)->NSDictionary? {
        guard Thread.isMainThread,String(reflecting:Swift.type(of:object))=="KakaoTalk.MoreTabHomeViewController",
              let context=KCNavigationReflection.field(object,"walletHomeLayoutContext"),let padding=moreContextPadding(context) else{return nil}
        return ["context":context as AnyObject,"padding":padding]
    }
    @objc(moreContextPadding:) static func moreContextPadding(_ context:Any)->NSNumber? {
        guard Thread.isMainThread,String(reflecting:Swift.type(of:context))=="WalletUI.WalletHomeLayoutContext",
              let value=KCNavigationReflection.field(context,"_topPadding"),String(reflecting:Swift.type(of:value))=="CoreGraphics.CGFloat",
              let padding=value as? CGFloat,padding.isFinite,padding>=0,padding<=500 else{return nil}
        return NSNumber(value:Double(padding))
    }
    @objc(nowCurrentRole:) static func nowCurrentRole(_ object:Any)->Int {
        guard String(reflecting:Swift.type(of:object))=="KakaoTalk.NowTabContainerViewController",let value=KCNavigationReflection.field(object,"currentType") else{return -1}
        let role=KCNavigationReflection.role(value)
        guard role.type=="TalkAppBase.NowSubTabType" else{return -1}
        return role.role=="openChat" ? 1:(["shortForm","brand"].contains(role.role) ? 0:-1)
    }
    @objc(moreHomeItemIndex:) static func moreHomeItemIndex(_ objects:[Any])->Int {
        let roles=objects.map(KCNavigationReflection.role)
        return KCNavigationAdPolicy.moreHomeIndex(roles.map{($0.type,$0.role)}) ?? -1
    }
    @objc(friendChannelItemIndexes:) static func friendChannelItemIndexes(_ objects:[Any])->[NSNumber]? {
        guard objects.count<=20_000 else{return nil}
        var roles=[KCFriendCategoryRole]()
        for object in objects {
            let value=KCNavigationReflection.value(object),type=String(reflecting:Swift.type(of:value)),m=Mirror(reflecting:value)
            guard type==KCChannelNavigationPolicy.friendItemType,m.displayStyle == .enum else{return nil}
            let outer=m.children.first
            guard let itemRole=outer?.label else {
                roles.append(.init(itemType:type,itemRole:"<other>",categoryType:nil,categoryRole:nil,payloadType:nil));continue
            }
            if itemRole != "category" {roles.append(.init(itemType:type,itemRole:itemRole,categoryType:nil,categoryRole:nil,payloadType:nil));continue}
            guard let value=outer?.value,let category=KCNavigationReflection.unwrap(value) else{return nil}
            let cm=Mirror(reflecting:category),ct=String(reflecting:Swift.type(of:category))
            guard ct==KCChannelNavigationPolicy.friendCategoryType,cm.displayStyle == .enum,let c=cm.children.first else{return nil}
            roles.append(.init(itemType:type,itemRole:itemRole,categoryType:ct,categoryRole:c.label,payloadType:String(reflecting:Swift.type(of:c.value))))
        }
        return KCChannelNavigationPolicy.friendChannelIndexes(roles)?.map{NSNumber(value:$0)}
    }
    @objc(channelKind:) static func channelKind(_ object:AnyObject)->Int {KCNavigationNative.classifies(object)}
    @objc(presentationUnread:) static func presentationUnread(_ controller:Any)->Int {
        guard Thread.isMainThread,KCNavigationNative.chatMetadataAvailable else{return -1}
        let type=String(reflecting:Swift.type(of:controller))
        let owner:Any
        if type=="KakaoTalk.ChannelChatsViewController",let vm=KCNavigationReflection.field(controller,"viewModel") {owner=vm}
        else if type=="KakaoTalk.ChatsArchiveTableViewController" {owner=controller}
        else if type=="KakaoTalk.OpenChatsTableViewController" {
            let symbols=["$s11TalkAppBase17UnreadCountHelperCMa","$s11TalkAppBase17UnreadCountHelperC6sharedACvgZ","$s11TalkAppBase17UnreadCountHelperC08openChatdE0Sivg"]
            for name in symbols {
                guard let pointer=dlsym(UnsafeMutableRawPointer(bitPattern:-2),name) else{return -1}
                var info=Dl_info()
                guard dladdr(pointer,&info) != 0,let path=info.dli_fname,
                      String(cString:path).hasSuffix("/KakaoTalk.app/KakaoTalk") else{return -1}
            }
            return UnreadCountHelper.shared.openChatUnreadCount
        }
        else{return -1}
        guard let frc=KCNavigationReflection.field(owner,"fetchedResultsController") as? NSFetchedResultsController<NSFetchRequestResult>,
              frc.managedObjectContext.concurrencyType == .mainQueueConcurrencyType,let objects=frc.fetchedObjects,
              objects.count<=50_000 else{return -1}
        for name in ["$s14TalkSharedBase15NSEUserSettingsCMa","$s14TalkSharedBase15NSEUserSettingsC6sharedACvgZ","$s14TalkSharedBase15NSEUserSettingsC24excludeBadgeForMutedChatSbvg"] {
            guard KCNavigationNative.frameworkSymbol(name) != nil else{return -1}
        }
        let excludeMuted=NSEUserSettings.shared.excludeBadgeForMutedChat
        var total=0
        for object in objects {
            guard let chat=object as? NSManagedObject,chat is Chat else{return -1}
            let sel=NSSelectorFromString("unreadCount")
            guard let method=class_getInstanceMethod(Swift.type(of:chat),sel),method_getNumberOfArguments(method)==2 else{return -1}
            let rt=method_copyReturnType(method);defer{free(rt)}
            guard String(cString:rt)=="q" else{return -1}
            typealias Getter = @convention(c) (AnyObject,Selector)->Int
            let count=unsafeBitCast(method_getImplementation(method),to:Getter.self)(chat,sel)
            if count<=0 {continue}
            if excludeMuted {
                let push=NSSelectorFromString("canPushAlert")
                guard let method=class_getInstanceMethod(Swift.type(of:chat),push),method_getNumberOfArguments(method)==2 else{return -1}
                let rt=method_copyReturnType(method);defer{free(rt)}
                guard String(cString:rt)=="B" else{return -1}
                typealias PushGetter = @convention(c) (AnyObject,Selector)->Bool
                if !unsafeBitCast(method_getImplementation(method),to:PushGetter.self)(chat,push){continue}
            }
            guard count<=Int.max-total else{return -1}
            total+=count
        }
        return total
    }
    @objc(excludingChannelPredicate) static func excludingChannelPredicate()->NSPredicate? {KCNavigationNative.predicate("excludingChannel")}
    @objc(excludingQuietEntryInContext:) static func excludingQuietEntryInContext(_ context:NSManagedObjectContext)->NSPredicate? {
        guard Thread.isMainThread,context.concurrencyType == .mainQueueConcurrencyType,
              let model=context.persistentStoreCoordinator?.managedObjectModel,
              let cls=NSClassFromString("SilentChat"),
              let entity=model.entities.first(where:{guard let name=$0.managedObjectClassName else{return false};return NSClassFromString(name)==cls}),
              let symbol=KCNavigationNative.frameworkSymbol("$s14TalkSharedBase13ChatPredicateO15excludingEntityySo11NSPredicateCSo19NSEntityDescriptionCFZ") else{return nil}
        return unsafeBitCast(symbol,to:(@convention(thin) (NSEntityDescription)->NSPredicate).self)(entity)
    }
    @objc(channelChatObjectID:) static func channelChatObjectID(_ object:Any)->NSManagedObjectID? {
        let value=KCNavigationReflection.value(object),type=String(reflecting:Swift.type(of:value)),mirror=Mirror(reflecting:value)
        guard type.hasPrefix("KakaoTalk.ChannelChatsViewModel."),type.hasSuffix(".Item"),mirror.displayStyle == .enum,
              let child=mirror.children.first,child.label=="channelChat" else{return nil}
        return child.value as? NSManagedObjectID
    }
    @objc(routeChannelObjectID:from:coordinator:) static func routeChannelObjectID(_ objectID:NSManagedObjectID,from controller:Any,coordinator:Any)->Bool {
        guard Thread.isMainThread,KCNavigationNative.chatMetadataAvailable,
              String(reflecting:Swift.type(of:coordinator))=="KakaoTalk.ChatFolderContentCoordinator" else{return false}
        let type=String(reflecting:Swift.type(of:controller)),owner:Any
        if type=="KakaoTalk.ChannelChatsViewController",let vm=KCNavigationReflection.field(controller,"viewModel") {owner=vm}
        else if type=="KakaoTalk.ChatsArchiveTableViewController" {owner=controller}
        else{return false}
        guard let frc=KCNavigationReflection.field(owner,"fetchedResultsController") as? NSFetchedResultsController<NSFetchRequestResult>,
              frc.managedObjectContext.concurrencyType == .mainQueueConcurrencyType,
              let chat=try? frc.managedObjectContext.existingObject(with:objectID) as? Chat,
              let route=KCNavigationReflection.field(coordinator,"onChatSelected") as? (Chat,Bool)->Void else{return false}
        guard let vc=controller as? UIViewController,!vc.isEditing else{return false}
        guard KCNavigationNative.boolProperty(chat,"isEnterAllowed")==true,
              let plus=KCNavigationNative.boolProperty(chat,"isPlusChat"),let partner=KCNavigationNative.boolProperty(chat,"isPartnerChat"),plus || partner else{return false}
        for name in ["isOpenChat","isSilentChat","isOpenSilentChat","isThreadChat","isNoticeCenterChat","isWarehouse","isKindOfSecretChat"] {
            guard KCNavigationNative.boolProperty(chat,name)==false else{return false}
        }
        route(chat,false)
        return true
    }

}
#endif
