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

#include <regex.h>

#import "NetflixController.h"

#import <BackRow/BRAlertController.h>
#import <BackRow/BRControllerStack.h>
#import <BackRow/BRDisplayManager.h>
#import <BackRow/BREvent.h>
#import <BackRow/BRRenderScene.h>
#import <BackRow/BRSentinel.h>
#import <BackRow/BRSettingsFacade.h>

#import <Carbon/Carbon.h>

#define AGENTSTRING @"Mozilla/5.0 (Macintosh; U; Intel Mac OS X 10_5_6; en-us)"\
" AppleWebKit/525.27.1 (KHTML, like Gecko) Version/3.2.1 Safari/525.27.1"

@protocol  BRAppManagerDelegate
- (void)_continueDestroyScene:id;
@end


@interface NetflixController (private)
- (void)_loadVideo;
@end

@implementation NetflixController

- (id)initWithAsset:(NetflixAsset*)asset
{
  [super init];
  asset_ = [asset retain];
  return self;
}

- (void)dealloc
{
  [asset_ release];
  [super dealloc];
}

- (void)_loadVideo
{
  // invoking shared application ensures that windows can be ordered
  [NSApplication sharedApplication];
  NSRect rect = [[NSScreen mainScreen] frame];
  mainView_ = [[[WebView alloc] initWithFrame:rect] retain];
  [[[mainView_ mainFrame] frameView] setAllowsScrolling:NO];
  [mainView_ setFrameLoadDelegate:self];
  window_ = [[[NSWindow alloc] initWithContentRect:rect 
                                         styleMask:0 
                                           backing:NSBackingStoreBuffered 
                                             defer:YES] retain];
  [window_ setContentView:mainView_];
  NSURLRequest* pageRequest = [NSURLRequest requestWithURL:[asset_ url]];
  [mainView_ setCustomUserAgent:AGENTSTRING];
  [[mainView_ mainFrame] loadRequest:pageRequest];
}

// recenters the player in the window
// calling this breaks the play pause functinoality
/*- (void)_maximizePlayer_broken
{
  NSPoint newOrigin;
  NSSize screen,oldSize,newSize;

  oldSize = [pluginView_ frame].size;
  screen = [mainView_ frame].size;
  newSize = screen;
  if( oldSize.height / screen.height > oldSize.width  / screen.width )
    newSize.width = (oldSize.width / oldSize.height) * newSize.height;
  else
    newSize.height = (oldSize.height / oldSize.width) * newSize.width;    
  
  newOrigin.x = (screen.width - newSize.width)/2;
  newOrigin.y = (screen.height - newSize.height)/2;
  
  WebScriptObject* script = [mainView_ windowScriptObject];  
  NSString* position = @"document.getElementById('SLPlayer').setAttribute('style','width: %4.0fpx; height: %4.0fpx; position:absolute; top:%4.0fpx; left:%4.0fpx;')";
  position = [NSString stringWithFormat:position,
              ceilf(newSize.width),
              ceilf(newSize.height),
              ceilf(newOrigin.y),
              ceilf(newOrigin.x)+20,
              nil];
  [script evaluateWebScript:position];
  [(NSView*)pluginView_ display];
}*/

// <WebFrameLoadDelegate> callback once the video loads
- (void)webView:(WebView*)view didFinishLoadForFrame:(WebFrame*)frame
{
  if( frame != [view mainFrame] ) return;
  [window_ display];
  // if there is a plugin, we want to fullscreen it. if not (e.g. if the user
  // isn't logged in and the movie won't be shown) we report an error
  if( [self hasPluginView] ) {
    [self makeMainViewFullscreen];
    [self reveal];
  } else {
    NSString* title = @"Error";
    NSString* primary = @"Video Could Not Be Loaded";
    NSString* secondary = @"Please ensure that you are logged into your Netfli"\
    "x account in Safari, and that you have not reached your viewing limit.";
    BRAlertController* alert = [BRAlertController alertOfType:kBRAlertTypeError
                                                       titled:title
                                                  primaryText:primary
                                                secondaryText:secondary];
    [[self stack] swapController:alert];
  }
}

- (void)playPause
{
  [self sendPluginKeyCode:49 withCharCode:0]; // space-bar
}

#pragma mark BR Control

- (void)controlWillActivate
{
  [super controlWillActivate];
  [self _loadVideo];
}

- (void)controlWillDeactivate
{
  [super controlWillDeactivate];
  [self returnToFR];
  [window_ close];
  window_ = nil;
}

@end
