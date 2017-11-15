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
#import <BackRow/BRMediaMenuController.h>
#import <UnderstudyAsset.h>

@class BRTextMenuItemLayer;

@protocol FeedDelegate <UnderstudyAsset>
// return an autoreleased array of UnderStudyAsset objects. Feed delegates 
// should not load their assests until specifically asked.
- (NSArray*) currentAssets;
// feeds that are managed by the FR menu system should be named there
// (as opposed to determining the name from web content or provider)
- (id)initWithTitle:(NSString*)title forUrl:(NSURL*)url;
@end

@interface FeedMenuController : BRMediaMenuController<BRMenuListItemProvider>
{
 @private
  NSArray* assets_;
  NSObject<FeedDelegate>* delegate_;  // delegate is retained by controller
  NSDate* lastrebuild_;
}

- (id)initWithDelegate:(NSObject<FeedDelegate>*)delegate;

@end
