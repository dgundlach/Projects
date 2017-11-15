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

#import "HuluAddDialog.h"
#import "MainMenuController.h"

#import <BackRow/BRControllerStack.h>
#import <BackRow/BRTextMenuItemLayer.h>

@implementation HuluAddDialog

#define FEED_OPTION_COUNT 8

- (id)init
{
  [super init];
  [self setTitle:@"Hulu Feeds"];
  feeds_ = [[NSMutableArray arrayWithObjects: 
             @"http://hulu.com/feed/recent/videos", 
             @"http://hulu.com/feed/recent/shows", 
             @"http://hulu.com/feed/recent/movies", 
             @"http://hulu.com/feed/highest_rated/videos", 
             @"http://hulu.com/feed/popular/videos/today", 
             @"http://hulu.com/feed/popular/videos/this_week", 
             @"http://hulu.com/feed/popular/videos/this_month", 
             @"http://hulu.com/feed/popular/videos/all_time", nil] retain];
  titles_ = [[NSMutableArray arrayWithObjects: @"Recently Added Videos", 
              @"Recently Added Shows", @"Recently Added Movies", 
              @"Highest Rated Videos", @"Most Popular: Today", 
              @"Most Popular: This Week", @"Most Popular: This Month", 
              @"Most Popular: All Time", nil] retain];
  [self startAutoDiscovery];
  [[self list] setDatasource:self];
  return self;
}

- (void)dealloc
{
  [titles_ release];
  [feeds_ release];
  [profile_ release];
  [super dealloc];
}

// call-back for an item having been selected
- (void)itemSelected:(long)index
{  
  if( index < [feeds_ count] ){
    MainMenuController* main_ = [MainMenuController sharedInstance];
    [main_ addFeed:[feeds_ objectAtIndex:index] 
         withTitle:[titles_ objectAtIndex:index]];
  }
  [[self stack] popController];
}

// once Hulu parses show feeds
// www.hulu.com/feed/show_recommendations/<id>
// www.hulu.com/feed/subscriptions/<id>
# pragma mark Auto Discovery

- (void)startAutoDiscovery
{
  searching_ = YES;
  NSURL* url = [NSURL URLWithString:@"http://www.hulu.com/users/profile"];
  NSURLRequest* request = [NSURLRequest requestWithURL:url];
  if( ![[NSURLConnection alloc] initWithRequest:request
                                       delegate:self
                               startImmediately:YES] )
    NSLog(@"Failed to open connection for Hulu feed auto-discovery");
}

# pragma mark BRMenuListItemProvider

- (long)itemCount{ return [feeds_ count] + (searching_ ? 1 : 0) ; }
- (float)heightForRow:(long)row{ return 0; }
- (BOOL)rowSelectable:(long)row{ return !(row == 0 && searching_); }
- (NSString*)titleForRow:(long)row{ 
  if( searching_ ) row -= 1;
  return [titles_ objectAtIndex:row];
}
- (BRLayer<BRMenuItemLayer>*)itemForRow:(long)row
{
  BRTextMenuItemLayer* item;
  if( [self rowSelectable:row] ){
    item = [BRTextMenuItemLayer menuItem];
    [item setTitle:[self titleForRow:row]];
  }else{
    item = [BRTextMenuItemLayer progressMenuItem];
    [item setTitle:@"Looking for user profile"];
    [item setDimmed:YES];
  }
  return item;
}


# pragma mark NSURLConnection Delegation

- (void)connection:(NSURLConnection*)connection 
didReceiveResponse:(NSURLResponse*)response
{
  // a resonse with URL starting with www.hulu.com/users/profile means success
  NSString* path = [[response URL] path];
  if( [path hasPrefix:@"/users/profile"] )
  {
    NSString* feed;
    profile_ = [[path lastPathComponent] retain];
    [connection cancel];
    
    feed = @"www.hulu.com/feed/subscriptions/";
    feed = [feed stringByAppendingString:profile_];
    [feeds_ insertObject:feed atIndex:0];
    [titles_ insertObject:@"Subscriptions" atIndex:0];

    feed = @"www.hulu.com/feed/show-recommendations/";
    feed = [feed stringByAppendingString:profile_];
    [feeds_ insertObject:feed atIndex:0];
    [titles_ insertObject:@"Recommended Shows" atIndex:0];
    
    feed = @"www.hulu.com/feed/recommendations/";
    feed = [feed stringByAppendingString:profile_];
    [feeds_ insertObject:feed atIndex:0];
    [titles_ insertObject:@"Recommended Videos" atIndex:0];
    
    feed = [@"www.hulu.com/feed/queue/" stringByAppendingString:profile_];
    [feeds_ insertObject:feed atIndex:0];
    [titles_ insertObject:@"My Queue" atIndex:0];
    
    searching_ = NO;
    [[self list] reload];
  }
}

- (void)connection:(NSURLConnection*)connection 
  didFailWithError:(NSError*)error
{
  [connection release];
  searching_ = NO;
  [[self list] reload];
}

- (void)connectionDidFinishLoading:(NSURLConnection*)connection
{
  [connection release];
  searching_ = NO;
  [[self list] reload];
}

@end
