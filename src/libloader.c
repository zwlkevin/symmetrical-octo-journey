/*
 * "libloader.c 2021-05-17 15:55:05
 *  
 *  libcups load routine for TSC Printer Driver
 *  
 *  Copyright (c) 2005, by TSC Printronix Auto ID .
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at

 *      http://www.apache.org/licenses/LICENSE-2.0

 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.

 *
 */

#include "config.h"
#include "debug.h"
#include "libloader.h"
#include "mycups.h"
#include <dlfcn.h>

void*	TryLibLocation(const char* str)
{
	void*	hmodule = dlopen(str, RTLD_NOW);

	DebugPrintf("TryLibLocation: %s, %s\n", str, hmodule ? "OK" : "Fail");

	return hmodule;
}

int	LoadCupsLibrary(CUPSLIB_FUNCTION* cupsfun)
{
	int			nRtn = 1;
	int			i;
	const char	*szLibgs[] = {
#ifdef __MACOS__
		"/usr/lib/libcups.dylib",
		"/usr/lib/libcups.2.dylib",
		"/usr/local/lib/libcups.dylib",
		"/usr/local/lib/libcups.2.dylib",
#else
#if defined(__x86_64__) || defined(__aarch64__)
		// try 64 bit libraries on 64 bit system
		"/usr/lib64/libcups.so",
		"/usr/lib64/libcups.so.2",
		"/usr/local/lib64/libcups.so",
		"/usr/local/lib64/libcups.so.2",
		"/usr/lib/x86_64-linux-gnu/libcups.so.2",
#else
		"/usr/lib/libcups.so",
		"/usr/lib/libcups.so.2",
		"/usr/local/lib/libcups.so",
		"/usr/local/lib/libcups.so.2",
		"/usr/lib/i386-linux-gnu/libcups.so.2",
		"/usr/lib/aarch64-linux-gnu/libcups.so.2",
		"/usr/lib/mipsel-linux-gnu/libcups.so.2",			//add
		
#endif  // __x86_64__
#endif
		NULL,
	};

	memset(cupsfun, 0, sizeof(CUPSLIB_FUNCTION));
	for(i=0; cupsfun->hmodule==NULL && szLibgs[i]; i++)
	{
		cupsfun->hmodule = TryLibLocation(szLibgs[i]);
	}

	if ( cupsfun->hmodule )
	{
		cupsfun->cupsGetDests = (PFN_cupsGetDests) dlsym(cupsfun->hmodule, "cupsGetDests");
		cupsfun->cupsSetDests = (PFN_cupsSetDests) dlsym(cupsfun->hmodule, "cupsSetDests");
		cupsfun->cupsFreeDests = (PFN_cupsFreeDests) dlsym(cupsfun->hmodule, "cupsFreeDests");
		cupsfun->cupsGetDest = (PFN_cupsGetDest) dlsym(cupsfun->hmodule, "cupsGetDest");
		cupsfun->cupsAddOption = (PFN_cupsAddOption) dlsym(cupsfun->hmodule,"cupsAddOption");
		cupsfun->cupsParseOptions = (PFN_cupsParseOptions) dlsym(cupsfun->hmodule, "cupsParseOptions");
		cupsfun->cupsMarkOptions = (PFN_cupsMarkOptions) dlsym(cupsfun->hmodule, "cupsMarkOptions");
		cupsfun->cupsGetOption = (PFN_cupsGetOption) dlsym(cupsfun->hmodule, "cupsGetOption");
		cupsfun->cupsFreeOptions = (PFN_cupsFreeOptions) dlsym(cupsfun->hmodule, "cupsFreeOptions");
		cupsfun->cupsGetPPD = (PFN_cupsGetPPD) dlsym(cupsfun->hmodule, "cupsGetPPD");
		cupsfun->cupsTempFile2 = (PFN_cupsTempFile2) dlsym(cupsfun->hmodule, "cupsTempFile2");

		cupsfun->ppdOpenFile = (PFN_ppdOpenFile) dlsym(cupsfun->hmodule, "ppdOpenFile");
		cupsfun->ppdClose = (PFN_ppdClose) dlsym(cupsfun->hmodule, "ppdClose");
		cupsfun->ppdMarkDefaults = (PFN_ppdMarkDefaults) dlsym(cupsfun->hmodule, "ppdMarkDefaults");
		cupsfun->ppdFindOption = (PFN_ppdFindOption) dlsym(cupsfun->hmodule, "ppdFindOption");
		cupsfun->ppdFindChoice = (PFN_ppdFindChoice) dlsym(cupsfun->hmodule, "ppdFindChoice");
		cupsfun->ppdMarkOption = (PFN_ppdMarkOption) dlsym(cupsfun->hmodule, "ppdMarkOption");
		cupsfun->ppdFindMarkedChoice = (PFN_ppdFindMarkedChoice) dlsym(cupsfun->hmodule, "ppdFindMarkedChoice");
		cupsfun->ppdPageSize = (PFN_ppdPageSize) dlsym(cupsfun->hmodule, "ppdPageSize");
		cupsfun->ppdFindAttr = (PFN_ppdFindAttr) dlsym(cupsfun->hmodule, "ppdFindAttr");
		cupsfun->ppdFindNextAttr = (PFN_ppdFindNextAttr) dlsym(cupsfun->hmodule, "ppdFindNextAttr");
		cupsfun->ppdIsMarked = (PFN_ppdIsMarked) dlsym(cupsfun->hmodule, "ppdIsMarked");
		cupsfun->ppdLocalize = (PFN_ppdLocalize) dlsym(cupsfun->hmodule, "ppdLocalize");

		cupsfun->cupsFileClose = (PFN_cupsFileClose) dlsym(cupsfun->hmodule, "cupsFileClose");
		cupsfun->cupsFileGetLine = (PFN_cupsFileGetLine) dlsym(cupsfun->hmodule, "cupsFileGetLine");
		cupsfun->cupsFileOpen = (PFN_cupsFileOpen) dlsym(cupsfun->hmodule, "cupsFileOpen");
		cupsfun->cupsFileOpenFd = (PFN_cupsFileOpenFd) dlsym(cupsfun->hmodule, "cupsFileOpenFd");
		cupsfun->cupsFileStdin = (PFN_cupsFileStdin) dlsym(cupsfun->hmodule, "cupsFileStdin");
		cupsfun->cupsFileWrite = (PFN_cupsFileWrite) dlsym(cupsfun->hmodule, "cupsFileWrite");
		cupsfun->cupsFileTell = (PFN_cupsFileTell) dlsym(cupsfun->hmodule, "cupsFileTell");
		cupsfun->cupsFileRead = (PFN_cupsFileRead) dlsym(cupsfun->hmodule, "cupsFileRead");
		cupsfun->cupsFileSeek = (PFN_cupsFileSeek) dlsym(cupsfun->hmodule, "cupsFileSeek");
		cupsfun->cupsFileRewind	= (PFN_cupsFileRewind) dlsym(cupsfun->hmodule, "cupsFileRewind");

		cupsfun->cupsArrayAdd = (PFN_cupsArrayAdd) dlsym(cupsfun->hmodule, "cupsArrayAdd");
		cupsfun->cupsArrayNew = (PFN_cupsArrayNew) dlsym(cupsfun->hmodule, "cupsArrayNew");
		cupsfun->cupsArrayCount = (PFN_cupsArrayCount) dlsym(cupsfun->hmodule, "cupsArrayCount");
		cupsfun->cupsArrayFirst = (PFN_cupsArrayFirst) dlsym(cupsfun->hmodule, "cupsArrayFirst");
		cupsfun->cupsArrayLast = (PFN_cupsArrayLast) dlsym(cupsfun->hmodule, "cupsArrayLast");
		cupsfun->cupsArrayIndex = (PFN_cupsArrayIndex) dlsym(cupsfun->hmodule, "cupsArrayIndex");

		cupsfun->cupsLangDefault = (PFN_cupsLangDefault) dlsym(cupsfun->hmodule, "cupsLangDefault");
		cupsfun->cupsLangFree = (PFN_cupsLangFree) dlsym(cupsfun->hmodule, "cupsLangFree");

		if ( cupsfun->ppdLocalize == NULL )
		{
DebugPrintf( "Use myself ppdLocalize\n");
			cupsfun->ppdLocalize = my_ppdLocalize;
		}

		if ( cupsfun->cupsTempFile2 == NULL
			|| cupsfun->cupsFileClose == NULL
			|| cupsfun->cupsFileGetLine == NULL
			|| cupsfun->cupsFileOpen == NULL
			|| cupsfun->cupsFileOpenFd == NULL
			|| cupsfun->cupsFileStdin == NULL
			|| cupsfun->cupsFileWrite == NULL
			|| cupsfun->cupsFileTell == NULL
			|| cupsfun->cupsFileRead == NULL
			|| cupsfun->cupsFileSeek == NULL
			|| cupsfun->cupsFileRewind == NULL
		)
		{
DebugPrintf( "Use myself cupsFile functions\n");
			cupsfun->cupsTempFile2 = (PFN_cupsTempFile2) my_cupsTempFile2;
			cupsfun->cupsFileClose = (PFN_cupsFileClose) my_cupsFileClose;
			cupsfun->cupsFileGetLine = (PFN_cupsFileGetLine) my_cupsFileGetLine;
			cupsfun->cupsFileOpen = (PFN_cupsFileOpen) my_cupsFileOpen;
			cupsfun->cupsFileOpenFd = (PFN_cupsFileOpenFd) my_cupsFileOpenFd;
			cupsfun->cupsFileStdin = (PFN_cupsFileStdin) my_cupsFileStdin;
			cupsfun->cupsFileWrite = (PFN_cupsFileWrite) my_cupsFileWrite;
			cupsfun->cupsFileTell = (PFN_cupsFileTell) my_cupsFileTell;
			cupsfun->cupsFileRead = (PFN_cupsFileRead) my_cupsFileRead;
			cupsfun->cupsFileSeek = (PFN_cupsFileSeek) my_cupsFileSeek;
			cupsfun->cupsFileRewind	= (PFN_cupsFileRewind) my_cupsFileRewind;
		}

		// before CUPS 1.2, not have Array funtion, use myself function
		if ( cupsfun->cupsArrayAdd == NULL
			|| cupsfun->cupsArrayNew == NULL
			|| cupsfun->cupsArrayCount == NULL
			|| cupsfun->cupsArrayFirst == NULL
			|| cupsfun->cupsArrayLast == NULL
			|| cupsfun->cupsArrayIndex == NULL
		)
		{
DebugPrintf( "Use myself cupsArray functions\n");
			cupsfun->cupsArrayAdd = (PFN_cupsArrayAdd) my_cupsArrayAdd;
			cupsfun->cupsArrayNew = (PFN_cupsArrayNew) my_cupsArrayNew;
			cupsfun->cupsArrayCount = (PFN_cupsArrayCount) my_cupsArrayCount;
			cupsfun->cupsArrayFirst = (PFN_cupsArrayFirst) my_cupsArrayFirst;
			cupsfun->cupsArrayLast = (PFN_cupsArrayLast) my_cupsArrayLast;
			cupsfun->cupsArrayIndex = (PFN_cupsArrayIndex) my_cupsArrayIndex;
		}
		if ( cupsfun->cupsLangDefault == NULL )
		{
DebugPrintf( "Use myself cupsLangDefault\n");
			cupsfun->cupsLangDefault = my_cupsLangDefault;
		}

		if ( cupsfun->cupsGetDests
			&& cupsfun->cupsFreeDests
			&& cupsfun->cupsSetDests
			&& cupsfun->cupsGetDest
			&& cupsfun->cupsAddOption
			&& cupsfun->cupsParseOptions
			&& cupsfun->cupsMarkOptions
			&& cupsfun->cupsGetOption
			&& cupsfun->cupsFreeOptions
			&& cupsfun->cupsGetPPD
			&& cupsfun->cupsTempFile2
			&& cupsfun->ppdOpenFile
			&& cupsfun->ppdClose
			&& cupsfun->ppdMarkDefaults
			&& cupsfun->ppdFindOption
			&& cupsfun->ppdFindChoice
			&& cupsfun->ppdMarkOption
			&& cupsfun->ppdFindMarkedChoice
			&& cupsfun->ppdPageSize
			&& cupsfun->ppdFindAttr
			&& cupsfun->ppdFindNextAttr
			&& cupsfun->ppdIsMarked
			&& cupsfun->ppdLocalize
			&& cupsfun->cupsFileClose
			&& cupsfun->cupsFileGetLine
			&& cupsfun->cupsFileOpen
			&& cupsfun->cupsFileOpenFd
			&& cupsfun->cupsFileStdin
			&& cupsfun->cupsFileWrite
			&& cupsfun->cupsFileTell
			&& cupsfun->cupsFileRead
			&& cupsfun->cupsFileSeek
			&& cupsfun->cupsFileRewind
			&& cupsfun->cupsArrayAdd
			&& cupsfun->cupsArrayNew
			&& cupsfun->cupsArrayCount
			&& cupsfun->cupsArrayFirst
			&& cupsfun->cupsArrayLast
			&& cupsfun->cupsArrayIndex
			&& cupsfun->cupsLangDefault
			&& cupsfun->cupsLangFree
			)
		{
			nRtn = 0;
		}

		if ( nRtn )
		{
			FreeCupsLibrary(cupsfun);
		}
	}

	return nRtn;
}

