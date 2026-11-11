import Foundation

@main struct ChannelNavigationTests {
    static func main() {
        var passed=0
        func check(_ value:@autoclosure ()->Bool,line:UInt=#line) {precondition(value(),"Navigation check at \(line)");passed+=1}
        let item=KCChannelNavigationPolicy.friendItemType,category=KCChannelNavigationPolicy.friendCategoryType
        func friend(_ role:String,_ cat:String?=nil,_ payload:String?=nil)->KCFriendCategoryRole {
            .init(itemType:item,itemRole:role,categoryType:cat==nil ? nil:category,categoryRole:cat,payloadType:payload)
        }
        // Delete only the actual followed-channel category. People, birthdays,
        // recommendations and unknown cases retain their identifiers/order.
        check(KCChannelNavigationPolicy.friendChannelIndexes([friend("friend"),friend("category","plusFriend","Swift.Int"),friend("category","birthday","Swift.Int"),friend("newNativeCase")])==[1])
        check(KCChannelNavigationPolicy.friendChannelIndexes([friend("category","plusFriend","Swift.String")])==nil)
        check(KCChannelNavigationPolicy.friendChannelIndexes([friend("category")])==nil)
        check(KCChannelNavigationPolicy.friendChannelIndexes([.init(itemType:"Other.Item",itemRole:"category",categoryType:category,categoryRole:"plusFriend",payloadType:"Swift.Int")])==nil)
        check(KCChannelNavigationPolicy.friendChannelIndexes(Array(repeating:friend("friend"),count:20001))==nil)
        check(KCChannelNavigationPolicy.friendChannelIndexes([])==[])
        check(!KCChannelNavigationPolicy.permittedField(type:"KakaoTalk.ChatsTableViewController",name:"message"))
        check(!KCChannelNavigationPolicy.permittedField(type:"Other.Controller",name:"fetchedResultsController"))
        let open="OpenLink.OpenChatsTableSection",openItem="OpenLink.OpenChatsTableItem"
        let plan=KCNavigationAdPolicy.removals(sections:[(open,"bizboard"),(open,"chat"),(open,"feed"),(open,"feedRecommends"),(open,"penalty"),(open,"unsupported")],items:[(openItem,"chat"),(openItem,"threadChat"),(openItem,"adFitBanner"),(openItem,"feedAD"),(openItem,"feed"),(openItem,"feedHidden"),(openItem,"penalty"),(openItem,"unsupported"),(openItem,"futureCase")])
        check(plan?.sections==[0,2,3]);check(plan?.items==[2,3,4,5])
        check(KCNavigationAdPolicy.removals(sections:[(open,"chat")],items:[("Other.Chat","feed")])==nil)
        check(KCNavigationAdPolicy.removals(sections:[(open,"chat"),("Other.Section","feed")],items:[])==nil)
        check(KCNavigationAdPolicy.removals(sections:[("Other.Section","bizboard")],items:[])==nil)
        for type in ["KakaoTalk.ChatListSection","KakaoTalk.ChatsTableViewController.ChatsTableSection","KakaoTalk.ChannelChatsViewModel.Section"] {
            let p=KCNavigationAdPolicy.removals(sections:[(type,"bizboard"),(type,"bookmarkEntry"),(type,"main")],items:[("opaque","personalChat")])
            check(p?.sections==[0] && p?.items==[])
            check(KCNavigationAdPolicy.removals(sections:[(type,"bizboard"),(type,"bizboard")],items:[])==nil)
        }
        let more="KakaoTalk.MoreTabContentType"
        check(KCNavigationAdPolicy.moreHomeIndex([(more,"game"),(more,"home")])==1)
        check(KCNavigationAdPolicy.moreHomeIndex([(more,"game")])==nil)
        check(KCNavigationAdPolicy.moreHomeIndex([(more,"home"),(more,"home")])==nil)
        check(KCNavigationAdPolicy.moreHomeIndex([("Other.Content","home")])==nil)
        check(KCNavigationAdPolicy.moreHomeIndex([(more,"home"),(more,"futureTab")])==nil)
        print("Passed \(passed) channel navigation policy checks (unknown types/payloads preserved)")
    }
}
