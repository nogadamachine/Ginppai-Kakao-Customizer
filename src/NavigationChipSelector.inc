// Matches ChatFolder's independent capsule chips, with separate accessible
// buttons and horizontal scrolling rather than a segmented-control background.
@interface KCNavigationChipButton:UIButton
@property(nonatomic,strong) UILabel *unreadBadge;
@end
@implementation KCNavigationChipButton
- (BOOL)pointInside:(CGPoint)point withEvent:(UIEvent *)event {return CGRectContainsPoint(CGRectInset(self.bounds,0,-MAX(0,(44-self.bounds.size.height)/2)),point);}
- (void)layoutSubviews {
    [super layoutSubviews];if(self.unreadBadge.hidden)return;
    CGSize text=[[self titleForState:UIControlStateNormal] sizeWithAttributes:@{NSFontAttributeName:self.titleLabel.font}];
    CGFloat badgeWidth=MAX(18,ceil([self.unreadBadge.text sizeWithAttributes:@{NSFontAttributeName:self.unreadBadge.font}].width)+8);
    CGFloat left=(self.bounds.size.width-text.width-6-badgeWidth)/2;
    self.titleLabel.frame=CGRectMake(left,(self.bounds.size.height-text.height)/2,text.width,text.height);
    self.unreadBadge.frame=CGRectMake(left+text.width+6,(self.bounds.size.height-18)/2,badgeWidth,18);
}
@end
@interface KCNavigationChipSelector:UIControl
@property(nonatomic) NSInteger selectedSegmentIndex;
@property(nonatomic) CGFloat horizontalInset;
- (instancetype)initWithItems:(NSArray<NSString *> *)items;
- (void)setTitle:(NSString *)title forSegmentAtIndex:(NSUInteger)index;
- (void)setUnreadCount:(NSInteger)count forSegmentAtIndex:(NSUInteger)index;
- (void)setTitleTextAttributes:(NSDictionary *)attributes forState:(UIControlState)state;
@end
@implementation KCNavigationChipSelector {
    UIScrollView *_scroll;
    NSMutableArray<KCNavigationChipButton *> *_chips;
    UIFont *_normalFont,*_selectedFont;
}
- (instancetype)initWithItems:(NSArray<NSString *> *)items {
    if((self=[super initWithFrame:CGRectZero])) {
        _selectedSegmentIndex=0;_chips=[NSMutableArray new];
        _scroll=[UIScrollView new];_scroll.showsHorizontalScrollIndicator=NO;_scroll.alwaysBounceVertical=NO;
        _scroll.contentInsetAdjustmentBehavior=UIScrollViewContentInsetAdjustmentNever;[self addSubview:_scroll];
        for(NSString *title in items) {
            KCNavigationChipButton *button=[[KCNavigationChipButton alloc] initWithFrame:CGRectZero];button.tag=_chips.count;
            button.unreadBadge=[UILabel new];button.unreadBadge.hidden=YES;button.unreadBadge.textAlignment=NSTextAlignmentCenter;
            button.unreadBadge.font=[UIFont boldSystemFontOfSize:11];button.unreadBadge.backgroundColor=UIColor.systemRedColor;
            button.unreadBadge.textColor=UIColor.whiteColor;button.unreadBadge.layer.cornerRadius=9;button.unreadBadge.clipsToBounds=YES;
            button.unreadBadge.isAccessibilityElement=NO;[button addSubview:button.unreadBadge];
            [button setTitle:title forState:UIControlStateNormal];
            [button addTarget:self action:@selector(chipPressed:) forControlEvents:UIControlEventTouchUpInside];
            button.accessibilityTraits=UIAccessibilityTraitButton;[_scroll addSubview:button];[_chips addObject:button];
        }
        self.isAccessibilityElement=NO;[self updateChips];
    }return self;
}
- (void)chipPressed:(UIButton *)sender {
    self.selectedSegmentIndex=sender.tag;
    [self sendActionsForControlEvents:UIControlEventValueChanged];
}
- (void)setSelectedSegmentIndex:(NSInteger)index {
    if(index<0 || index>=(NSInteger)_chips.count)return;
    _selectedSegmentIndex=index;[self updateChips];
}
- (void)setHorizontalInset:(CGFloat)inset {_horizontalInset=MAX(0,inset);[self setNeedsLayout];}
- (void)setTitle:(NSString *)title forSegmentAtIndex:(NSUInteger)index {
    if(index>=_chips.count || [title isEqualToString:[_chips[index] titleForState:UIControlStateNormal]])return;
    [_chips[index] setTitle:title forState:UIControlStateNormal];[self setNeedsLayout];
}
- (void)setUnreadCount:(NSInteger)count forSegmentAtIndex:(NSUInteger)index {
    if(index>=_chips.count)return;KCNavigationChipButton *button=_chips[index];BOOL hidden=count<=0;
    NSString *text=count>999?@"999+":[@(MAX(0,count)) stringValue];
    if(button.unreadBadge.hidden==hidden && [button.unreadBadge.text isEqualToString:text])return;
    button.unreadBadge.hidden=hidden;button.unreadBadge.text=text;
    button.accessibilityValue=hidden?nil:[NSString stringWithFormat:@"읽지 않은 메시지 %@개",text];[self setNeedsLayout];
}
- (void)setTitleTextAttributes:(NSDictionary *)attributes forState:(UIControlState)state {
    UIFont *font=attributes[NSFontAttributeName];if(![font isKindOfClass:UIFont.class])return;
    if(state==UIControlStateSelected)_selectedFont=font;else _normalFont=font;
    [self updateChips];
}
- (void)updateChips {
    for(NSUInteger index=0;index<_chips.count;index++) {
        UIButton *button=_chips[index];BOOL selected=(NSInteger)index==self.selectedSegmentIndex;
        button.selected=selected;
        UIFont *font=selected?_selectedFont:_normalFont;
        button.titleLabel.font=font?:[UIFont preferredFontForTextStyle:UIFontTextStyleHeadline];
        button.titleLabel.adjustsFontForContentSizeCategory=YES;
        UIColor *foreground=selected?UIColor.systemBackgroundColor:UIColor.secondaryLabelColor;
        [button setTitleColor:foreground forState:UIControlStateNormal];[button setTitleColor:foreground forState:UIControlStateSelected];
        button.backgroundColor=selected?UIColor.labelColor:UIColor.tertiarySystemFillColor;
        button.accessibilityLabel=[button titleForState:UIControlStateNormal];
        button.accessibilityTraits=UIAccessibilityTraitButton|(selected?UIAccessibilityTraitSelected:0);
    }[self setNeedsLayout];
}
- (void)layoutSubviews {
    [super layoutSubviews];_scroll.frame=self.bounds;
    CGFloat x=self.horizontalInset,height=MAX(38,MIN(60,[_chips.firstObject.titleLabel.font lineHeight]+18));
    CGFloat y=MAX(0,MIN(2,(self.bounds.size.height-height)/2));
    for(KCNavigationChipButton *button in _chips) {
        CGFloat width=MAX(58,ceil([[button titleForState:UIControlStateNormal] sizeWithAttributes:@{NSFontAttributeName:button.titleLabel.font}].width)+24);
        if(!button.unreadBadge.hidden)width+=6+MAX(18,ceil([button.unreadBadge.text sizeWithAttributes:@{NSFontAttributeName:button.unreadBadge.font}].width)+8);
        button.frame=CGRectMake(x,y,width,height);button.layer.cornerRadius=height/2;
        x+=width+6;
    }
    _scroll.contentSize=CGSizeMake(MAX(self.bounds.size.width,x-6+self.horizontalInset),MAX(self.bounds.size.height,height));
}
@end
