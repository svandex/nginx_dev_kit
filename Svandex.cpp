#include "Svandex.h"

std::string Svandex::tools::GetCurrentPath()
{
    TCHAR dest[MAX_PATH];
    if (!dest)
        return std::string();

    HMODULE hModule = GetModuleHandle(NULL);
    if (hModule != NULL)
    {
        DWORD length = GetModuleFileName(hModule, dest, MAX_PATH);
    }
    else
    {
        return std::string();
    }

#if (NTDDI_VERSION >= NTDDI_WIN8)
    PathCchRemoveFileSpec(dest, MAX_PATH);
#else
    PathRemoveFileSpec(dest);
#endif
    size_t len = wcslen(dest) + 1;
    size_t converted = 0;
    char *destStr = (char *)malloc(len * sizeof(char));
    wcstombs_s(&converted, destStr, len, dest, _TRUNCATE);
    auto returnStr=std::string(destStr);
    free(destStr);
    return returnStr;
}
std::string Svandex::tools::GetCurrentTimeFT() {
	auto t = std::time(nullptr);
	char strrep[100];
	std::tm tms;
	localtime_s(&tms, &t);

	//get char* in specified time format
	std::strftime(strrep, sizeof(strrep), "%F %T", &tms);
	std::string str(strrep);
	str.shrink_to_fit();
	return str;
}

std::vector<std::string> Svandex::tools::GetEnvVariable(const char* pEnvName)
{
	#if defined(_WIN64)
	std::vector<std::string> vbuf;
	char* buf;
	size_t buf_num;
	errno_t err = _dupenv_s(&buf, &buf_num, pEnvName);
	if (err) {
		return vbuf;
	}
	if (buf_num > 0) {
		std::stringstream ss(buf);
		std::string sn;
		while (std::getline(ss, sn, ';')) {
			vbuf.push_back(sn);
		}
	}

	free(buf);
	return vbuf;
	#endif
}

std::string Svandex::tools::GetUUID() {
	UUID uuid;
	std::string ret;
	RPC_STATUS ret_val = UuidCreate(&uuid);
	if (ret_val == RPC_S_OK) {
		CHAR* wszuuid = NULL;
		UuidToStringA(&uuid, (RPC_CSTR*)&wszuuid);
		if (wszuuid != NULL) {
			ret = wszuuid;
			RpcStringFreeA((RPC_CSTR*)&wszuuid);
			return ret;
		}
	}
	return "";
}

Svandex::WebSocket& Svandex::WebSocket::operator=(Svandex::WebSocket&& ws) {
	m_HttpContext = ws.m_HttpContext;
	m_HttpServer = ws.m_HttpServer;
	m_opreation_functor = ws.m_opreation_functor;
	m_buf_size = ws.m_buf_size;

	//nullptr
	ws.m_HttpContext = nullptr;
	ws.m_HttpServer = nullptr;
	ws.m_opreation_functor = nullptr;

	//char vector initialization
	m_read_once.reserve(m_buf_size);
	m_writ_once.reserve(m_buf_size);
	m_buf.reserve(m_buf_size);
	return *this;
}

Svandex::WebSocket::WebSocket(IHttpServer *is, IHttpContext *ic, Svandex::WebSocket::WebSocketFunctor wsf, DWORD bufsize)
	:m_HttpServer(is), m_HttpContext(ic), m_opreation_functor(wsf),m_buf_size(bufsize){
	/*
	IWebSocketContext
	*/
	IHttpResponse *pHttpResponse = ic->GetResponse();
	pHttpResponse->ClearHeaders();
	pHttpResponse->SetStatus(101, "Switching Protocols");
	/*
	IWebSocketContext Ready
	*/
	HRESULT	hr = pHttpResponse->Flush(TRUE, TRUE, NULL);
	if (FAILED(hr)) {
		wsf = nullptr;
	}
	else {
		reset_arguments();
	}

	//buf initialization
	m_buf.reserve(m_buf_size);
}


/*
Main Thread
Controlling read cycle, need syncronization from readAsycn and writAsyc thread
*/
HRESULT Svandex::WebSocket::StateMachine() {
	IHttpContext3* pHttpContext3;
	HRESULT hr = HttpGetExtendedInterface(m_HttpServer, m_HttpContext, &pHttpContext3);
	IWebSocketContext* pWebSocketContext = (IWebSocketContext*)pHttpContext3->GetNamedContextContainer()->GetNamedContext(IIS_WEBSOCKET);

	//Start State Machine
	DWORD read_bytes = m_buf_size;
	while (TRUE) {
		read_bytes = m_buf_size;
		//m_buf.resize(m_buf_size);
		HRESULT hr = pWebSocketContext->ReadFragment(m_buf.data(), &read_bytes, TRUE, &m_fisutf8, &m_ffinalfragment, &m_fconnectionclose, Svandex::functor::ReadAsyncCompletion, this, &m_fCompletionExpected);
		std::unique_lock<std::mutex> lk(m_pub_mutex);
		m_cv.wait(lk, [this] {
			return m_sm_cont == TRUE;
			});
		m_sm_cont = FALSE;
		if (m_sm_close) {
			break;
		}
	}
	return hr;
}

