#include "manager.h"

using namespace node;
using namespace v8;

namespace webcl {

void Manager::add(Persistent<Object>* p, cl_type id) {
	if(objects.count(p)<1) {
#ifdef LOGGING
    WebCLObject *obj=ObjectWrap::Unwrap<WebCLObject>(*p);
	  printf("Adding %s cl %p, handle %p\n",obj->getCLObjName(),id,p);
#endif
	  objects[p]=id;
	  references[p]=0;
	}
	references[p]++;
}

void Manager::remove(Persistent<Object>* p) {
	if(references.count(p)<1)
		return;
	if(--references[p]<=0) {
#ifdef LOGGING
	  printf("Erasing %p\n",p);
#endif
		objects.erase(p);
		references.erase(p);
	}
}

Persistent<Object>* Manager::find(cl_type id) {
#ifdef LOGGING
	printf("find %p\n",id);
#endif
	for(auto it=objects.begin();it!=objects.end();++it) {
		if(it->second == id) {
			return it->first;
		}
	}
	return nullptr;
}

void Manager::clear() {
	for(auto it=objects.begin();it!=objects.end();++it) {
		auto p= *(it->first);
    if(!p.IsEmpty() && !p.IsNearDeath()) {
      WebCLObject *obj=ObjectWrap::Unwrap<WebCLObject>(p);
		  obj->Destructor();
    }
	}
	references.clear();
	objects.clear();
}

void Manager::stats() {
	printf("WebCL Manager stats:\n");
	printf("  #objects: %ld\n",objects.size());
	for(auto it=references.begin(); it!=references.end(); ++it) {
		auto p= *(it->first);
    if(!p.IsEmpty() && !p.IsNearDeath()) {
      WebCLObject *obj=ObjectWrap::Unwrap<WebCLObject>(p);
		  int count=it->second;
		  printf("    %s: %d\n",obj->getCLObjName(),count);
    }
	}
}

} // namespace webcl