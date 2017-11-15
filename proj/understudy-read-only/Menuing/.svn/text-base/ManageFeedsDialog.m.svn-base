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

#import "ManageFeedsDialog.h"
#import "MainMenuController.h"
#import "RenameDialog.h"

#import <BackRow/BRControllerStack.h>
#import <BackRow/BRTextMenuItemLayer.h>

@implementation ManageFeedsDialog

- (id)init
{
  [super init];
  [self setTitle:[self title]];
  [self addOptionText:@"Add"];
  [self addOptionText:@"Remove"];
  [self addOptionText:@"Rename"];
  [self addOptionText:@"Move"];
  [self setActionSelector:@selector(itemSelected) target:self];
  addController_ = [[AddFeedDialog alloc] init];
  return self;
}

- (void)controlWasActivated
{
  [super controlWasActivated];
  // if the main menu doesn't have any feeds, go right to the add dialog
  if( [[MainMenuController sharedInstance] itemCount] <= 1 )
    [[self stack] swapController:addController_];
}

- (void)_presentMoveDialog
{
  [moveDialog_ release];
  moveDialog_ = [[BROptionDialog alloc] init];
  [moveDialog_ setTitle:@"Move Feed"];
  MainMenuController* main = [MainMenuController sharedInstance];
  int i;
  for(i = 0; i<([main itemCount]-1); i++)
    [moveDialog_ addOptionText:[main titleForRow:i]];  
  [moveDialog_ setActionSelector:@selector(_moveFrom) target:self];
  [[self stack] pushController:moveDialog_];  
}

- (void)_moveFrom
{
  [moveDialog_ setPrimaryInfoText:@"Select new position." 
                   withAttributes:nil];
  [moveDialog_ setActionSelector:@selector(_moveTo) target:self];
  // misusing the |tag|, rather than using the user info
  [moveDialog_ setTag:[moveDialog_ selectedIndex]];
}

- (void)_moveTo
{
  long from = [moveDialog_ tag];
  long to = [moveDialog_ selectedIndex];
  [[MainMenuController sharedInstance] moveFeedFromIndex:from toIndex:to];
  [[self stack] popController];
}

- (void)_presentRemoveDialog
{
  [removeDialog_ release];
  removeDialog_ = [[BROptionDialog alloc] init];
  [removeDialog_ setTitle:@"Remove Feed"];
  
  MainMenuController* main = [MainMenuController sharedInstance];
  int i;
  for(i = 0; i<([main itemCount]-1); i++)
    [removeDialog_ addOptionText:[main titleForRow:i]];  
  [removeDialog_ setActionSelector:@selector(_remove) target:self];
  [[self stack] pushController:removeDialog_];
}

// call back for the remove dialog
- (void)_remove
{
  long index = [removeDialog_ selectedIndex];
  [[MainMenuController sharedInstance] removeFeedAtIndex:index];
  [[self stack] popController];
}

- (void)_presentRenameDialog
{
  RenameDialog* rename = [[RenameDialog alloc] init];
  [[self stack] pushController:rename];
  [rename autorelease];
}

// call-back for an item having been selected
- (void)itemSelected
{
  switch([self selectedIndex])
  {
    case 0: // add
      [[self stack] pushController:addController_];
      break;
    case 1: // remove
      [self _presentRemoveDialog];
      break;
    case 2: // rename
      [self _presentRenameDialog];
      break;
    case 3: // move
      [self _presentMoveDialog];
      break;
    default:
      NSLog(@"unexpected index in add dialog");
  }
}

- (BRLayer<BRMenuItemLayer>*)menuItem
{
  BRTextMenuItemLayer* manager = [BRTextMenuItemLayer menuItem];
  [manager setTitle:[self title]];
  return manager;
}

- (BRController*)controller{ return self; }

- (NSString*)title
{
  return @"Manage Feeds";
}

- (NSArray*)containedAssets
{
  return nil;
}

- (BRControl*)preview
{
  return nil;
}

@end