/*
Read Async Thread
Will be closed after return
*/
void WINAPI Svandex::functor::ReadAsyncCompletion(HRESULT hr, PVOID completionContext, DWORD cbio, BOOL fUTF8Encoded, BOOL fFinalFragment, BOOL fClose) {
	Svandex::WebSocket* pws = (Svandex::WebSocket*)completionContext;
	/*
	IWebSocketCOntext Pointer
	*/
	IHttpContext3* pHttpContext3;
	HttpGetExtendedInterface(pws->m_HttpServer, pws->m_HttpContext, &pHttpContext3);
	IWebSocketContext* pWebSocketContext = (IWebSocketContext*)pHttpContext3->GetNamedContextContainer()->GetNamedContext(IIS_WEBSOCKET);
	if (FAILED(hr)) {
		pws->m_sm_cont = TRUE;
		pws->m_sm_close = TRUE;
		pws->m_cv.notify_all();
		return;
	}
	else {
		if (fClose) {
			pws->m_sm_cont = TRUE;
			pws->m_sm_close = TRUE;
			pws->m_cv.notify_all();
			return;
		}
		HRESULT hrac;
		if (cbio > 0) {
			pws->m_read_once.reserve(pws->m_read_once.size() + cbio);
			//merge m_buf to m_read_once
			pws->m_read_once.insert(pws->m_read_once.end(), pws->m_buf.begin(), pws->m_buf.begin() + cbio);
		}
		while (!fFinalFragment) {
			BOOL tCompletionExpected = FALSE;

			//read after write
			/*
			DWORD ZEROn = 0;
			hrac = pWebSocketContext->WriteFragment(NULL, &ZEROn, TRUE, TRUE, TRUE, Svandex::functor::fNULL, NULL);
			*/

			pWebSocketContext->CancelOutstandingIO();

			//read again
			cbio = 10;
			pws->m_buf.resize(pws->m_buf_size);
			hrac = pWebSocketContext->ReadFragment(pws->m_buf.data(), &cbio, TRUE, &fUTF8Encoded, &fFinalFragment, &fClose, Svandex::functor::fWebSocketNULL, NULL, &tCompletionExpected);

			//has read
			if (cbio > 0) {
				pws->m_read_once.reserve(pws->m_read_once.size() + cbio);
				//merge m_buf to m_read_once
				pws->m_read_once.insert(pws->m_read_once.end(), pws->m_buf.begin(), pws->m_buf.begin() + cbio);
			}
			if (FAILED(hrac)) {
				pws->m_sm_cont = TRUE;
				pws->m_sm_close = TRUE;
				pws->m_cv.notify_all();
				return;
			}
		}//while

		pws->m_writ_once.clear();
		hrac = pws->m_opreation_functor(pws->m_read_once, pws->m_writ_once);
		DWORD writ_bytes = (DWORD)pws->m_writ_once.size();
		hrac = pWebSocketContext->WriteFragment(pws->m_writ_once.data(), &writ_bytes, TRUE, TRUE, TRUE, Svandex::functor::WritAsyncCompletion, pws);
	}
}

void WINAPI Svandex::functor::WritAsyncCompletion(HRESULT hr, PVOID completionContext, DWORD cbio, BOOL fUTF8Encoded, BOOL fFinalFragment, BOOL fClose) {
	Svandex::WebSocket* pws = (Svandex::WebSocket*)completionContext;
	std::lock_guard<std::mutex> lk(pws->m_pub_mutex);
	if (FAILED(hr)) {
		pws->m_sm_cont = TRUE;
		pws->m_sm_close = TRUE;
		pws->m_cv.notify_all();
		return;
	}
	pws->m_read_once.clear();
	pws->m_sm_cont = TRUE;
	pws->m_cv.notify_all();
}

void WINAPI Svandex::functor::fWebSocketNULL(HRESULT hr, PVOID completionContext, DWORD cbio, BOOL fUTF8Encoded, BOOL fFinalFragment, BOOL fClose) {
}

std::string Svandex::json::message(std::string _Mess, const char* _Type, const char* _Extra) {
	if (std::strcmp(_Extra, "") == 0) {
		return (std::string("{\"") + _Type + "\":" + _Mess + "}");
	}
	else {
		return (std::string("{\"") + _Type + "\":" + _Mess + ",\"description\":\"" + _Extra + "\"}");
	}
}
