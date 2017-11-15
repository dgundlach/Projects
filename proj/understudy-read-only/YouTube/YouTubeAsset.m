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

#import <BackRow/BRImageManager.h>
#import <BackRow/BRImageManager.h>
#import <BackRow/BRTextMenuItemLayer.h>

#import "FeedMenuController.h"
#import "YouTubeAsset.h"
#import "YouTubeFeed.h"
#import "YouTubeController.h"

@interface YouTubeAsset (Parsing)
- (void)buildFromId:(NSXMLElement*)idtag;
- (void)parseMediaGroup:(NSXMLElement*)media;
@end


@implementation YouTubeAsset

- (id)initWithXMLElement:(NSXMLElement*)dom
{
  imageManager_ = [BRImageManager sharedInstance];
  [super init];
  NSXMLElement* title = [[dom elementsForName:@"title"] objectAtIndex:0];
  title_ = [[title childAtIndex:0] stringValue];
  title_ = [title_ stringByReplacingOccurrencesOfString:@"Videos published by :"
                                             withString:@""];
  [title_ retain];
  
  NSXMLElement* content = [[dom elementsForName:@"content"] objectAtIndex:0];
  // some YouTube feeds contain other feeds, some contain videos. we make the
  // distinction here at the asset level
  NSXMLElement* category = [[dom elementsForName:@"category"] objectAtIndex:0];
  NSString* term = [[category attributeForName:@"term"] stringValue];
  if( [term hasSuffix:@"video"] ){
    isVideo_ = TRUE;
    // depending on the feed, the video |entry| will be structured differently
    // those with a media:group are easier to work with
    NSArray* mediagroups = [dom elementsForName:@"media:group"];
    if( [mediagroups count] ) 
      [self parseMediaGroup:[mediagroups objectAtIndex:0]];
    else
      [self buildFromId:[[dom elementsForName:@"id"] lastObject]];
  }else{
    isVideo_ = FALSE;
    NSString* src = [[content attributeForName:@"src"] stringValue];
    url_ = [[NSURL URLWithString:src] retain];
    // feeds may have a thumbnail
    NSXMLElement* thumbnail;
    thumbnail = [[dom elementsForName:@"media:thumbnail"] lastObject];
    if( thumbnail ){
      thumbnailID_ = [[thumbnail attributeForName:@"url"] stringValue];
      NSURL* thumburl = [NSURL URLWithString:thumbnailID_];
      thumbnailID_ = [[imageManager_ writeImageFromURL:thumburl] retain];
    }
  }
  return self;
}

// in feeds that don't use the media:group, we try to parse information from
// the content element of each feed entry. much less information is provided
- (void)buildFromId:(NSXMLElement*)idtag;
{
  if( !idtag ) return;
  NSURL* url;
  NSString *player,*thumbnail;
  NSString* mediaid = [idtag stringValue];
  NSRange range = [mediaid rangeOfString:@"video:"];
  if( range.location == NSNotFound ) return;
  mediaid = [mediaid substringFromIndex:NSMaxRange(range)];
  range = [mediaid rangeOfString:@":"];
  if( range.location != NSNotFound )
    mediaid = [mediaid substringToIndex:range.location];
  player = [@"http://www.youtube.com/watch?v=" stringByAppendingString:mediaid];
  url_ = [[NSURL URLWithString:player] retain];
  
  thumbnail = [@"http://i.ytimg.com/vi/" stringByAppendingString:mediaid];
  thumbnail = [thumbnail stringByAppendingString:@"/1.jpg"];
  url = [NSURL URLWithString:thumbnail];
  thumbnailID_ = [[imageManager_ writeImageFromURL:url] retain];
  
  videoID_ = [mediaid retain];
}

- (void)parseMediaGroup:(NSXMLElement*)media
{
  NSLog(@"parseMediaGroup");
  NSXMLElement* xml;
  NSXMLNode* attribute;
  xml = [[media elementsForName:@"media:title"] objectAtIndex:0];
  title_ = [[xml stringValue] retain];
  xml = [[media elementsForName:@"yt:description"] objectAtIndex:0];
  description_ = [[xml stringValue] retain];
  xml = [[media elementsForName:@"media:player"] objectAtIndex:0];
  url_ = [[xml stringValue] retain];
  // YT often has multiple thumbnails. we could provide them as a parade, or
  // make them available for 
  xml = [[media elementsForName:@"media:thumbnail"] objectAtIndex:0];
  attribute = [xml attributeForName:@"url"];
  NSURL* thumburl = [NSURL URLWithString:[attribute stringValue]];
  thumbnailID_ = [[imageManager_ writeImageFromURL:thumburl] retain];
  xml = [[media elementsForName:@"yt:duration"] objectAtIndex:0];
  duration_ = [[[xml attributeForName:@"seconds"] stringValue] intValue];
  xml = [[media elementsForName:@"gd:rating"] objectAtIndex:0];
  starrating_ = [[[xml attributeForName:@"average"] stringValue] floatValue];
  xml = [[media elementsForName:@"published"] objectAtIndex:0];
  return;
}

- (BRLayer<BRMenuItemLayer>*)menuItem
{
  if( !menuitem_ )
  {
    if( isVideo_ ) menuitem_ = [BRTextMenuItemLayer menuItem];
    else menuitem_ = [BRTextMenuItemLayer folderMenuItem];
    [menuitem_ setTitle:[self title]];
    [menuitem_ retain];
  }
  return menuitem_;
}

- (BRController*)controller
{
  if( isVideo_ )
    return [[YouTubeController alloc] initWithAsset:self];
  else{
    YouTubeFeed* del = [[YouTubeFeed alloc] initWithTitle:title_
                                                    forUrl:url_];
    return [del controller];
  }
}


- (NSString*)assetID{ return [url_ description]; }
- (NSString*)title{ return title_; }
- (NSString*)titleForSorting{ return [self title]; }
- (NSString*)mediaSummary{ return description_; }
- (NSString*)mediaDescription{ return description_; }
- (float)starRating{ return starrating_; }
- (long)duration{ return duration_; }
- (NSString*)mediaURL{ return [url_ description]; }
- (BOOL)hasCoverArt{ return YES; }
- (NSString*)coverArtID{ return thumbnailID_; }
- (NSString*)thumbnailArtID{ return thumbnailID_; }
- (BRImage*)coverArt{ return [self thumbnailArt]; }
- (BRImage*)thumbnailArt
{ 
  return [imageManager_ imageNamed:thumbnailID_];
}
- (BRMediaType*)mediaType{ return [BRMediaType ytVideo]; }
- (NSDate*)datePublished{ return published_; }
- (NSString*)datePublishedString{ return [published_ description]; }
- (NSString*)videoID{ return videoID_; }

@end
