/** @file

    A brief file description

    @section license License

    Licensed to the Apache Software Foundation (ASF) under one
    or more contributor license agreements.  See the NOTICE file
    distributed with this work for additional information
    regarding copyright ownership.  The ASF licenses this file
    to you under the Apache License, Version 2.0 (the
    "License"); you may not use this file except in compliance
    with the License.  You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/
#ifndef _URL_MAPPING_PATH_INDEX_H

#define _URL_MAPPING_PATH_INDEX_H

#include <list>
#include <map>

#include "URL.h"
#include "UrlMapping.h"
#include "Trie.h"

class UrlMappingPathIndex
{
public:
  UrlMappingPathIndex() { };

  bool Insert(url_mapping *mapping);

  url_mapping *Search(URL *request_url, int request_port, bool normal_search = true) const;

  typedef std::list<url_mapping *> MappingList;
  
  void GetMappings(MappingList &mapping_list) const;

  void Clear();

  virtual ~UrlMappingPathIndex() 
  {
    Clear();
  }


private:
  typedef Trie<url_mapping *> UrlMappingTrie;
  struct UrlMappingTrieKey 
  {
    int scheme_wks_idx;
    int port;
    UrlMappingTrieKey(int idx, int p) : scheme_wks_idx(idx), port(p) { };
    bool operator <(const UrlMappingTrieKey &rhs) const 
    {
      if (scheme_wks_idx == rhs.scheme_wks_idx) {
        return (port < rhs.port);
      }
      return (scheme_wks_idx < rhs.scheme_wks_idx);
    };
  };
  
  typedef std::map<UrlMappingTrieKey, UrlMappingTrie *> UrlMappingGroup;
  UrlMappingGroup m_tries;
    
  // make copy-constructor and assignment operator private
  // till we properly implement them
  UrlMappingPathIndex(const UrlMappingPathIndex &rhs) { };
  UrlMappingPathIndex &operator =(const UrlMappingPathIndex &rhs) { return *this; }

  inline UrlMappingTrie *_GetTrie(URL *url, int &idx, int port, bool search = true) const
  {
    idx = url->scheme_get_wksidx();
    UrlMappingGroup::const_iterator group_iter;
    if (search) { // normal search
      group_iter = m_tries.find(UrlMappingTrieKey(idx, port));
    } else { // return the first trie arbitrarily
      Debug("UrlMappingPathIndex::_GetTrie", "Not performing search; will return first available trie");
      group_iter = m_tries.begin();
    }
    if (group_iter != m_tries.end()) {
      return group_iter->second;
    }
    return 0;
  };
  
};

#endif // _URL_MAPPING_PATH_INDEX_H
