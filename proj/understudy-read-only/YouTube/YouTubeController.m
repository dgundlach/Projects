//
//  Copyright 2009 Kirk Kelsey.
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

#import "YouTubeController.h"
#import "YouTubeAsset.h"

@implementation YouTubeController

- (id)initWithAsset:(YouTubeAsset*)asset
{
  [super init];
  asset_ = asset;
  return self;
}

#define EMBED_URL @"http://youtube.com/apiplayer?enablejsapi=1&fs=1"

- (void)load
{
  [NSApplication sharedApplication];
  NSRect rect = [[NSScreen mainScreen] frame];
  view_ = [[WebView alloc] initWithFrame:rect];
  [[[view_ mainFrame] frameView] setAllowsScrolling:NO];
  [view_ setFrameLoadDelegate:self];
  [view_ setResourceLoadDelegate:self];
  window_ = [[NSWindow alloc] initWithContentRect:rect 
                                        styleMask:0 
                                          backing:NSBackingStoreBuffered 
                                            defer:YES];
  [window_ setContentView:view_];
  NSURL* url = [NSURL URLWithString:EMBED_URL];
  NSURLRequest* request = [NSURLRequest requestWithURL:url];
  [[view_ mainFrame] loadRequest:request];
}

- (void)_enqueueVideo
{
  WebScriptObject* script = [[view_ windowScriptObject] retain];
  [script autorelease];
  id player;
  long tries = 0;
  NSString* getPlayer = @"document.getElementsByTagName('EMBED')[0]";
  do{
    player = [script evaluateWebScript:getPlayer];
    tries++;
  }while( player == [WebUndefined undefined]);
  NSLog(@"got player after %d tries",tries);
  player = [script evaluateWebScript:@"document.getElementsByTagName('EMBED')[0].loadVideoById"];
  if( player == [WebUndefined undefined] ) NSLog(@"loadVideoById undefined");
  else NSLog([player description]);
  NSString* loadVideo;
  loadVideo = [getPlayer stringByAppendingString:@".loadVideoById('%@',0)"];
  NSLog(loadVideo);
  loadVideo = [NSString stringWithFormat:loadVideo, [asset_ videoID]];
  [script evaluateWebScript:loadVideo];  
}

- (void)webView:(WebView *)sender 
       resource:(id)identifier 
didFinishLoadingFromDataSource:(WebDataSource *)dataSource
{
  NSLog(@"finished loading datasource: %@",dataSource);
}

// <WebFrameLoadDelegate> callback once the video loads
- (void)webView:(WebView*)view didFinishLoadForFrame:(WebFrame*)frame
{
  NSLog(@"did finish loading");
  if( frame != [view mainFrame] ) return;
  [window_ display];
  [window_ orderFrontRegardless];
  [window_ setLevel:CGShieldingWindowLevel()];
  [self _enqueueVideo];
  [self reveal];
}

- (void)webView:(WebView *)sender didFailLoadWithError:(NSError *)error 
       forFrame:(WebFrame *)frame
{
  NSLog(@"failed loading frame");
}

- (void)controlWillActivate
{
  [self load];
}

- (void)controlWillDeactivate
{
  [self returnToFR];
  [view_ close];
  view_ = nil;
  [window_ close];
  window_ = nil;
}

@end
