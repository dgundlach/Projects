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
#import "HuluAsset.h"
#import "HuluController.h"


#import <BackRow/BRControllerStack.h>
#import <BackRow/BRComboMenuItemLayer.h>
#import <BackRow/BRTextMenuItemLayer.h>

#import <Foundation/NSXMLDocument.h>

@implementation HuluFeed

- (id)initWithTitle:(NSString*)title forUrl:(NSURL*)url
{
  [super init];
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

- (NSString*)title
{
  return title_;
}

- (NSArray*)currentAssets
{
  NSMutableArray* assets = [[[NSMutableArray alloc] init] autorelease];
  NSError* err;
  NSXMLDocument* doc = [NSXMLDocument alloc];
  doc = [[doc initWithContentsOfURL:url_ 
                           options:0
                             error:&err] autorelease];
  if( !doc ) return nil;
  NSXMLElement* root = [doc rootElement];
  NSXMLElement* channel = [[root elementsForName:@"channel"] objectAtIndex:0];
  NSArray* feeditems = [channel elementsForName:@"item"];
  for( NSXMLElement* feeditem in feeditems )
  {
    HuluAsset* asset = [[HuluAsset alloc] initWithXMLElement:feeditem];
    [asset autorelease];
    [assets insertObject:asset atIndex:0];
  }
  [assets_ autorelease];
  assets_ = [assets retain];
  return assets;
}

- (BRLayer<BRMenuItemLayer>*)menuItem
{
  BRTextMenuItemLayer*item = [BRTextMenuItemLayer menuItem];
  [item setTitle:title_];
  return item;
}

- (BRController*)controller
{
  FeedMenuController* con = [FeedMenuController alloc];
  return [con initWithDelegate:self];
}

- (BRControl*)preview
{
  if( assets_ )
    return [BRMediaPreviewControllerFactory previewControlForAssets:assets_
                                                       withDelegate:self];
  return nil;
}

@end
