//
//  Copyright 2008-2009 Kirk Kelsey.
//
//  This file is part of Understudy.
//
//  Understudy is free software: you can redistribute it and/or modify it under
//  the terms of the GNU Lesser General Public License as published by the Free
//  Software Foundation, either version 3 of the License, or (at your option)
//  any later version.
//
//  Understudy is distributed in the hope that it will be useful, but WITHOUT 
//  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or 
//  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License
//  for more details.
//
//  You should have received a copy of the GNU Lesser General Public License
//  along with Understudy.  If not, see <http://www.gnu.org/licenses/>.

#import "HuluFeed.h"
#import "NetflixFeed.h"
#import "YouTubeFeed.h"

#import "MainMenuController.h"
#import "ManageFeedsDialog.h"

#import <BackRow/BRControllerStack.h>
#import <BackRow/BRTextMenuItemLayer.h>
#import <BackRow/RUIPreferences.h>

@interface MainMenuController (PrivateMenuHandling)
- (void)_buildMenu;
- (NSObject<UnderstudyAsset>*)assetForRow:(long)row;
@end

@implementation MainMenuController

- (id)init
{
  [super init];
  [self _buildMenu];
  [self setListTitle:@"Understudy"];
  [[self list] setDatasource:self];
  return self;
}

- (void)dealloc
{
  [feeds_ release];
  [titles_ release];
  [assets_ release];
  [super dealloc];
}

// we'll assume that FR won't try to release or copy the controller
static MainMenuController *sharedInstance_;
+ (MainMenuController*)sharedInstance
{
  if(!sharedInstance_) sharedInstance_ = [[MainMenuController alloc] init];
  return sharedInstance_;
}

void upgradePrefs(RUIPreferences* FRprefs)
{
  // versions up to 0.2 used "hulu" as the name for the feeds information
  NSDictionary* prefDict = (NSDictionary*) [FRprefs objectForKey:@"hulu"];
  if( prefDict )
  {
    prefDict = (NSDictionary*) [prefDict objectForKey:@"feeds"];
    // we now use an array of feeds, another array of titles
    NSMutableDictionary* newprefs = [NSMutableDictionary dictionary];
    NSArray* feeds = [prefDict allKeys];
    NSMutableArray* titles = [NSMutableArray array];
    for(NSString* url in feeds) [titles addObject:[prefDict objectForKey:url]];
    [newprefs setObject:titles forKey:@"titles"];
    [newprefs setObject:feeds forKey:@"feeds"];
    [FRprefs setObject:newprefs forKey:@"understudy"];
    [FRprefs setObject:nil forKey:@"hulu"];
  }
}

- (void)_buildMenu
{  
  RUIPreferences* FRprefs = [RUIPreferences sharedFrontRowPreferences];
  upgradePrefs(FRprefs);
  NSDictionary* prefDict = (NSDictionary*) [FRprefs objectForKey:@"understudy"];
  feeds_ = [[[prefDict objectForKey:@"feeds"] mutableCopy] retain];
  titles_ = [[[prefDict objectForKey:@"titles"] mutableCopy] retain];

  if(!feeds_) feeds_ = [[NSMutableArray alloc] init];
  if(!titles_) titles_ = [[NSMutableArray alloc] init];

  assets_ = [[NSMutableArray arrayWithCapacity:[titles_ count]] retain];
  for( id t in titles_ ) [assets_ addObject:[NSNull null]];
  [assets_ addObject:[[[ManageFeedsDialog alloc] init] autorelease]];
  [[self list] addDividerAtIndex:[titles_ count] withLabel:nil];
  [[self list] reload];
}

- (void)savePreferences
{
  RUIPreferences* FRprefs = [RUIPreferences sharedFrontRowPreferences];
  NSMutableDictionary* prefs;
  prefs = [[FRprefs objectForKey:@"understudy"] mutableCopy];
  if( !prefs ) prefs = [NSMutableDictionary dictionary];
  [prefs setObject:titles_ forKey:@"titles"];
  [prefs setObject:feeds_ forKey:@"feeds"];
  [FRprefs setObject:prefs forKey:@"understudy"];
}

