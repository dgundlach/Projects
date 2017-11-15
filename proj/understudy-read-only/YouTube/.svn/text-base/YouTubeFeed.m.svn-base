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

#import "YouTubeFeed.h"
#import "YouTubeAsset.h"

#import <BackRow/BRTextMenuItemLayer.h>

// YT feeds for a user (all should include ?client=ytapi-youtube-user&v=2)
// gdata.youtube.com/feeds/base/users/<username>
// contains references to other feeds to which the user is subscribed

// gdata.youtube.com/feeds/api/users/username/uploads
// gdata.youtube.com/feeds/base/users/<username>/playlists
//contains references to playlists the user created

//  when adding, should auto discover the username, and make both the 
//  subscriptions and playlists.

@implementation YouTubeFeed

- (id)initWithTitle:(NSString*)title forUrl:(NSURL*)url
{
  url_ = [url retain];
  title_ = [title copy];
  return self;
}

- (void)dealloc
{
  [url_ release];
  [title_ release];
  [super dealloc];
}

- (BRLayer<BRMenuItemLayer>*)menuItem
{
  BRTextMenuItemLayer*item = [BRTextMenuItemLayer menuItem];
  [item setTitle:title_];
  return item;
}

- (BRController*)controller
{
  if( !controller_ )
  {
    controller_ = [FeedMenuController alloc];
    controller_ = [controller_ initWithDelegate:self];
  }
  return controller_;
}

- (NSArray*)currentAssets
{
  NSError* err;
  NSXMLDocument* doc = [[NSXMLDocument alloc] initWithContentsOfURL:url_
                                                            options:0
                                                              error:&err];
  if( !doc ) return nil;
  NSMutableArray* assets = [NSMutableArray array];
  NSXMLElement* feed = [doc rootElement];
  NSArray* entries = [feed elementsForName:@"entry"];
  for( NSXMLElement* item in entries )
    [assets addObject:[[YouTubeAsset alloc] initWithXMLElement:item]];
  return assets;
}

- (NSString*)title
{
  return title_;
}

@end
