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

#import <Cocoa/Cocoa.h>

#import <BackRow/BRBaseMediaAsset.h>
#import <BackRow/BRImageManager.h>
#import <BackRow/BRMediaAsset.h>
#import <BackRow/BRTextMenuItemLayer.h>
#import <BackRow/BRMediaPreviewControllerFactory.h>

#import "BaseUnderstudyAsset.h"
#import "UnderstudyAsset.h"

@class HuluFeed;
@class HuluFeedDiscoverer;

// class HuluAsset
//
// Represents the attributes of a Hulu Video

@interface HuluAsset : BaseUnderstudyAsset<UnderstudyAsset> 
{
 @private
  NSString* title_;
  NSString* series_;
  NSString* description_;
  NSString* credit_;
  NSURL* url_;
  long duration_;
  int episode_;
  int season_;
  NSString* episodeInfo_;
  NSDate* airdate_;
  NSDate* added_;
  NSString* thumbnailID_;
  BRImageManager* imageManager_;
  BRTextMenuItemLayer* menuitem_;

  // url's that aren't for a video are loaded in hopes of finding a feed url
  // in them (e.g. a show page contains it's "episodes" feed)
  HuluFeed* feed_;                     // the contained feed
  HuluFeedDiscoverer* feedDiscoverer_; // document that loads the original page
}

// Designated initializer. Builds the asset from the provided XML object
// model. Some of the attributes are generic like media:url, but much of it
// is specific to Hulu
- (id)initWithXMLElement:(NSXMLElement*)dom;

// Provides the primary url of the media.
- (NSURL*)url;

// The aspect ratio of the video (width / height).  The player defaults to 16/9 
// but may be overridden by user settings for shows that are still 4/3
- (float)aspectRatio;
@end
