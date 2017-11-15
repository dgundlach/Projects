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

#import "BaseController.h"

#import <BackRow/BRControllerStack.h>
#import <BackRow/BRDisplayManager.h>
#import <BackRow/BREvent.h>
#import <BackRow/BRRenderScene.h>
#import <BackRow/BRSentinel.h>
#import <BackRow/BRSettingsFacade.h>

#import <Carbon/Carbon.h>

@interface BRAppManager : NSObject { }
+ (BRAppManager*)sharedApplication;
- (id)delegate;
@end

@interface BRRenderer{}
- (void)orderIn;
- (void)orderOut;
@end

@implementation BaseController

- (void)reveal
{
  BRSentinel* sentinel = [BRSentinel sharedInstance];
  id<BRRendererProvider> provider = [sentinel rendererProvider];
  BRRenderer* renderer = [provider renderer];
  [renderer orderOut];  
}

- (void)returnToFR
{
  BRSentinel* sentinel = [BRSentinel sharedInstance];
  id<BRRendererProvider> provider = [sentinel rendererProvider];
  BRRenderer* renderer = [provider renderer];
  [renderer orderIn];
  if( [mainView_ isInFullScreenMode] )
    [mainView_ exitFullScreenModeWithOptions:nil];
  if( [pluginView_ isInFullScreenMode] )
    [pluginView_ exitFullScreenModeWithOptions:nil];
  [mainView_ close];
}

- (BOOL)hasPluginView
{
  if( pluginView_ ) return YES;
  if( !mainView_ ) return NO;
  
  NSMutableSet* views = [[[NSMutableSet set] retain] autorelease];
  NSMutableSet* webviews = [[[NSMutableSet set] retain] autorelease];
  [views addObjectsFromArray:[mainView_ subviews]];
  while( [views count] ){
    WebView* view = [views anyObject];
    if( [[view className] isEqual:@"WebNetscapePluginDocumentView"] )
        [webviews addObject:view];
    [views addObjectsFromArray:[view subviews]];
    [views removeObject:view];
  }
  if( [webviews count] < 1 ){
    NSLog(@"got no plugin views");
    return NO;
  }else{
    if( [webviews count] > 1 ) NSLog(@"got multiple plugin views");
    pluginView_ = [[webviews anyObject] retain];
    return YES;
  }
}

- (void)_makeViewFullscreen:(WebView*)view
{
  BRDisplayManager* manager = [BRDisplayManager sharedInstance];
  NSDictionary* mode = [manager displayMode];
  NSArray* objects = [NSArray arrayWithObjects: NSFullScreenModeAllScreens,
                      NSFullScreenModeWindowLevel,
                      NSFullScreenModeSetting,
                      nil ];
  NSArray* keys = [NSArray arrayWithObjects: [NSNumber numberWithBool:YES],
                   [NSNumber numberWithInt:14],
                   mode,
                   nil ];
  NSDictionary* options = [NSDictionary dictionaryWithObjects:objects
                                                      forKeys:keys];
  [view enterFullScreenMode:[NSScreen mainScreen]
                withOptions:options];
}

- (void)makeMainViewFullscreen
{
  [self _makeViewFullscreen:mainView_];
}

- (void)makePluginFullscreen
{
  if( ![self hasPluginView] ) return;
  [self _makeViewFullscreen:pluginView_];
}

- (BOOL)isNetworkDependent
{
  return YES;
}

// play/pause works as expected, anythings else will fullscren the flash player
- (BOOL)brEventAction:(BREvent*)event
{
  BRSettingsFacade* settings;
  switch ([event remoteAction]) {
    case kBRRemotePlayPauseSelectButton:
      [self playPause];
      return YES;
      break;
    case kBRRemoteUpButton:
      settings = [BRSettingsFacade sharedInstance];
      [settings setSystemVolume:([settings systemVolume]+0.05)];
      return YES;
    case kBRRemoteDownButton:
      settings = [BRSettingsFacade sharedInstance];
      [settings setSystemVolume:([settings systemVolume]-0.05)];
      return YES;
    default:
      return [super brEventAction:event];
  }
}

- (void)sendPluginKeyCode:(int)keyCode withCharCode:(int)charCode
{
  if( ![self hasPluginView] )return;
  EventRecord event; 
  event.what = keyDown; 
  event.message = (keyCode << 8) + charCode;
  event.modifiers = 0;
  [(id)pluginView_ sendEvent:(NSEvent *)&event];
  event.what = keyUp;
  [(id)pluginView_ sendEvent:(NSEvent *)&event];
}

- (void)playPause{}

@end
