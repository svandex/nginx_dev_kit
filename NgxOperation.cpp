#include "NgxOperation.hpp"

void NgxOperation::AddStringKeyValueToRet(const char* key,const char * value){
    rapidjson::Document::AllocatorType &_dallocator = _default_document.GetAllocator();
    _default_document.AddMember(rapidjson::Value(key, _dallocator), rapidjson::Value(value, _dallocator), _dallocator);
}

void NgxOperation::AddExceptionMessageToRet(std::exception &e)
{
        AddStringKeyValueToRet("exception", e.what());
}


void NgxOperation::ReadAllDataToBuf(ngx_http_request_t*r){
    NgxChain c(r->request_body->bufs);
    void* cbufs=NgxPool(r->pool).nalloc<void>(c.size());
    size_t stored_pos=0;
    for(NgxChainIterator it=c.begin();it!=c.end();it++){
	ngx_str_t node=(*it).data().range();
	//memcpy(cbufs+stored_pos,node.data,node.len);
	stored_pos+=(*it).data().size();
    }

}