void	FreeCupsLibrary(CUPSLIB_FUNCTION* cupsfun)
{
	if (cupsfun && cupsfun->hmodule != NULL)
	{
		dlclose(cupsfun->hmodule);
		memset(cupsfun, 0, sizeof(CUPSLIB_FUNCTION));
	}
}

int	LoadGsLibrary(GSLIB_FUNCTION* gsfun)
{
	int			nRtn = 1;
	int			i;
	const char	*szLibgs[] = {
#if defined(__MACOS__)
		"/usr/lib/libgs.dylib",
		"/usr/lib/libgs.8.dylib",
		"/usr/local/lib/libgs.dylib",
		"/usr/local/lib/libgs.8.dylib",
#elif defined(__x86_64__) || defined(__aarch64__)
		// try 64 bit libraries on 64 bit system
		"/usr/lib64/libgs.so",
		"/usr/lib64/libgs.so.8",
		"/usr/local/lib64/libgs.so",
		"/usr/local/lib64/libgs.so.8",
#else
		"/usr/lib/libgs.so",
		"/usr/lib/libgs.so.8",
		"/usr/local/lib/libgs.so",
		"/usr/local/lib/libgs.so.8",
#endif
		NULL,
	};

	memset(gsfun, 0, sizeof(GSLIB_FUNCTION));
	for(i=0; gsfun->hmodule==NULL && szLibgs[i]; i++)
	{
		gsfun->hmodule = TryLibLocation(szLibgs[i]);
	}

	if ( gsfun->hmodule )
	{
		/* Get pointers to functions */
		gsfun->gsapi_revision = (PFN_gsapi_revision) dlsym(gsfun->hmodule, "gsapi_revision");
		if (gsfun->gsapi_revision)
		{
			gsapi_revision_t rv;

			/* check DLL version */
			if (gsfun->gsapi_revision(&rv, sizeof(rv)) == 0)
			{
				gsfun->gsapi_new_instance = (PFN_gsapi_new_instance) dlsym(gsfun->hmodule,"gsapi_new_instance");
				gsfun->gsapi_delete_instance = (PFN_gsapi_delete_instance) dlsym(gsfun->hmodule, "gsapi_delete_instance");
				gsfun->gsapi_set_stdio = (PFN_gsapi_set_stdio) dlsym(gsfun->hmodule, "gsapi_set_stdio");
				gsfun->gsapi_set_poll = (PFN_gsapi_set_poll) dlsym(gsfun->hmodule, "gsapi_set_poll");
				gsfun->gsapi_set_display_callback = (PFN_gsapi_set_display_callback) dlsym(gsfun->hmodule, "gsapi_set_display_callback");
				gsfun->gsapi_init_with_args = (PFN_gsapi_init_with_args) dlsym(gsfun->hmodule, "gsapi_init_with_args");
				gsfun->gsapi_run_string = (PFN_gsapi_run_string) dlsym(gsfun->hmodule, "gsapi_run_string");
				gsfun->gsapi_run_string_with_length = (PFN_gsapi_run_string_with_length) dlsym(gsfun->hmodule, "gsapi_run_string_with_length");
				gsfun->gsapi_run_string_begin = (PFN_gsapi_run_string_begin) dlsym(gsfun->hmodule, "gsapi_run_string_begin");
				gsfun->gsapi_run_string_continue = (PFN_gsapi_run_string_continue) dlsym(gsfun->hmodule, "gsapi_run_string_continue");
				gsfun->gsapi_run_string_end = (PFN_gsapi_run_string_end) dlsym(gsfun->hmodule, "gsapi_run_string_end");
				gsfun->gsapi_exit = (PFN_gsapi_exit) dlsym(gsfun->hmodule, "gsapi_exit");

				if ( gsfun->gsapi_new_instance
					&& gsfun->gsapi_delete_instance
					&& gsfun->gsapi_set_stdio
					&& gsfun->gsapi_set_poll
					&& gsfun->gsapi_set_display_callback
					&& gsfun->gsapi_init_with_args
					&& gsfun->gsapi_run_string
					&& gsfun->gsapi_run_string_with_length
					&& gsfun->gsapi_run_string_begin
					&& gsfun->gsapi_run_string_continue
					&& gsfun->gsapi_run_string_end
					&& gsfun->gsapi_exit )
				{
					nRtn = 0;
				}
			}
		}

		if ( nRtn )
		{
			FreeGsLibrary(gsfun);
		}
	}

	return nRtn;
}

void	FreeGsLibrary(GSLIB_FUNCTION* gsfun)
{
	if (gsfun && gsfun->hmodule != NULL)
	{
		dlclose(gsfun->hmodule);
		memset(gsfun, 0, sizeof(GSLIB_FUNCTION));
	}
}