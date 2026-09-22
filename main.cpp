#include <windows.h>
#include <winbase.h>
#include <winreg.h>
#include <winuser.h>
#include <string.h>
#include <iostream>

#pragma comment(lib,"Advapi32.lib");

void includeDLL(const char* groupName,const char* dll){
HKEY key=NULL;
const char *subkey="SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Svchost";
if(RegOpenKeyExA(
    HKEY_LOCAL_MACHINE,
    subkey,
    0,
    KEY_ALL_ACCESS,
    &key
)!=0){
std::cout<<"FAILED TO OPEN REGISTRY KEY"<<std::endl;
exit(1);
}


DWORD dwType;
DWORD cbData;
DWORD newSize=0;
BYTE* buff=NULL;

if(RegGetValueA(key,NULL,groupName,RRF_RT_ANY,&dwType,NULL,&cbData)==0){
    std::cout<<"getting val succ"<<std::endl;
    newSize=(cbData+strlen(dll)+1)*sizeof(char);
    buff=new BYTE[newSize];
    ZeroMemory(buff,newSize);
}else{
    std::cout<<"Failed to Get current size and type of key"<<std::endl;
    exit(1);
}

if(RegGetValueA(key,NULL,groupName,RRF_RT_ANY,&dwType,buff,&cbData)==0){
    int i=0;
    while(i<newSize-sizeof(char)){ 
        if(buff[i]==0 && buff[i+1]==0){
            ++i; 
            // now buff[i] equals to 0. buff+i is ptr to 0
            memcpy(buff+i,dll,strlen(dll)+1);  
            buff[i+strlen(dll)+2]='\0';
            break;
        }
        i++;
    }
}else{
std::cout<<"FAILED TO GET CURRENT VALUE FROM REGISTRY"<<std::endl;
exit(1);
}


if(RegSetValueExA(
    key,
    groupName,
    0,
    dwType,
    buff,
    newSize
)!=0){
    std::cout<<"FAILED TO WRITE TO SVCHOST KEY"<<std::endl;
    exit(1);
}

}

void adjustPath(const char* serviceName,const char* dllPath,const char* mainFunctionName,HKEY svcKey=NULL){
std::cout<<"ADJUST PATH EXECUTED"<<std::endl;
HKEY serviceKey=NULL;
if(!svcKey){
const char* genSubkey="SYSTEM\\CurrentControlSet\\Services\\";
char subkey[strlen(genSubkey)+strlen(serviceName)+1]={0};
strcpy(subkey,genSubkey);
strcat(subkey,serviceName);
if(RegOpenKeyExA(
    HKEY_LOCAL_MACHINE,
    subkey,
    0,
    KEY_ALL_ACCESS,
    &serviceKey
)!=0){
std::cout<<"FAILED TO OPEN REGISTRY KEY"<<std::endl;
exit(1);
}
}else{
    serviceKey=svcKey;
}
HKEY parameters=0;
RegCreateKeyExA(
    serviceKey,
    "Parameters",
    0,
    NULL,
    REG_OPTION_NON_VOLATILE,
    KEY_ALL_ACCESS,
    NULL,
    &parameters,
    NULL
);

if(!parameters){
    std::cout<<"failed to create parameters subkey"<<std::endl;
    exit(1);
}
// settin up value under Services\ifnecessary\Parameters
RegSetValueExA(
    parameters,
    "ServiceDll",
    0,
    REG_EXPAND_SZ,
    (BYTE*)dllPath,
    (strlen(dllPath)+1)
);
RegSetValueExA(
    parameters,
    "ServiceMain",
    0,
    REG_SZ,
    (BYTE*)mainFunctionName,
    (strlen(mainFunctionName)+1)
);
}