- (void)addFeed:(NSString*)feedURL withTitle:(NSString*)title
{
  // ensure no duplicate titles
  if( [titles_ containsObject:title] )
  {
    NSString* format = @"%@ %d", *newtitle;
    int i = 0;
    do{
      newtitle = [NSString stringWithFormat:format,title,i++];
    }while( [titles_ containsObject:newtitle]);
    title = newtitle;
  }
  [[self list] removeDividers];
  [feeds_ addObject:feedURL];
  [titles_ addObject:title];
  [assets_ insertObject:[NSNull null] atIndex:([assets_ count]-1)];
  [[self list] addDividerAtIndex:[titles_ count] withLabel:nil];
  [[self list] reload];
  [self savePreferences];
}

- (void)moveFeedFromIndex:(long)from toIndex:(long)to
{
  NSObject* item;
  // ensure the values are valid
  if( from < 0 || ([assets_ count]-1) < from 
     || to < 0 || ([assets_ count]-1) < to 
     || to == from ) return;

  // if the |to| position is after the from, the new index must be decremented to 
  // acount for the item no longer being the array by the time is't added
  if( from < to ) --to;
  
  // move the feed
  item = [feeds_ objectAtIndex:from];
  [feeds_ removeObjectAtIndex:from];
  [feeds_ insertObject:item atIndex:to];
  // move the title
  item = [titles_ objectAtIndex:from];
  [titles_ removeObjectAtIndex:from];
  [titles_ insertObject:item atIndex:to];
  // move the asset
  item = [assets_ objectAtIndex:from];
  [assets_ removeObjectAtIndex:from];
  [assets_ insertObject:item atIndex:to];
  [[self list] reload];
  [self savePreferences];
}

- (void)removeFeedAtIndex:(long)index
{
  [[self list] removeDividers];
  [feeds_ removeObjectAtIndex:index];
  [titles_ removeObjectAtIndex:index];
  [assets_ removeObjectAtIndex:index];
  [[self list] addDividerAtIndex:[titles_ count] withLabel:nil];
  [[self list] reload];
  [self savePreferences];
}

- (void)renameFeedAtIndex:(long)index withTitle:(NSString*)title
{
  [titles_ replaceObjectAtIndex:index withObject:[[title copy]autorelease]];
  [assets_ replaceObjectAtIndex:index withObject:[NSNull null]];
  [[self list] reload];
  [self savePreferences];
}

- (NSObject<UnderstudyAsset>*)assetForRow:(long)row
{
  NSObject<UnderstudyAsset>* asset = [assets_ objectAtIndex:row];
  if( (id)asset == (id)[NSNull null] ){
    NSString* feed = [feeds_ objectAtIndex:row];
    NSString* title = [titles_ objectAtIndex:row];
    NSURL* url = [NSURL URLWithString:feed];
    NSString* host = [[url host] lowercaseString];
    
    if( [host rangeOfString:@"hulu"].location != NSNotFound )
      asset = [[HuluFeed alloc] initWithTitle:title forUrl:url];
    else if( [host rangeOfString:@"netflix"].location != NSNotFound )
      asset = [[NetflixFeed alloc] initWithTitle:title forUrl:url];
    else if( [host rangeOfString:@"youtube"].location != NSNotFound )
      asset = [[YouTubeFeed alloc] initWithTitle:title forUrl:url];
    else asset = (NSObject<UnderstudyAsset>*)[NSNull null];
    
    [assets_ replaceObjectAtIndex:row withObject:asset];
    [asset autorelease];
  }
  return asset;
}

#pragma mark Back Row subclassing
  
- (long)itemCount
{
  return [assets_ count];
}

- (NSString*)titleForRow:(long)row
{
  return [(BRTextMenuItemLayer*)[self itemForRow:row] title];
}

- (BRLayer<BRMenuItemLayer>*)itemForRow:(long)row
{
  NSObject<UnderstudyAsset>* asset = [self assetForRow:row];
  return [asset menuItem];
}

-(float)heightForRow:(long)row
{
  return 0; //0 tells FR to use its standard height
}

-(BOOL)rowSelectable:(long)row
{
  return YES;
}

- (void)itemSelected:(long)itemIndex
{
  NSObject<UnderstudyAsset>* asset = [self assetForRow:itemIndex];
  [[self stack] pushController:[asset controller]];
}

- (BRControl*)previewControlForItem:(long)itemIndex
{
  NSLog(@"preview for index %d",itemIndex);
  BaseUnderstudyAsset<UnderstudyAsset>* asset;
  asset = [assets_ objectAtIndex:itemIndex];
  if( (id)asset != [NSNull null] ) NSLog(@"asset isn't nil");
  return [asset preview];
}

@end
