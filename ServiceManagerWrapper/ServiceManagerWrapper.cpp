#include "ServiceManagerWrapper.h"



int  ServiceManagerWrapper::DeleteSvc(String^ svcName)
{
	IntPtr ptrToNativeSvcName = Marshal::StringToHGlobalUni(svcName);
	auto res =  ServiceConsoleManager::DeleteSvc((LPCTSTR)ptrToNativeSvcName.ToPointer());
	Marshal::FreeHGlobal(ptrToNativeSvcName);

	return res;
}

int ServiceManagerWrapper::InstallSvc(String^ svcName, String^ svcDisplayName, String^ svcDecription, String^ pathToBinary)
{
	IntPtr ptrToNativeSvcName = Marshal::StringToHGlobalUni(svcName);
	IntPtr ptrToNativeSvcDisplayName = Marshal::StringToHGlobalUni(svcDisplayName);
	IntPtr ptrToNativeSvcDecription = Marshal::StringToHGlobalUni(svcDecription);
	IntPtr ptrToNativePath = Marshal::StringToHGlobalUni(pathToBinary);

	auto res =  ServiceConsoleManager::InstallSvc(
		(LPCTSTR)ptrToNativeSvcName.ToPointer(),
		(LPCTSTR)ptrToNativeSvcDisplayName.ToPointer(),
		(LPTSTR)ptrToNativeSvcDecription.ToPointer(),
		(LPTSTR)ptrToNativePath.ToPointer());

	Marshal::FreeHGlobal(ptrToNativeSvcName);
	Marshal::FreeHGlobal(ptrToNativeSvcDisplayName);
	Marshal::FreeHGlobal(ptrToNativeSvcDecription);
	Marshal::FreeHGlobal(ptrToNativePath);

	return res;
}

int ServiceManagerWrapper::StopSvc(String^ svcName, int dwMillisecondsTimeout)
{
	IntPtr ptrToNativeSvcName = Marshal::StringToHGlobalUni(svcName);
	auto res =  ServiceConsoleManager::StopSvc((LPCTSTR)ptrToNativeSvcName.ToPointer(), dwMillisecondsTimeout);
	Marshal::FreeHGlobal(ptrToNativeSvcName);
	return res;
}

int ServiceManagerWrapper::StartSvc(String^ svcName)
{
	IntPtr ptrToNativeSvcName = Marshal::StringToHGlobalUni(svcName);
	auto res =  ServiceConsoleManager::StartSvc((LPCTSTR)ptrToNativeSvcName.ToPointer());

	Marshal::FreeHGlobal(ptrToNativeSvcName);
	return res;
}

int ServiceManagerWrapper::getServiceStatus(String^ svcName)
{
	IntPtr ptrToNativeSvcName = Marshal::StringToHGlobalUni(svcName);
	auto res =  ServiceConsoleManager::getServiceStatus((LPCTSTR)ptrToNativeSvcName.ToPointer());

	Marshal::FreeHGlobal(ptrToNativeSvcName);
	return res;
}