// this function is bypassed by win api CreateServiceA()...
void linkPath(const char* dllName){
// service name will be ifnecessary
// links dll name written in registry under svchost key to actual dll file
// add to HKLM\SYSTEM\CurrentControlSet\Services\sevcc
const char* subkey="SYSTEM\\CurrentControlSet\\Services";
HKEY regHandle;
if(RegOpenKeyExA(
    HKEY_LOCAL_MACHINE,
    subkey,
    0,
    KEY_ALL_ACCESS,
    &regHandle
)!=0){
    std::cout<<"FAILED TO OPEN HKLM\\SYSTEM\\CurrentControlSet\\Services"<<std::endl;
    exit(1);
}

// handle to ...\...\ifnecessary
HKEY hCreated=0;
RegCreateKeyExA(
    regHandle,
    dllName,
    0,
    NULL,
    REG_OPTION_VOLATILE,
    KEY_ALL_ACCESS,
    NULL,
    &hCreated,
    NULL
);
if(hCreated==0){
    std::cout<<"CANNOT CREATE SUBKEY IN SERVICES"<<std::endl;
    exit(1);
}
//required values for subkey
const char* root="C:\\Windows\\System32\\";
const char* ImagePath="C:\\Windows\\System32\\svchost.exe -k netsvcs";
char ServiceDll[strlen(root)+strlen(dllName)+1]={0};
strcpy(ServiceDll,root);
strcat(ServiceDll,dllName);
const char* ServiceMain="ServiceMain";
DWORD Start=0x3;
DWORD Type=SERVICE_WIN32_OWN_PROCESS;
DWORD ErrorControl=0x0;
const char* ObjectName="NT AUTHORITY\\NetworkService";
// setting value under Services\ifnecessary
RegSetValueExA(
    hCreated,
    "ImagePath",
    0,
    REG_EXPAND_SZ, 
    (BYTE*)ImagePath,
    (DWORD)(strlen(ImagePath)+1)
);
RegSetValueExA(
    hCreated,
    "Start",
    0,
    REG_DWORD,
    (BYTE*)&Start,
    sizeof(DWORD)
);
RegSetValueExA(
    hCreated,
    "Type",
    0,
    REG_DWORD,
    (BYTE*)&Type,
    sizeof(DWORD)
);
RegSetValueExA(
    hCreated,
    "ErrorControl",
    0,
    REG_DWORD,
    (BYTE*)&ErrorControl,
    sizeof(DWORD)
);

 adjustPath(
    "ifnecessary",
    "C:\\Windows\\System32\\ifnecessary.dll",
    "ServiceMain",
    hCreated
);

std::cout<<"path has been linked"<<std::endl;
}



void registerServiceGroup(const char* name){
    HKEY svchost=0;
    const char* subkey="Software\\Microsoft\\Windows NT\\CurrentVersion\\Svchost";
    if(RegOpenKeyExA(
        HKEY_LOCAL_MACHINE,
        subkey,
        0,
        KEY_ALL_ACCESS,
        &svchost
    )!=0){
        std::cout<<"[-] registerServiceGroup"<<std::endl;
        exit(1);
    }
    const char* val="ifnecessary\0\0";

    if(RegSetValueExA(
        svchost,
        "ifnec",
        0,
        REG_MULTI_SZ,
        (BYTE*)val,
        (DWORD)(strlen(val)+2)
    )!=0){
        std::cout<<"[-] registerServiceGroup"<<std::endl;
        exit(1);
    }
    std::cout<<"REGISTERED GROUP"<<std::endl;
}

void undocumented(){
    includeDLL("netsvcs","ifnecessary");
    linkPath("ifnecessary");

}



void documented(bool customGrp){
    // assuming custmGrp = ifnec
    // assuming defaultGrp = netsvcs
    if(customGrp){
        registerServiceGroup("ifnec");
    }else{
        // or  whatever group instead of netsvcs
        includeDLL("netsvcs","ifnecessary");
    }
    SC_HANDLE SCM=OpenSCManager(
        NULL,
        NULL,
        SC_MANAGER_ALL_ACCESS
    );

    // createService abstacts what i did with linkPath() function 
    // + createservice syncs SCM unlike my undocumented approach.
    SC_HANDLE service;
    if(customGrp){
        service=CreateServiceA(
        SCM,
        "ifnecessary",
        "ifnecessary",
        SERVICE_ALL_ACCESS,
        SERVICE_WIN32_SHARE_PROCESS,
        SERVICE_DEMAND_START,
        SERVICE_ERROR_IGNORE,
        "C:\\Windows\\System32\\svchost.exe -k ifnec",
        NULL,
        NULL,
        NULL,
        NULL,
        NULL
    );
}else{
        service=CreateServiceA(
        SCM,
        "ifnecessary",
        "ifnecessary",
        SERVICE_ALL_ACCESS,
        SERVICE_WIN32_SHARE_PROCESS,
        SERVICE_AUTO_START, 
        SERVICE_ERROR_IGNORE,
        "C:\\Windows\\System32\\svchost.exe -k netsvcs",
        NULL,
        NULL,
        NULL,
        NULL,
        NULL
    );
}
if(!service){
        std::cout<<"[-] CreateServiceA(...)"<<std::endl;
        exit(1);
    }
adjustPath("ifnecessary","C:\\Windows\\System32\\ifnecessary.dll","ServiceMain");
   
}

int main(void){
    documented(false);
    std::cout<<"---------------------0xifnecessary0x-------------------------"<<std::endl;
    return 0;
}