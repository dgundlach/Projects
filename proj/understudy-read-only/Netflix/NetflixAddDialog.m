//
//  Copyright 2008 Kirk Kelsey.
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

#import "MainMenuController.h"
#import "NetflixAddDialog.h"

#import <BackRow/BRControllerStack.h>

@interface NetflixAddDialog (PrivateMethods)
- (void)_startAutoDiscovery;
@end

@implementation NetflixAddDialog

#define FEED_OPTION_COUNT 4

NSString* NETFLIXURLS[] = {
@"http://netflix.com/NewWatchInstantlyRSS",
@"http://netflix.com/TopWatchInstantlyRSS",
@"http://netflix.com/TopWatchInstantlyThisWeekRSS",
@"http://netflix.com/TopWatchInstantlyPastThreeMonthsRSS"};

NSString* NETFLIXTITLES[] = { 
@"New Choices to Watch Instantly",
@"All Time Top Choices",
@"Last Week's Top Choices",
@"Last 3 Month's Top Choices"};

- (id)init
{
  [super init];
  [self setTitle:@"Netflix Feeds"];
  int i = 0;
  for( i=0; i<FEED_OPTION_COUNT; i++) [self addOptionText:NETFLIXTITLES[i]];
  [self setActionSelector:@selector(itemSelected) target:self];
  // start the autodiscovery process
  [self _startAutoDiscovery];
  return self;
}

- (void)dealloc
{
  [super dealloc];
  [pageData_ release];
  [queue_ release];
}

// call-back for an item having been selected
- (void)itemSelected
{
  MainMenuController* main_ = [MainMenuController sharedInstance];
  int index = [self selectedIndex];
  if( index < FEED_OPTION_COUNT ){
    [main_ addFeed:NETFLIXURLS[index] withTitle:NETFLIXTITLES[index]];
  }else if( index == FEED_OPTION_COUNT ){
    [main_ addFeed:queue_ withTitle:@"Netflix Queue"];
  }else NSLog(@"unexpected option selected for Netflix");
  // we might want to pop back to the main menu
  [[self stack] popController];
}

# pragma mark Auto Discovery

// load the web page http://www.netflix.com/RSSFeeds and look for a link to the
// users watch instantly queue. This requires that they be logged in through
// Safari
- (void)_startAutoDiscovery
{
  NSURL* url = [NSURL URLWithString:@"http://www.netflix.com/RSSFeeds"];
  NSURLRequest* request = [NSURLRequest requestWithURL:url];
  NSURLConnection* connection = [[NSURLConnection alloc] initWithRequest:request
                                                                delegate:self
                                                        startImmediately:YES];
  if(connection) pageData_ = [[NSMutableData data] retain];
  else NSLog(@"Failed to open connection for Netflix feed auto-discovery");
}

- (void)_autoDiscover
{
  NSStringEncoding encoding = NSISOLatin1StringEncoding;
  NSString *contents;
  NSRange start, end, searchRange, queueRange;
  
  contents = [[NSString alloc] initWithData:pageData_ encoding:encoding];
  start = [contents rangeOfString:@"http://rss.netflix.com/QueueEDRSS?id="];
  searchRange = NSMakeRange(NSMaxRange(start), 
                            [contents length]-NSMaxRange(start));
  end = [contents rangeOfString:@"\"" options:0 range:searchRange]; 
  if( start.location == NSNotFound || end.location == NSNotFound) return;
  queueRange = NSMakeRange(start.location, end.location-start.location);
  queue_ = [[contents substringWithRange:queueRange] retain];
  [self addOptionText:@"My Watch Instantly Queue"];
}

# pragma mark NSURLConnection Delegation

- (void)connection:(NSURLConnection*)connection 
didReceiveResponse:(NSURLResponse*)response
{
  [pageData_ setLength:0];  
}

- (void)connection:(NSURLConnection*)connection 
  didFailWithError:(NSError*)error
{
  NSLog(@"Netflix RSS auto-discovery failed: %@",error);
  [pageData_ release];
  [connection release];
  pageData_ = nil;
}

- (void)connection:(NSURLConnection *)connection didReceiveData:(NSData *)data
{
  [pageData_ appendData:data];  
}

- (void)connectionDidFinishLoading:(NSURLConnection*)connection
{
  [connection release];
  [self _autoDiscover];
}

@end
