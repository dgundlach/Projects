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

#import "RenameDialog.h"
#import "MainMenuController.h"

#import <BackRow/BRControllerStack.h>

@implementation RenameDialog

- (id)init
{
  [super init];
  [self setActionSelector:@selector(itemSelected) target:self];
  int i;
  MainMenuController* main = [MainMenuController sharedInstance];
  for( i = 0; i < [main itemCount]-1; i++ )
    [self addOptionText:[main titleForRow:i]];
  [self setTitle:@"Rename Feed"];
  [self setPrimaryInfoText:@"Select a feed to rename" withAttributes:nil];
  return self;
}

- (void)itemSelected
{
  BRTextEntryController* entry = [BRTextEntryController alloc];
  [entry initWithTextEntryStyle:kFullKeyboardTextEntryStyle];
  [entry setTextEntryCompleteDelegate:self];
  [entry setTitle:@"New Name"];
  [[self stack] pushController:entry];
}

- (void)textDidChange:(id<BRTextContainer>)container{ }

- (void)textDidEndEditing:(id<BRTextContainer>)container
{
  NSString* newname = [container stringValue];
  MainMenuController* main =  [MainMenuController sharedInstance];
  [main renameFeedAtIndex:[self selectedIndex] withTitle:newname];
  [[self stack] popToController:main];
}

@end
