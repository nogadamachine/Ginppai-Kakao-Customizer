// Foundation fixtures exercise real Objective-C hook ABIs and reuse isolation.
// UIKit rendering and compositional layouts still require device validation.
#import <Foundation/Foundation.h>
#import <CoreGraphics/CoreGraphics.h>
#import <objc/runtime.h>
#import <dispatch/dispatch.h>
typedef float UILayoutPriority;
static const CGFloat UIViewNoIntrinsicMetric=-1;
typedef NS_ENUM(NSInteger,NSLayoutAttribute){NSLayoutAttributeWidth=7,NSLayoutAttributeHeight=8};
#define NSLayoutConstraint KCSpacingConstraint
@class UIView;
@interface NSLayoutConstraint:NSObject
@property BOOL active;
@property(nonatomic,weak) id firstItem;
@property(nonatomic,weak) id secondItem;
@property NSLayoutAttribute firstAttribute;
@property CGFloat constant;
@end
@implementation NSLayoutConstraint
@end
@interface UIView:NSObject
@property(nonatomic,weak) UIView *superview;
@property(nonatomic,weak) UIView *window;
@property(nonatomic,strong) NSArray *constraints;
@property NSUInteger intrinsicInvalidations;
- (CGSize)sizeThatFits:(CGSize)size;
- (CGSize)intrinsicContentSize;
- (CGSize)systemLayoutSizeFittingSize:(CGSize)size;
- (CGSize)systemLayoutSizeFittingSize:(CGSize)size withHorizontalFittingPriority:(UILayoutPriority)horizontal verticalFittingPriority:(UILayoutPriority)vertical;
- (void)invalidateIntrinsicContentSize;
+ (void)performWithoutAnimation:(void(^)(void))block;
@end
@implementation UIView
- (instancetype)init{if((self=[super init]))_constraints=@[];return self;}
- (CGSize)sizeThatFits:(CGSize)size{return CGSizeMake(size.width,86);}
- (CGSize)intrinsicContentSize{return CGSizeMake(402,86);}
- (CGSize)systemLayoutSizeFittingSize:(CGSize)size{return CGSizeMake(size.width,86);}
- (CGSize)systemLayoutSizeFittingSize:(CGSize)size withHorizontalFittingPriority:(UILayoutPriority)horizontal verticalFittingPriority:(UILayoutPriority)vertical{return CGSizeMake(size.width,86);}
- (void)invalidateIntrinsicContentSize{self.intrinsicInvalidations++;}
+ (void)performWithoutAnimation:(void(^)(void))block{block();}
@end
@interface UITableViewCell:UIView
@end
@implementation UITableViewCell
@end
@interface UITableView:UIView
@property(nonatomic,weak) id delegate;
@property(nonatomic,strong) NSMapTable *cellPaths;
@property BOOL hasUncommittedUpdates;
@property NSUInteger updates;
- (NSIndexPath *)indexPathForCell:(UITableViewCell *)cell;
- (void)beginUpdates;
- (void)endUpdates;
@end
@implementation UITableView
- (instancetype)init{if((self=[super init]))_cellPaths=[NSMapTable weakToStrongObjectsMapTable];return self;}
- (NSIndexPath *)indexPathForCell:(UITableViewCell *)cell{return [self.cellPaths objectForKey:cell];}
- (void)beginUpdates{self.updates++;}
- (void)endUpdates{}
@end
@interface UICollectionViewLayoutAttributes:NSObject<NSCopying>
@property CGSize size;
@end
@implementation UICollectionViewLayoutAttributes
- (id)copyWithZone:(NSZone *)zone{UICollectionViewLayoutAttributes *a=[UICollectionViewLayoutAttributes new];a.size=self.size;return a;}
@end
@interface UICollectionViewCell:UIView
- (UICollectionViewLayoutAttributes *)preferredLayoutAttributesFittingAttributes:(UICollectionViewLayoutAttributes *)input;
@end
@implementation UICollectionViewCell
- (UICollectionViewLayoutAttributes *)preferredLayoutAttributesFittingAttributes:(UICollectionViewLayoutAttributes *)input{return input;}
@end
@interface UICollectionViewLayout:NSObject
@property NSUInteger invalidations;
- (void)invalidateLayout;
@end
@implementation UICollectionViewLayout
- (void)invalidateLayout{self.invalidations++;}
@end
@interface UICollectionView:UIView
@property(nonatomic,weak) id delegate;
@property(nonatomic,strong) NSMapTable *cellPaths;
@property(nonatomic,strong) UICollectionViewLayout *collectionViewLayout;
- (NSIndexPath *)indexPathForCell:(UICollectionViewCell *)cell;
@end
@implementation UICollectionView
- (instancetype)init{if((self=[super init])){_cellPaths=[NSMapTable weakToStrongObjectsMapTable];_collectionViewLayout=[UICollectionViewLayout new];}return self;}
- (NSIndexPath *)indexPathForCell:(UICollectionViewCell *)cell{return [self.cellPaths objectForKey:cell];}
@end
@interface SpacingDelegate:NSObject
@property NSUInteger normalHeightCalls;
- (CGFloat)tableView:(UITableView *)table heightForRowAtIndexPath:(NSIndexPath *)path;
- (CGSize)collectionView:(UICollectionView *)collection layout:(id)layout sizeForItemAtIndexPath:(NSIndexPath *)path;
@end
@implementation SpacingDelegate
- (CGFloat)tableView:(UITableView *)table heightForRowAtIndexPath:(NSIndexPath *)path{self.normalHeightCalls++;return 86;}
- (CGSize)collectionView:(UICollectionView *)collection layout:(id)layout sizeForItemAtIndexPath:(NSIndexPath *)path{return CGSizeMake(402,86);}
@end
@interface BadSpacingDelegate:NSObject
- (id)tableView:(id)table heightForRowAtIndexPath:(id)path;
@end
@implementation BadSpacingDelegate
- (id)tableView:(id)table heightForRowAtIndexPath:(id)path{return @86;}
@end
#define KC_AD_SPACING_TESTING 1
static void event(NSString *key){}
static BOOL option(NSString *key){return YES;}
static void hideClass(NSString *name){}
static void replace(Class cls,SEL sel,IMP imp,const char *types){if(!class_addMethod(cls,sel,imp,types))class_replaceMethod(cls,sel,imp,types);}
#include "AdSpacing.inc"
static void require(BOOL ok,NSString *name){if(!ok){NSLog(@"FAIL: %@",name);exit(1);}}
static void drain(void){NSDate *until=[NSDate dateWithTimeIntervalSinceNow:0.02];while(until.timeIntervalSinceNow>0)[NSRunLoop.currentRunLoop runUntilDate:until];}
int main(void){@autoreleasepool{
    Class adClass=objc_allocateClassPair(UITableViewCell.class,"TalkAppBase.BizboardCell",0);objc_registerClassPair(adClass);
    Class collectionClass=objc_allocateClassPair(UICollectionViewCell.class,"TalkAppBase.BizboardCollectionCell",0);objc_registerClassPair(collectionClass);
    Class moreClass=objc_allocateClassPair(UIView.class,"MoreTab.LocalBizboardUIView",0);objc_registerClassPair(moreClass);
    installAdSpacing();
    UITableViewCell *cell=[adClass new];UITableView *table=[UITableView new];table.window=table;cell.superview=table;
    SpacingDelegate *delegate=[SpacingDelegate new];table.delegate=delegate;
    NSIndexPath *p0=[NSIndexPath indexPathWithIndex:0],*p1=[NSIndexPath indexPathWithIndex:1];
    [table.cellPaths setObject:p0 forKey:cell];
    kcAdRegisterCell(cell);kcAdRegisterCell(cell);drain();
    require(table.updates==1,@"coalesced table refresh, no layout loop");
    require([delegate tableView:table heightForRowAtIndexPath:p0]==0.01,@"fixed 86-point ad row collapses");
    require([delegate tableView:table heightForRowAtIndexPath:p1]==86,@"ordinary row keeps original height");
    require(delegate.normalHeightCalls==1,@"ordinary rows still invoke original delegate");
    [table.cellPaths setObject:p1 forKey:cell];
    require([delegate tableView:table heightForRowAtIndexPath:p0]==86,@"snapshot move cannot collapse stale row");
    kcAdRegisterCell(cell);drain();
    require([delegate tableView:table heightForRowAtIndexPath:p1]==0.01 && [kcAdCells(table) objectForKey:p0]==nil,@"reuse updates path and removes stale key");
    UITableView *otherTable=[UITableView new];otherTable.delegate=delegate;
    require([delegate tableView:otherTable heightForRowAtIndexPath:p1]==86,@"shared delegate leaves other lists unchanged");
    BadSpacingDelegate *bad=[BadSpacingDelegate new];otherTable.delegate=bad;
    require(!kcAdInstallListSizing(otherTable),@"incorrect return ABI rejected");
    NSObject *missing=[NSObject new];otherTable.delegate=missing;require(!kcAdInstallListSizing(otherTable),@"missing optional delegate method not synthesized");
    [table.cellPaths removeObjectForKey:cell];require([delegate tableView:table heightForRowAtIndexPath:p1]==86,@"detached cell no longer collapses row");
    require([cell systemLayoutSizeFittingSize:CGSizeMake(402,100)].height==0,@"self-sizing table height");
    require([cell systemLayoutSizeFittingSize:CGSizeMake(402,100) withHorizontalFittingPriority:1000 verticalFittingPriority:50].height==0,@"priority fitting struct ABI");
    require([cell sizeThatFits:CGSizeMake(402,100)].width==402 && cell.intrinsicContentSize.height==0,@"width preserved and intrinsic height zero");
    UIView *normal=[UIView new];require([normal sizeThatFits:CGSizeMake(402,100)].height==86,@"UIKit superclass implementation unchanged");
    UIView *more=[moreClass new];require([more sizeThatFits:CGSizeMake(402,100)].height==0,@"More banner uses sizing hook");
    NSLayoutConstraint *own=[NSLayoutConstraint new];own.active=YES;own.firstItem=more;own.firstAttribute=NSLayoutAttributeHeight;own.constant=86;
    NSLayoutConstraint *relative=[NSLayoutConstraint new];relative.active=YES;relative.firstItem=more;relative.secondItem=normal;relative.firstAttribute=NSLayoutAttributeHeight;relative.constant=8;
    more.constraints=@[own,relative];kcAdCollapseOwnHeight(more);kcAdCollapseOwnHeight(more);
    require(own.constant==0 && relative.constant==8,@"only ad constant height altered");
    require(more.intrinsicInvalidations==1,@"intrinsic invalidation does not loop");
    UICollectionViewCell *item=[collectionClass new];UICollectionView *collection=[UICollectionView new];collection.window=collection;collection.delegate=delegate;item.superview=collection;
    [collection.cellPaths setObject:p0 forKey:item];kcAdRegisterCell(item);kcAdRegisterCell(item);drain();
    require(collection.collectionViewLayout.invalidations==1,@"collection invalidation coalesced");
    CGSize adSize=[delegate collectionView:collection layout:collection.collectionViewLayout sizeForItemAtIndexPath:p0];
    require(adSize.width==402 && adSize.height==0.01,@"flow layout size preserves width and collapses height");
    require([delegate collectionView:collection layout:collection.collectionViewLayout sizeForItemAtIndexPath:p1].height==86,@"ordinary collection item unchanged");
    UICollectionViewLayoutAttributes *attrs=[UICollectionViewLayoutAttributes new];attrs.size=CGSizeMake(402,86);
    UICollectionViewLayoutAttributes *result=[item preferredLayoutAttributesFittingAttributes:attrs];
    require(result!=attrs && attrs.size.height==86 && result.size.height==0.01,@"self-sizing attributes copied, input not mutated");
    __weak UITableViewCell *weakCell;
    @autoreleasepool{UITableViewCell *temporary=[adClass new];temporary.superview=table;weakCell=temporary;[table.cellPaths setObject:p0 forKey:temporary];kcAdRegisterCell(temporary);}
    require(weakCell==nil,@"tracking does not retain ad cells");
    UITableView *delayed=[UITableView new];delayed.delegate=delegate;
    UITableViewCell *delayedCell=[adClass new];delayedCell.superview=delayed;[delayed.cellPaths setObject:p0 forKey:delayedCell];
    kcAdRegisterCell(delayedCell);drain();require(delayed.updates==0,@"offscreen list not refreshed");
    delayed.window=delayed;kcAdRegisterCell(delayedCell);drain();require(delayed.updates==1,@"attachment retries pending layout");
    UITableView *busy=[UITableView new];busy.delegate=delegate;busy.window=busy;busy.hasUncommittedUpdates=YES;
    UITableViewCell *busyCell=[adClass new];busyCell.superview=busy;[busy.cellPaths setObject:p0 forKey:busyCell];
    kcAdRegisterCell(busyCell);drain();require(busy.updates==0,@"uncommitted table update left alone");
    busy.hasUncommittedUpdates=NO;kcAdRegisterCell(busyCell);drain();require(busy.updates==1,@"layout retries after uncommitted update");
    __weak UITableView *weakList;
    UITableViewCell *survivor=[adClass new];
    @autoreleasepool{UITableView *temporaryList=[UITableView new];temporaryList.delegate=delegate;temporaryList.window=temporaryList;survivor.superview=temporaryList;[temporaryList.cellPaths setObject:p0 forKey:survivor];weakList=temporaryList;kcAdRegisterCell(survivor);}
    drain();require(weakList==nil,@"cell marker and queued refresh do not retain lists");
    require(!kcIsAdSpacingCell(normal),@"normal and birthday rows excluded from whitelist");
    puts("Ad spacing tests passed: fixed/self-sizing rows, collection ABI, reuse, isolation, constraints and coalescing");
}return 0;}
