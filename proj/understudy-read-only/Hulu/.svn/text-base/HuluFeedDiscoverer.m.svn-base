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

#import "HuluFeedDiscoverer.h"


@implementation HuluFeedDiscoverer

- (id)initWithUrl:(NSURL*)url
{
  url_ = [url copy];
  NSURLRequest* request = [NSURLRequest requestWithURL:url_];
  [super initWithURLRequest:request];
  [[BRURLDocumentManager textDocumentManager] loadDocument:self];
  return self;
}

- (void)dealloc
{
  [url_ release];
  [feed_ release];
  [super dealloc];
}

// once the document has loaded, find the feed URL
- (void)documentLoaded
{
  NSStringEncoding encoding = NSISOLatin1StringEncoding;
  NSString *contents, *url;
  NSRange start, end, searchRange, urlRange;
  
  contents = [[NSString alloc] initWithData:[self content]
                                   encoding:encoding];
  // we're banking on the "episodes" feed preceeding the "clips"
  start = [contents rangeOfString:@"http://www.hulu.com/feed/show/"];
  searchRange = NSMakeRange(NSMaxRange(start), 
                            [contents length]-NSMaxRange(start));
  end = [contents rangeOfString:@"\"" options:0 range:searchRange]; 
  if( start.location != NSNotFound && end.location != NSNotFound){
    urlRange = NSMakeRange(start.location, end.location-start.location);
    url = [contents substringWithRange:urlRange];
    feed_ = [[NSURL URLWithString:url] retain];
  }
}

- (NSURL*)feed
{
  return feed_;
}

- (NSURL*)finalURL
{
  return [[self URLResponse] URL];
}

@end
