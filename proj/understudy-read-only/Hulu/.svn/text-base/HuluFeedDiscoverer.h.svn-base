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

#import <Cocoa/Cocoa.h>

#import <BackRow/BRURLDocumentManager.h>

@interface HuluFeedDiscoverer : BRURLDocument {
 @private
  NSURL* url_;  // the provided url
  NSURL* feed_; // the discovered feed url
}

- (id)initWithUrl:(NSURL*)url;

// report the discovered feed
- (NSURL*)feed;

// report the URL after any redirections occur. This is important in cases where
// the initial URL doesn't appear to be for a specific video, but ultimately is.
- (NSURL*)finalURL;
@end
