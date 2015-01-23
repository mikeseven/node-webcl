#ifndef __WEBCLMANAGER_H__
#define __WEBCLMANAGER_H__

#include "platform.h"
#include "device.h"
#include <nan.h>
#include <map>
#include <set>

using namespace v8;

namespace webcl {

typedef void* cl_type;

class Manager {
public:
  static Manager* instance() {
    static Manager mgr;
    return &mgr;
  }

  void add(Persistent<Object>* p, cl_type id);  
  void remove(Persistent<Object>* p);
  Persistent<Object>* find(cl_type id);
  void clear();
  void stats();

private:
  explicit Manager() {}
  ~Manager() {
#ifdef LOGGING
  	cout<<"~Manager"<<endl;
#endif
  	clear();
  }

private:
  map<Persistent<Object>*, cl_type> objects;
  map<Persistent<Object>*, int> references;
};

} // namespace webcl

#endif // __WEBCLMANAGER_H__
