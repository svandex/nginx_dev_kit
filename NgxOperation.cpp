#include "NgxOperation.hpp"


void NgxOperation::AddExceptionMessageToRet(std::exception &e)
{
        AddKeyValueToRet("exception", e.what());
}

ngx_str_t NgxOperation::NgxChainToNgxStr(ngx_chain_t* c,ngx_pool_t* p){
    ngx_str_t ccp=ngx_null_string;
    NgxChain chain(c);
    //NgxPool pool(p);

    //ccp.data=pool.nalloc<u_char>(chain.size());
    ccp.data=(u_char*)ngx_pcalloc(p,chain.size());

    size_t _pos=0;

    for(NgxChainIterator it=chain.begin();it!=chain.end();it++){
	ngx_str_t node=(*it).data().range();
	if(node.len==0){
	    break;
	}
	memcpy(ccp.data+_pos,node.data,node.len+1);
	_pos+=(*it).data().size();
    }

    if(_pos==chain.size()){
	ccp.len=_pos;
    }else{
	ccp.data=nullptr;
	ccp.len=0;
    }
    return ccp;
}

