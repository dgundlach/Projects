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

#import "NetflixAsset.h"
#import "NetflixController.h"

#import <BackRow/BRImage.h>

#import <CoreFoundation/CFXMLNode.h>

#include <regex.h>

@implementation NetflixAsset

#define WATCHURL @"http://www.netflix.com/WiPlayer?movieid=%@"
#define BOXSHOTS @"http://cdn-0.nflximg.com/us/boxshots/large/%@.jpg"

- (id)initWithXMLElement:(NSXMLElement*) dom
{
  NSString *mediaID, *url;
  NSURL* thumburl;

  imageManager_ = [BRImageManager sharedInstance];
  [super init];
  
  NSXMLElement* titleline = [[dom elementsForName:@"title"] objectAtIndex:0];
  NSString* titlestring = [[titleline childAtIndex:0] description];
  titlestring = (NSString*) CFXMLCreateStringByUnescapingEntities(NULL,(CFStringRef)titlestring,NULL);
  NSRange hy = [titlestring rangeOfString:@"- "];
  if( hy.location != NSNotFound )
    title_ = [[titlestring substringFromIndex:(hy.location+hy.length)] retain];
  else
    title_ = [titlestring copy];

  NSXMLElement* link = [[dom elementsForName:@"link"] objectAtIndex:0];
  mediaID = [[link stringValue] lastPathComponent];

  url = [NSString stringWithFormat:WATCHURL,mediaID];
  url_ = [[NSURL URLWithString:url] retain];

  thumburl = [NSURL URLWithString:[NSString stringWithFormat:BOXSHOTS,mediaID]];
  thumbnailID_ = [[imageManager_ writeImageFromURL:thumburl] retain];

  NSString* description = [[[dom elementsForName:@"description"] 
                            objectAtIndex:0] stringValue];
  NSRange br = [description rangeOfString:@"<br>"];
  if( br.location != NSNotFound )
    description = [description substringFromIndex: NSMaxRange(br)];
  description = [description stringByReplacingOccurrencesOfString:@"<br>"
                                                       withString:@"\n"];
  description_ = (NSString*) CFXMLCreateStringByUnescapingEntities(NULL,(CFStringRef)description,NULL);
  [description_ retain];
  
  return self;
}

- (void)dealloc
{
  [description_ release];
  [title_ release];
  [url_ release];
  [super dealloc];
}

- (NSURL*)url{ return url_; }

#pragma mark BRMediaPreviewFactoryDelegate
- (BRMediaType*)mediaPreviewMissingMediaType{ return nil; }
- (BOOL)mediaPreviewShouldShowMetadata{ return YES; }
- (BOOL)mediaPreviewShouldShowMetadataImmediately{ return YES; }


#pragma mark BRMediaAsset
- (NSString*)title{ return title_; }
- (NSString*)titleForSorting{ return [self title]; }
- (NSString*)mediaSummary{ return description_; }
- (NSString*)mediaDescription{ return [self mediaSummary]; }
- (NSString*)mediaURL{ return [url_ description]; }
- (NSString*)thumbnailArtID{ return thumbnailID_; }
- (BRImage*)coverArt{ return [self thumbnailArt]; }
- (BRImage*)thumbnailArt
{ 
  return [imageManager_ imageNamed:thumbnailID_];
}
- (BRImage*)coverArtForBookmarkTimeInMS:(unsigned)ms{ return [self coverArt]; }
- (BRMediaType*)mediaType{ return [BRMediaType ytVideo]; }
- (BOOL)hasVideoContent{ return YES; }
- (BOOL)isDisabled{ return NO; }
- (BOOL)isInappropriate{ return NO; }

- (NSString*)assetID{ return [url_ description]; }
- (BOOL)isProtectedContent{ return NO; }
- (BRResolution*)resolution{ return [BRResolution ED480p]; }
- (BOOL)canBePlayedInShuffle{ return NO; }
- (BOOL)isLocal{ return NO; }
- (BRImage*)coverArtNoDefault{ return [self coverArt]; }

#pragma mark BRImageProvider
- (NSString*)imageID{return nil;}
- (void)registerAsPendingImageProvider:(BRImageLoader*)loader
{ NSLog(@"registerAsPendingImageProvider"); }
- (void)loadImage:(BRImageLoader*)loader{ }

- (BRLayer<BRMenuItemLayer>*)menuItem
{
  if( !menuitem_ )
  {
    menuitem_ = [BRTextMenuItemLayer menuItem];
    [menuitem_ setTitle:[self title]];
    [menuitem_ retain];
  }
  return menuitem_;  
}

- (BRController*)controller
{
  return [[NetflixController alloc] initWithAsset:self];
}


@end
