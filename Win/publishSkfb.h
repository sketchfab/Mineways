#include "ExportPrint.h"
#include<string>

class PublishSkfb
{
public:
    PublishSkfb(void);
    ~PublishSkfb(void);
};

void getPublishSkfbData(ExportFileData *pEpd);
void setPublishSkfbData(ExportFileData *pEpd);
int doPublishSkfb(HINSTANCE hInst,HWND hWnd);
int uploadToSketchfab(wchar_t* wcZip, HWND progressBar, std::string api_token, std::string name, std::string description, std::string tags, bool draft, bool usePrivate, std::string password);
