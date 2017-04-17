#include "platform_detection.h"
#if LW_PLATFORM_WIN32
#include <windows.h>
#include <stdlib.h>  
#include <stdio.h>  
#include <conio.h>  
#include <process.h> 
#include <dshow.h>

struct Sound
{
	bool valid;
	wchar_t filename[MAX_PATH];
	HANDLE play_event;
};

#define MAX_SOUND (8)

static Sound sound_pool[MAX_SOUND];
#else
#define HRESULT int
#endif



#include "constants.h"
#if LW_PLATFORM_WIN32
static void play_sound_thread_proc(void* pContext)
{

	auto pSound = reinterpret_cast<Sound*>(pContext);

	while (true)
	{
		WaitForSingleObject(pSound->play_event, INFINITE);

		IGraphBuilder *pGraph = nullptr;
		IMediaPosition *pMediaPosition = nullptr;
		IMediaControl *pControl = nullptr;
		IMediaEventEx *pEvent = nullptr;

		auto hr = S_OK;

		hr = ::CoCreateInstance(CLSID_FilterGraph, nullptr, CLSCTX_INPROC_SERVER, IID_IGraphBuilder, reinterpret_cast<void **>(&pGraph));

		hr = pGraph->QueryInterface(IID_IMediaControl, reinterpret_cast<void **>(&pControl));
		hr = pGraph->QueryInterface(IID_IMediaEvent, reinterpret_cast<void **>(&pEvent));
		hr = pGraph->QueryInterface(IID_IMediaPosition, reinterpret_cast<void**>(&pMediaPosition));

		// Build the graph.
		hr = pGraph->RenderFile(pSound->filename, nullptr);

		if (SUCCEEDED(hr)) {
			// Run the graph.
			hr = pControl->Run();

			if (SUCCEEDED(hr)) {
				// Wait for completion.
				long evCode;
				pEvent->WaitForCompletion(INFINITE, &evCode);
			}
		}

		/*
		long evCode;
		pMediaPosition->put_CurrentPosition(0);
		pControl->Run();
		pEvent->WaitForCompletion(INFINITE, &evCode);
		*/

		// Clean up in reverse order.
		pMediaPosition->Release();
		pEvent->Release();
		pControl->Release();
		pGraph->Release();

		pGraph = nullptr;

		ResetEvent(pSound->play_event);

		pSound->valid = false;

		//::CoUninitialize();
	}
	
	_endthread();

}
#endif

extern "C" HRESULT init_ext_sound_lib()
{
#if LW_PLATFORM_WIN32
	/*
	for (auto i = 0; i < _countof(sound_pool); i++)
	{
		sound_pool[i].play_event = CreateEvent(nullptr, TRUE, FALSE, nullptr);

		_beginthread(play_sound_thread_proc, 0, &sound_pool[i]);
	}
	*/

	return S_OK;
#else
    return 0;
#endif
}

extern "C" void destroy_ext_sound_lib()
{
}

extern "C" void play_sound(enum LW_SOUND lws)
{
	/*
	for (auto i = 0; i < _countof(sound_pool); i++)
	{
		if (!sound_pool[i].valid)
		{
			wcscpy(sound_pool[i].filename, filename);

			sound_pool[i].valid = true;

			SetEvent(sound_pool[i].play_event);

			printf("Start play sound at track #%d", i);

			break;
		}
	}
	*/
}

extern "C" void stop_sound(LW_SOUND lws)
{
}
